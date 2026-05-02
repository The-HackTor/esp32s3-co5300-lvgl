/*
 * Pure-C IR codec dispatcher built on Flipper's encoder_decoder/ codecs.
 *
 * The previous slice 7b added an IRremoteESP8266 fallback for AC remotes,
 * but that pulled in a C++ codebase plus an Arduino runtime shim and hit
 * a libstdc++ cxx11 ABI-tag mismatch at link time. Both the C++ and the
 * Arduino glue are obstacles to the upcoming Zephyr port, so the fallback
 * tier is dropped and the dispatcher is C-only.
 *
 * Coverage: NEC, NECext, NEC42, NEC42ext, Samsung32, RC5/X, RC6, SIRC/15/20,
 * Kaseikyo, Pioneer, RCA. AC remotes (Daikin, Mitsubishi, etc.) require a
 * portable codec source and are deferred.
 */

#include "ir_codecs.h"
#include "infrared.h"
#include "codec_db.h"
#include "codec_db_send.h"

#include "nec/infrared_protocol_nec.h"
#include "samsung/infrared_protocol_samsung.h"
#include "rc5/infrared_protocol_rc5.h"
#include "rc6/infrared_protocol_rc6.h"
#include "sirc/infrared_protocol_sirc.h"
#include "kaseikyo/infrared_protocol_kaseikyo.h"
#include "pioneer/infrared_protocol_pioneer.h"
#include "rca/infrared_protocol_rca.h"

#include <stdlib.h>
#include <string.h>

static void copy_decoded(IrDecoded *out, const InfraredMessage *msg)
{
    out->source = IR_DECODED_FLIPPER;
    const char *name = infrared_get_protocol_name(msg->protocol);
    if(name) {
        strncpy(out->protocol, name, sizeof(out->protocol) - 1);
        out->protocol[sizeof(out->protocol) - 1] = '\0';
    }
    out->address   = msg->address;
    out->command   = msg->command;
    out->repeat    = msg->repeat;
    out->raw_value = 0;
}

typedef void*               (*proto_alloc_fn)(void);
typedef void                (*proto_free_fn)(void*);
typedef InfraredMessage*    (*proto_decode_fn)(void*, bool, uint32_t);
typedef InfraredMessage*    (*proto_ready_fn)(void*);

typedef struct {
    proto_alloc_fn  alloc;
    proto_decode_fn decode;
    proto_ready_fn  ready;
    proto_free_fn   free;
} ProtoEntry;

/* Truly isolated per-protocol decode. The Flipper-shipped dispatcher
 * (infrared_alloc_decoder + infrared_decode) allocates ALL 8 protocol
 * decoders simultaneously and runs them in parallel for every timing
 * sample. We were crashing reliably at rc6 (4th in dispatch order) with
 * InstrFetchProhibited at the indirect call decoder->protocol->decode --
 * the rc6 common_decoder->protocol pointer was being clobbered by an
 * adjacent protocol's per-sample state writes. Heap-poisoning canaries
 * cannot catch this because the writes are all to legal user-block
 * memory, just into the wrong protocol's chunk.
 *
 * Fix: alloc ONE protocol's decoder, feed the full buffer to it alone,
 * free, move to next. No two protocols are ever alive at the same time,
 * so no cross-protocol corruption is physically possible. First match
 * wins. Order matches Flipper's encoder_decoder[] table for behaviour
 * parity. */
