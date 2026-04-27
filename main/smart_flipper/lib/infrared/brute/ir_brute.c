#include "ir_brute.h"

#include "lib/infrared/universal_db/ir_universal_db.h"
#include "lib/infrared/ac/ac_brand.h"
#include "lib/infrared/ir_codecs.h"
#include "hw/hw_ir.h"

#include <stdlib.h>
#include <string.h>

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

static esp_err_t send_db_signal(const IrButton *b)
{
    if(!b) return ESP_ERR_INVALID_ARG;
    if(b->signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &b->signal.raw;
        if(!r->timings || r->n_timings == 0) return ESP_ERR_INVALID_STATE;
        return hw_ir_send_raw(r->timings, r->n_timings,
                              r->freq_hz ? r->freq_hz : 38000);
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
    if(enc_t && enc_n) err = hw_ir_send_raw(enc_t, enc_n, enc_hz);
    free(enc_t);
    return err;
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

esp_err_t ir_brute_step_send(const IrBruteContext *bc, size_t idx)
{
    if(idx < bc->db_count) {
        const IrButton *b = ir_universal_db_button_signal(bc->cat,
                                                          (size_t)bc->button_idx, idx);
        return send_db_signal(b);
    }

    size_t bi = idx - bc->db_count;
    uint16_t *t = NULL;
    size_t    n = 0;
    uint32_t  hz = 38000;
    esp_err_t err = encode_brand_to_buf(bi, &t, &n, &hz);
    if(err != ESP_OK) return err;
    if(t && n) err = hw_ir_send_raw(t, n, hz);
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
