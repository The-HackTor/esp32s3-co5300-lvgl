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

static bool decode_flipper(const uint16_t *timings, size_t n_timings, IrDecoded *out)
{
    /* Per-call alloc/free. The earlier singleton optimization retained
     * corrupted decoder state across calls (InstrFetchProhibited at a wild
     * function-pointer jump from inside common_decode meant decoder->protocol
     * was clobbered between calls). Now that Learn scene is the sole RX
     * consumer (ir_app_rx_pause/_resume bracket Learn on_enter/_exit), the
     * alloc/free pressure that motivated the singleton is bounded to the
     * actual user-driven capture cadence -- a few frames per session, not
     * 33 Hz constant. Pay the small alloc cost, get a clean decoder each
     * time. */
    InfraredDecoderHandler *handler = infrared_alloc_decoder();
    if(!handler) return false;

    /* Feed alternating (level, duration) pairs. Mark on even indices. */
    for(size_t i = 0; i < n_timings; i++) {
        const bool level = ((i & 1) == 0);
        const InfraredMessage *msg = infrared_decode(handler, level, timings[i]);
        if(msg) {
            copy_decoded(out, msg);
            infrared_free_decoder(handler);
            return true;
        }
    }
    /* SIRC family confirms the message only at end-of-frame timeout. */
    const InfraredMessage *msg = infrared_check_decoder_ready(handler);
    if(msg) {
        copy_decoded(out, msg);
        infrared_free_decoder(handler);
        return true;
    }
    infrared_free_decoder(handler);
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

    size_t cap = 256;
    size_t n   = 0;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) { infrared_free_encoder(handler); return ESP_ERR_NO_MEM; }

    bool started = false;
    bool expected_level = true;
    for(;;) {
        uint32_t duration = 0;
        bool     level    = false;
        InfraredStatus s = infrared_encode(handler, &duration, &level);
        if(s == InfraredStatusError) {
            free(buf);
            infrared_free_encoder(handler);
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
                if(!r) { free(buf); infrared_free_encoder(handler); return ESP_ERR_NO_MEM; }
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

    if(n == 0) {
        free(buf);
        infrared_free_encoder(handler);
        return ESP_FAIL;
    }

    *out_timings = buf;
    *out_n       = n;
    *out_freq_hz = infrared_get_protocol_frequency(proto);

    infrared_free_encoder(handler);
    return ESP_OK;
}