static const ProtoEntry s_protos[] = {
    { infrared_decoder_nec_alloc,      infrared_decoder_nec_decode,
      infrared_decoder_nec_check_ready, infrared_decoder_nec_free },
    { infrared_decoder_samsung32_alloc, infrared_decoder_samsung32_decode,
      infrared_decoder_samsung32_check_ready, infrared_decoder_samsung32_free },
    { infrared_decoder_rc5_alloc,      infrared_decoder_rc5_decode,
      infrared_decoder_rc5_check_ready, infrared_decoder_rc5_free },
    { infrared_decoder_rc6_alloc,      infrared_decoder_rc6_decode,
      infrared_decoder_rc6_check_ready, infrared_decoder_rc6_free },
    { infrared_decoder_sirc_alloc,     infrared_decoder_sirc_decode,
      infrared_decoder_sirc_check_ready, infrared_decoder_sirc_free },
    { infrared_decoder_kaseikyo_alloc, infrared_decoder_kaseikyo_decode,
      infrared_decoder_kaseikyo_check_ready, infrared_decoder_kaseikyo_free },
    { infrared_decoder_pioneer_alloc,  infrared_decoder_pioneer_decode,
      infrared_decoder_pioneer_check_ready, infrared_decoder_pioneer_free },
    { infrared_decoder_rca_alloc,      infrared_decoder_rca_decode,
      infrared_decoder_rca_check_ready, infrared_decoder_rca_free },
};

static bool decode_flipper(const uint16_t *timings, size_t n_timings, IrDecoded *out)
{
    for(size_t pi = 0; pi < sizeof(s_protos)/sizeof(s_protos[0]); pi++) {
        const ProtoEntry *p = &s_protos[pi];

        void *ctx = p->alloc();
        if(!ctx) continue;

        const InfraredMessage *msg = NULL;
        for(size_t i = 0; i < n_timings; i++) {
            const bool level = ((i & 1) == 0);
            msg = p->decode(ctx, level, timings[i]);
            if(msg) break;
        }
        if(!msg && p->ready) msg = p->ready(ctx);

        bool matched = false;
        if(msg) {
            copy_decoded(out, msg);
            matched = true;
        }
        p->free(ctx);
        if(matched) return true;
    }
    return false;
}

static bool decode_codec_db(const uint16_t *timings, size_t n_timings, IrDecoded *out)
{
    ir_decode_result_t db_out;
    const char *name = NULL;
    if(!codec_db_decode(timings, n_timings, &db_out, &name)) return false;

    out->source = IR_DECODED_IRREMOTEESP;   /* second-tier source flag */
    if(name) {
        strncpy(out->protocol, name, sizeof(out->protocol) - 1);
        out->protocol[sizeof(out->protocol) - 1] = '\0';
    }
    out->address   = db_out.address;
    out->command   = db_out.command;
    out->raw_value = db_out.value;
    out->repeat    = db_out.repeat;
    return true;
}

bool ir_codecs_decode(const uint16_t *timings, size_t n_timings, IrDecoded *out)
{
    if(!out || !timings || n_timings == 0) return false;
    memset(out, 0, sizeof(*out));

    if(decode_flipper(timings, n_timings, out))   return true;
    if(decode_codec_db(timings, n_timings, out))  return true;
    return false;
}

/* Drain one Done-terminated cycle from the encoder into a fresh buffer.
 * Skips leading silence so out_buf[0] is always a mark. Returns ESP_OK
 * with n==0 if the cycle produced nothing usable (e.g., encode error
 * before the first mark). */
static esp_err_t encode_one_cycle(InfraredEncoderHandler *handler,
                                  uint16_t **out_buf, size_t *out_n)
{
    size_t cap = 256;
    size_t n   = 0;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    bool started = false;
    bool expected_level = true;
    for(;;) {
        uint32_t duration = 0;
        bool     level    = false;
        InfraredStatus s = infrared_encode(handler, &duration, &level);
        if(s == InfraredStatusError) {
            free(buf);
            return ESP_FAIL;
        }
        if(!started) {
            if(!level) {
                if(s == InfraredStatusDone) break;
                continue;
            }
            started = true;
        }
        if(duration > 32767) duration = 32767;
        if(level == expected_level) {
            if(n == cap) {
                size_t new_cap = cap * 2;
                uint16_t *r = realloc(buf, new_cap * sizeof(uint16_t));
                if(!r) { free(buf); return ESP_ERR_NO_MEM; }
                buf = r;
                cap = new_cap;
            }
            buf[n++] = (uint16_t)duration;
            expected_level = !expected_level;
        } else if(n > 0) {
            uint32_t merged = (uint32_t)buf[n - 1] + duration;
            if(merged > 32767) merged = 32767;
            buf[n - 1] = (uint16_t)merged;
        }
        if(s == InfraredStatusDone) break;
    }

    if(n == 0) { free(buf); *out_buf = NULL; *out_n = 0; return ESP_OK; }
    *out_buf = buf;
    *out_n   = n;
    return ESP_OK;
}

