#include "ir_brute.h"

#include "lib/infrared/universal_db/ir_universal_db.h"
#include "lib/infrared/ac/ac_brand.h"
#include "lib/infrared/ir_codecs.h"
#include "lib/infrared/encoder_decoder/infrared.h"
#include "hw/hw_ir.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <stdlib.h>
#include <string.h>

#define BRUTE_INTER_FRAME_MS  110

static const char *BRUTE_TAG = "ir_brute";
static volatile bool s_log_next;

void ir_brute_log_next_send(void) { s_log_next = true; }

static const AcState s_ac_default = {
    .power  = true,
    .mode   = AC_MODE_COOL,
    .temp_c = 24,
    .fan    = AC_FAN_AUTO,
    .swing  = false,
};

static bool ac_button_is_power(const IrBruteContext *bc)
{
    if(bc->cat != IR_UNIVERSAL_CAT_AC) return false;
    const char *name = ir_universal_db_button_name(bc->cat, (size_t)bc->button_idx);
    if(!name) return false;
    return strcasestr(name, "POWER") != NULL || strcasestr(name, "ON") != NULL;
}

void ir_brute_init(IrBruteContext *bc, IrUniversalCategory cat, int button_idx)
{
    bc->cat         = cat;
    bc->button_idx  = button_idx;
    bc->db_count    = ir_universal_db_signal_count(cat, (size_t)button_idx);
    bc->brand_count = ac_button_is_power(bc) ? ac_brand_count : 0;
}

size_t ir_brute_total(const IrBruteContext *bc)
{
    return bc->db_count + bc->brand_count;
}

bool ir_brute_step_info(const IrBruteContext *bc, size_t idx, IrBruteStepInfo *out)
{
    if(!out || idx >= ir_brute_total(bc)) return false;
    memset(out, 0, sizeof(*out));
    out->button_label = ir_universal_db_button_name(bc->cat, (size_t)bc->button_idx);

    if(idx < bc->db_count) {
        const IrButton *b = ir_universal_db_button_signal(bc->cat,
                                                          (size_t)bc->button_idx, idx);
        if(!b) return false;
        out->is_brand   = false;
        out->brand_name = b->name;
        out->protocol   = (b->signal.type == INFRARED_SIGNAL_PARSED)
                              ? b->signal.parsed.protocol : "RAW";
        return true;
    }

    size_t bi = idx - bc->db_count;
    if(bi >= ac_brand_count) return false;
    const AcBrand *br = ac_brand_table[bi];
    out->is_brand   = true;
    out->brand_name = br ? br->name : "?";
    out->protocol   = "AC";
    return true;
}

static esp_err_t fire_n(const uint16_t *t, size_t n, uint32_t hz, uint8_t repeat)
{
    if(!t || n == 0) return ESP_ERR_INVALID_STATE;
    if(repeat == 0) repeat = 1;
    esp_err_t err = ESP_OK;
    for(uint8_t i = 0; i < repeat; i++) {
        err = hw_ir_send_raw(t, n, hz);
        if(err != ESP_OK) break;
        if(i + 1 < repeat) {
            vTaskDelay(pdMS_TO_TICKS(BRUTE_INTER_FRAME_MS));
        }
    }
    return err;
}

