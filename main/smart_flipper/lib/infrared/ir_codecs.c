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