esp_err_t ir_codecs_encode(const IrDecoded *in,
                           uint16_t **out_timings, size_t *out_n,
                           uint32_t *out_freq_hz)
{
    if(!in || !out_timings || !out_n || !out_freq_hz) return ESP_ERR_INVALID_ARG;

    InfraredProtocol proto = infrared_get_protocol_by_name(in->protocol);
    if(!infrared_is_protocol_valid(proto)) {
        ir_codec_send_fn fn = codec_db_send_lookup(in->protocol);
        if(fn) return fn(in->address, in->command, in->raw_value,
                         out_timings, out_n, out_freq_hz);
        return ESP_ERR_NOT_SUPPORTED;
    }

    InfraredEncoderHandler *handler = infrared_alloc_encoder();
    if(!handler) return ESP_ERR_NO_MEM;

    InfraredMessage msg = {
        .protocol = proto,
        .address  = in->address,
        .command  = in->command,
        .repeat   = false,
    };
    infrared_reset_encoder(handler, &msg);

    uint16_t *buf = NULL;
    size_t    n   = 0;
    esp_err_t err = encode_one_cycle(handler, &buf, &n);
    infrared_free_encoder(handler);
    if(err != ESP_OK) return err;
    if(n == 0)        return ESP_FAIL;

    *out_timings = buf;
    *out_n       = n;
    *out_freq_hz = infrared_get_protocol_frequency(proto);
    return ESP_OK;
}

size_t ir_codecs_min_repeat_count(const char *protocol)
{
    if(!protocol || !protocol[0]) return 1;
    InfraredProtocol p = infrared_get_protocol_by_name(protocol);
    if(!infrared_is_protocol_valid(p)) return 1;
    size_t n = infrared_get_protocol_min_repeat_count(p);
    return n ? n : 1;
}

static esp_err_t encode_n_cycles(InfraredEncoderHandler *handler, size_t cycles,
                                 uint16_t **out_buf, size_t *out_n)
{
    size_t cap = 256;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    bool   started = false;
    bool   expected_level = true;
    size_t cycles_done = 0;

    while(cycles_done < cycles) {
        uint32_t duration = 0;
        bool     level    = false;
        InfraredStatus s = infrared_encode(handler, &duration, &level);
        if(s == InfraredStatusError) { free(buf); return ESP_FAIL; }

        if(!started) {
            if(!level) {
                if(s == InfraredStatusDone) cycles_done++;
                continue;
            }
            started = true;
        }

        if(duration > 32767) duration = 32767;

        if(level == expected_level) {
            if(n == cap) {
                size_t new_cap = cap * 2;
                uint16_t *r = realloc(buf, new_cap * sizeof(uint16_t));
                if(!r) { free(buf); return ESP_ERR_NO_MEM; }
                buf = r;
                cap = new_cap;
            }
            buf[n++] = (uint16_t)duration;
            expected_level = !expected_level;
        } else if(n > 0) {
            uint32_t merged = (uint32_t)buf[n - 1] + duration;
            if(merged > 32767) merged = 32767;
            buf[n - 1] = (uint16_t)merged;
        }

        if(s == InfraredStatusDone) cycles_done++;
    }

    if(n == 0) { free(buf); *out_buf = NULL; *out_n = 0; return ESP_FAIL; }
    *out_buf = buf;
    *out_n   = n;
    return ESP_OK;
}