static esp_err_t resolve_db_signal(const IrButton *b,
                                   uint16_t **out_t, size_t *out_n, uint32_t *out_hz,
                                   bool *out_owned, const char **out_proto)
{
    *out_t = NULL; *out_n = 0; *out_hz = 38000; *out_owned = false;
    if(out_proto) *out_proto = NULL;
    if(!b) return ESP_ERR_INVALID_ARG;

    if(b->signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &b->signal.raw;
        if(!r->timings || r->n_timings == 0) return ESP_ERR_INVALID_STATE;
        *out_t   = r->timings;
        *out_n   = r->n_timings;
        *out_hz  = r->freq_hz ? r->freq_hz : 38000;
        *out_owned = false;
        if(out_proto) *out_proto = "RAW";
        return ESP_OK;
    }

    IrDecoded msg = {0};
    snprintf(msg.protocol, sizeof(msg.protocol), "%s", b->signal.parsed.protocol);
    msg.address = b->signal.parsed.address;
    msg.command = b->signal.parsed.command;

    uint16_t *enc_t  = NULL;
    size_t    enc_n  = 0;
    uint32_t  enc_hz = 38000;
    esp_err_t err = ir_codecs_encode(&msg, &enc_t, &enc_n, &enc_hz);
    if(err != ESP_OK) return err;
    if(!enc_t || enc_n == 0) {
        free(enc_t);
        return ESP_FAIL;
    }
    *out_t   = enc_t;
    *out_n   = enc_n;
    *out_hz  = enc_hz;
    *out_owned = true;
    if(out_proto) *out_proto = b->signal.parsed.protocol;
    return ESP_OK;
}

static esp_err_t encode_brand_to_buf(size_t brand_idx,
                                     uint16_t **out_t, size_t *out_n,
                                     uint32_t *out_hz)
{
    if(brand_idx >= ac_brand_count) return ESP_ERR_INVALID_ARG;
    const AcBrand *br = ac_brand_table[brand_idx];
    if(!br || !br->encode) return ESP_ERR_NOT_SUPPORTED;
    return br->encode(&s_ac_default, out_t, out_n, out_hz);
}

/* Per-protocol inter-frame silence_time (ms). Pulled from
 * lib/infrared/encoder_decoder/<proto>/infrared_protocol_<proto>_i.h. */
static uint16_t silence_ms_for_proto(const char *proto)
{
    if(!proto) return 110;
    if(strcmp(proto, "NEC") == 0)       return 110;
    if(strcmp(proto, "NECext") == 0)    return 110;
    if(strcmp(proto, "NEC42") == 0)     return 110;
    if(strcmp(proto, "NEC42ext") == 0)  return 110;
    if(strcmp(proto, "SIRC") == 0)      return 10;
    if(strcmp(proto, "SIRC15") == 0)    return 10;
    if(strcmp(proto, "SIRC20") == 0)    return 10;
    if(strcmp(proto, "Samsung32") == 0) return 145;
    if(strcmp(proto, "RC5") == 0)       return 27;
    if(strcmp(proto, "RC5X") == 0)      return 27;
    if(strcmp(proto, "RC6") == 0)       return 27;
    if(strcmp(proto, "Pioneer") == 0)   return 26;
    if(strcmp(proto, "Kaseikyo") == 0)  return 130;
    if(strcmp(proto, "RCA") == 0)       return 8;
    return 110;
}

