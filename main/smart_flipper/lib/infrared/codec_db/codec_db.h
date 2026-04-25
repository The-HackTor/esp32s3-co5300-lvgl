#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_DB_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_DB_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IR_CODEC_UNKNOWN = 0,
    IR_CODEC_AIRTON,
    IR_CODEC_AIWA_RC_T501,
    IR_CODEC_AMCOR,
    IR_CODEC_ARGO_WREM2,
    IR_CODEC_ARGO_WREM3,
    IR_CODEC_BLUESTAR_HEAVY,
    IR_CODEC_BOSCH144,
    IR_CODEC_BOSE,
    IR_CODEC_CARRIER_AC,
    IR_CODEC_CARRIER_AC40,
    IR_CODEC_CARRIER_AC64,
    IR_CODEC_CARRIER_AC128,
    IR_CODEC_CLIMA_BUTLER,
    IR_CODEC_COOLIX,
    IR_CODEC_COOLIX48,
    IR_CODEC_CORONA_AC,
    IR_CODEC_DAIKIN64,
    IR_CODEC_DAIKIN128,
    IR_CODEC_DAIKIN152,
    IR_CODEC_DAIKIN160,
    IR_CODEC_DAIKIN176,
    IR_CODEC_DAIKIN200,
    IR_CODEC_DAIKIN216,
    IR_CODEC_DELONGHI_AC,
    IR_CODEC_DENON,
    IR_CODEC_DISH,
    IR_CODEC_DOSHISHA,
    IR_CODEC_ECOCLIM,
    IR_CODEC_ELECTRA_AC,
    IR_CODEC_EPSON,
    IR_CODEC_EUROM,
    IR_CODEC_FUJITSU_AC,
    IR_CODEC_GICABLE,
    IR_CODEC_GORENJE,
    IR_CODEC_GREE,
    IR_CODEC_HAIER_AC,
    IR_CODEC_HAIER_AC_YRW02,
    IR_CODEC_HAIER_AC160,
    IR_CODEC_HAIER_AC176,
    IR_CODEC_HITACHI_AC1,
    IR_CODEC_HITACHI_AC3,
    IR_CODEC_INAX,
    IR_CODEC_JVC,
    IR_CODEC_KELON,
    IR_CODEC_KELVINATOR,
    IR_CODEC_LEGOPF,
    IR_CODEC_LG,
    IR_CODEC_LG2,
    IR_CODEC_METZ,
    IR_CODEC_MIDEA,
    IR_CODEC_MIDEA24,
    IR_CODEC_MILESTAG2,
    IR_CODEC_MIRAGE,
    IR_CODEC_MITSUBISHI,
    IR_CODEC_MITSUBISHI2,
    IR_CODEC_MITSUBISHI_AC,
    IR_CODEC_MITSUBISHI112,
    IR_CODEC_MITSUBISHI136,
    IR_CODEC_MITSUBISHI_HEAVY_88,
    IR_CODEC_MITSUBISHI_HEAVY_152,
    IR_CODEC_NEOCLIMA,
    IR_CODEC_NIKAI,
    IR_CODEC_PANASONIC,
    IR_CODEC_PANASONIC_AC,
    IR_CODEC_RHOSS,
    IR_CODEC_SAMSUNG36,
    IR_CODEC_SAMSUNG_AC,
    IR_CODEC_SANYO_LC7461,
    IR_CODEC_SANYO_AC,
    IR_CODEC_SANYO_AC88,
    IR_CODEC_SANYO_AC152,
    IR_CODEC_SHARP,
    IR_CODEC_SHARP_AC,
    IR_CODEC_SHERWOOD,
    IR_CODEC_SYMPHONY,
    IR_CODEC_TCL112,
    IR_CODEC_TECHNIBEL_AC,
    IR_CODEC_TECO,
    IR_CODEC_TEKNOPOINT,
    IR_CODEC_TOSHIBA_AC,
    IR_CODEC_TOTO,
    IR_CODEC_TRANSCOLD,
    IR_CODEC_TROTEC,
    IR_CODEC_TROTEC_3550,
    IR_CODEC_TRUMA,
    IR_CODEC_VESTEL_AC,
    IR_CODEC_VOLTAS,
    IR_CODEC_WHIRLPOOL_AC,
    IR_CODEC_WHYNTER,
    IR_CODEC_WOWWEE,
    IR_CODEC_XMP,
    IR_CODEC_YORK,
    IR_CODEC_ZEPEAL,
    IR_CODEC_LASERTAG,
    IR_CODEC_AIRWELL,
    IR_CODEC_ARRIS,
    IR_CODEC_CARRIER_AC84,
    IR_CODEC_DAIKIN,
    IR_CODEC_DAIKIN2,
    IR_CODEC_DAIKIN312,
    IR_CODEC_ELITESCREENS,
    IR_CODEC_GOODWEATHER,
    IR_CODEC_HITACHI_AC,
    IR_CODEC_HITACHI_AC2,
    IR_CODEC_HITACHI_AC264,
    IR_CODEC_HITACHI_AC296,
    IR_CODEC_HITACHI_AC344,
    IR_CODEC_HITACHI_AC424,
    IR_CODEC_KELON168,
    IR_CODEC_LUTRON,
    IR_CODEC_MAGIQUEST,
    IR_CODEC_MULTIBRACKETS,
    IR_CODEC_MWM,
    IR_CODEC_RCMM,
    IR_CODEC_SAMSUNG_AC_EXTENDED,
    IR_CODEC_TCL96_AC,
    IR_CODEC_TOTO_LONG,
    IR_CODEC__MAX
} ir_codec_id_t;

#define IR_DECODE_STATE_MAX 64

typedef struct {
    ir_codec_id_t id;
    uint16_t      bits;
    uint64_t      value;
    uint32_t      address;
    uint32_t      command;
    bool          repeat;
    uint8_t       state[IR_DECODE_STATE_MAX];
    uint16_t      state_nbytes;
} ir_decode_result_t;

typedef bool (*ir_codec_decode_fn_t)(const uint16_t *timings,
                                     size_t n_timings,
                                     ir_decode_result_t *out);

typedef struct {
    ir_codec_id_t        id;
    const char          *name;
    ir_codec_decode_fn_t decode;
} ir_codec_db_entry_t;

extern const ir_codec_db_entry_t codec_db_table[];
extern const size_t              codec_db_table_len;

bool codec_db_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out, const char **out_name);

const char *codec_db_name(ir_codec_id_t id);

#ifdef __cplusplus
}
#endif

#endif