esp_err_t ir_codecs_encode_full(const IrDecoded *in, size_t cycles,
                                uint16_t **out_timings, size_t *out_n,
                                uint32_t *out_freq_hz)
{
    if(!in || !out_timings || !out_n || !out_freq_hz) return ESP_ERR_INVALID_ARG;
    if(cycles == 0) cycles = 1;

    InfraredProtocol proto = infrared_get_protocol_by_name(in->protocol);
    if(!infrared_is_protocol_valid(proto)) {
        ir_codec_send_fn fn = codec_db_send_lookup(in->protocol);
        if(fn) return fn(in->address, in->command, in->raw_value,
                         out_timings, out_n, out_freq_hz);
        return ESP_ERR_NOT_SUPPORTED;
    }

    InfraredEncoderHandler *handler = infrared_alloc_encoder();
    if(!handler) return ESP_ERR_NO_MEM;

    InfraredMessage msg = {
        .protocol = proto,
        .address  = in->address,
        .command  = in->command,
        .repeat   = false,
    };
    infrared_reset_encoder(handler, &msg);

    uint16_t *buf = NULL;
    size_t    n   = 0;
    esp_err_t err = encode_n_cycles(handler, cycles, &buf, &n);
    infrared_free_encoder(handler);
    if(err != ESP_OK) return err;

    *out_timings = buf;
    *out_n       = n;
    *out_freq_hz = infrared_get_protocol_frequency(proto);
    return ESP_OK;
}

esp_err_t ir_codecs_encode_with_repeat(const IrDecoded *in,
                                       uint16_t **out_timings,        size_t *out_n,
                                       uint16_t **out_repeat_timings, size_t *out_repeat_n,
                                       uint32_t *out_freq_hz)
{
    if(!in || !out_timings || !out_n ||
       !out_repeat_timings || !out_repeat_n || !out_freq_hz) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_repeat_timings = NULL;
    *out_repeat_n       = 0;

    InfraredProtocol proto = infrared_get_protocol_by_name(in->protocol);
    if(!infrared_is_protocol_valid(proto)) {
        /* codec_db / IRremoteESP8266 protocols don't have a stateful
         * Flipper-style repeat encoder. Fall back to a single-frame
         * encode -- caller should re-fire the full frame on hold. */
        ir_codec_send_fn fn = codec_db_send_lookup(in->protocol);
        if(fn) return fn(in->address, in->command, in->raw_value,
                         out_timings, out_n, out_freq_hz);
        return ESP_ERR_NOT_SUPPORTED;
    }

    InfraredEncoderHandler *handler = infrared_alloc_encoder();
    if(!handler) return ESP_ERR_NO_MEM;

    InfraredMessage msg = {
        .protocol = proto,
        .address  = in->address,
        .command  = in->command,
        .repeat   = false,
    };
    infrared_reset_encoder(handler, &msg);

    /* Phase 1: full frame. */
    uint16_t *frame_buf = NULL;
    size_t    frame_n   = 0;
    esp_err_t err = encode_one_cycle(handler, &frame_buf, &frame_n);
    if(err != ESP_OK || frame_n == 0) {
        infrared_free_encoder(handler);
        if(frame_buf) free(frame_buf);
        return err == ESP_OK ? ESP_FAIL : err;
    }

    /* Phase 2: one repeat cycle. The encoder is in EncodeRepeat (NEC,
     * Samsung, ...) or Silence (RC5, RC6, Pioneer, RCA, ...) state and
     * keeps timings_sum across the transition. Either way, draining
     * another cycle yields the protocol-correct hold sequence. */
    uint16_t *rep_buf = NULL;
    size_t    rep_n   = 0;
    err = encode_one_cycle(handler, &rep_buf, &rep_n);
    if(err != ESP_OK) {
        infrared_free_encoder(handler);
        free(frame_buf);
        if(rep_buf) free(rep_buf);
        return err;
    }

    *out_timings        = frame_buf;
    *out_n              = frame_n;
    *out_repeat_timings = rep_buf;
    *out_repeat_n       = rep_n;
    *out_freq_hz        = infrared_get_protocol_frequency(proto);

    infrared_free_encoder(handler);
    return ESP_OK;
}