esp_err_t ir_brute_step_encode(const IrBruteContext *bc, size_t idx,
                               uint16_t **out_timings, size_t *out_n,
                               uint32_t *out_freq_hz, uint8_t *out_min_repeat,
                               uint16_t *out_silence_ms)
{
    if(!bc || !out_timings || !out_n || !out_freq_hz) return ESP_ERR_INVALID_ARG;
    *out_timings = NULL; *out_n = 0; *out_freq_hz = 38000;
    if(out_min_repeat) *out_min_repeat = 1;
    if(out_silence_ms) *out_silence_ms = 110;

    uint16_t *t = NULL;
    size_t    n = 0;
    uint32_t  hz = 38000;
    bool      owned = false;
    const char *proto = NULL;
    bool      is_brand = false;
    esp_err_t err;

    if(idx < bc->db_count) {
        const IrButton *b = ir_universal_db_button_signal(bc->cat,
                                                          (size_t)bc->button_idx, idx);
        err = resolve_db_signal(b, &t, &n, &hz, &owned, &proto);
    } else {
        size_t bi = idx - bc->db_count;
        err = encode_brand_to_buf(bi, &t, &n, &hz);
        owned = (err == ESP_OK);
        is_brand = true;
        if(err == ESP_OK && bi < ac_brand_count && ac_brand_table[bi]) {
            proto = ac_brand_table[bi]->name;
        }
    }
    if(err != ESP_OK) return err;
    if(!t || n == 0) {
        if(owned) free(t);
        return ESP_FAIL;
    }

    if(s_log_next) {
        s_log_next = false;
        uint32_t addr = 0, cmd = 0;
        if(idx < bc->db_count) {
            const IrButton *b = ir_universal_db_button_signal(bc->cat,
                                                              (size_t)bc->button_idx, idx);
            if(b && b->signal.type == INFRARED_SIGNAL_PARSED) {
                addr = b->signal.parsed.address;
                cmd  = b->signal.parsed.command;
            }
        }
        ESP_LOGI(BRUTE_TAG,
                 "step idx=%u brand=%d proto=%s addr=0x%lX cmd=0x%lX n=%u freq=%lu",
                 (unsigned)idx, (int)is_brand, proto ? proto : "?",
                 (unsigned long)addr, (unsigned long)cmd,
                 (unsigned)n, (unsigned long)hz);
    }

    if(owned) {
        *out_timings = t;
    } else {
        /* Caller-frees contract: copy DB-RAM signals onto the heap. */
        uint16_t *copy = malloc(n * sizeof(uint16_t));
        if(!copy) return ESP_ERR_NO_MEM;
        memcpy(copy, t, n * sizeof(uint16_t));
        *out_timings = copy;
    }
    *out_n       = n;
    *out_freq_hz = hz;
    if(out_min_repeat) {
        if(is_brand || !proto) {
            *out_min_repeat = 1;
        } else {
            InfraredProtocol p = infrared_get_protocol_by_name(proto);
            if(infrared_is_protocol_valid(p)) {
                size_t mr = infrared_get_protocol_min_repeat_count(p);
                *out_min_repeat = (mr > 0 && mr < 32) ? (uint8_t)mr : 1;
            } else {
                *out_min_repeat = 1;
            }
        }
    }
    if(out_silence_ms) {
        *out_silence_ms = (is_brand || !proto) ? 110 : silence_ms_for_proto(proto);
    }
    return ESP_OK;
}

esp_err_t ir_brute_step_send(const IrBruteContext *bc, size_t idx, uint8_t repeat)
{
    uint16_t *t = NULL;
    size_t    n = 0;
    uint32_t  hz = 38000;
    uint8_t   mr = 1;
    uint16_t  sil = 110;
    esp_err_t err = ir_brute_step_encode(bc, idx, &t, &n, &hz, &mr, &sil);
    if(err != ESP_OK) return err;
    uint8_t reps = repeat > mr ? repeat : mr;
    err = fire_n(t, n, hz, reps);
    free(t);
    return err;
}

esp_err_t ir_brute_step_to_button(const IrBruteContext *bc, size_t idx, IrButton *out)
{
    if(!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    if(idx < bc->db_count) {
        const IrButton *b = ir_universal_db_button_signal(bc->cat,
                                                          (size_t)bc->button_idx, idx);
        if(!b) return ESP_ERR_INVALID_STATE;
        return ir_button_dup(out, b);
    }

    size_t bi = idx - bc->db_count;
    if(bi >= ac_brand_count) return ESP_ERR_INVALID_ARG;
    const AcBrand *br = ac_brand_table[bi];

    uint16_t *t = NULL;
    size_t    n = 0;
    uint32_t  hz = 38000;
    esp_err_t err = encode_brand_to_buf(bi, &t, &n, &hz);
    if(err != ESP_OK) return err;

    snprintf(out->name, sizeof(out->name), "%s", br ? br->name : "AC");
    out->signal.type            = INFRARED_SIGNAL_RAW;
    out->signal.raw.freq_hz     = hz;
    out->signal.raw.duty_pct    = 33;
    out->signal.raw.timings     = t;
    out->signal.raw.n_timings   = n;
    return ESP_OK;
}
