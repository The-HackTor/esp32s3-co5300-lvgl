#include "codec_db.h"

#include "airton.h"
#include "airwell.h"
#include "aiwa.h"
#include "amcor.h"
#include "argo.h"
#include "arris.h"
#include "bluestar_heavy.h"
#include "bosch.h"
#include "bose.h"
#include "carrier.h"
#include "clima_butler.h"
#include "coolix.h"
#include "corona.h"
#include "daikin.h"
#include "delonghi.h"
#include "denon.h"
#include "dish.h"
#include "doshisha.h"
#include "ecoclim.h"
#include "electra.h"
#include "elitescreens.h"
#include "epson.h"
#include "eurom.h"
#include "fujitsu.h"
#include "gicable.h"
#include "goodweather.h"
#include "gorenje.h"
#include "gree.h"
#include "haier.h"
#include "hitachi.h"
#include "inax.h"
#include "jvc.h"
#include "kelon.h"
#include "kelvinator.h"
#include "lasertag.h"
#include "lego.h"
#include "lg.h"
#include "lutron.h"
#include "magiquest.h"
#include "metz.h"
#include "midea.h"
#include "milestag2.h"
#include "mirage.h"
#include "mitsubishi.h"
#include "mitsubishi_heavy.h"
#include "multibrackets.h"
#include "mwm.h"
#include "neoclima.h"
#include "nikai.h"
#include "panasonic.h"
#include "rcmm.h"
#include "rhoss.h"
#include "samsung.h"
#include "sanyo.h"
#include "sharp.h"
#include "sherwood.h"
#include "symphony.h"
#include "tcl.h"
#include "technibel.h"
#include "teco.h"
#include "teknopoint.h"
#include "toshiba.h"
#include "toto.h"
#include "transcold.h"
#include "trotec.h"
#include "truma.h"
#include "vestel.h"
#include "voltas.h"
#include "whirlpool.h"
#include "whynter.h"
#include "wowwee.h"
#include "xmp.h"
#include "york.h"
#include "zepeal.h"

const ir_codec_db_entry_t codec_db_table[] = {
    { IR_CODEC_AIRTON, "Airton", ir_airton_decode },
    { IR_CODEC_AIWA_RC_T501, "Aiwa RC-T501", ir_aiwa_decode },
    { IR_CODEC_AMCOR, "Amcor", ir_amcor_decode },
    { IR_CODEC_ARGO_WREM2, "Argo WREM-2", ir_argo_wrem2_decode },
    { IR_CODEC_ARGO_WREM3, "Argo WREM-3", ir_argo_wrem3_decode },
    { IR_CODEC_BLUESTAR_HEAVY, "Bluestar Heavy", ir_bluestar_heavy_decode },
    { IR_CODEC_BOSCH144, "Bosch 144", ir_bosch144_decode },
    { IR_CODEC_BOSE, "Bose", ir_bose_decode },
    { IR_CODEC_CARRIER_AC, "Carrier AC", ir_carrier_ac_decode },
    { IR_CODEC_CARRIER_AC40, "Carrier AC40", ir_carrier_ac40_decode },
    { IR_CODEC_CARRIER_AC64, "Carrier AC64", ir_carrier_ac64_decode },
    { IR_CODEC_CARRIER_AC128, "Carrier AC128", ir_carrier_ac128_decode },
    { IR_CODEC_CLIMA_BUTLER, "Clima-Butler", ir_clima_butler_decode },
    { IR_CODEC_COOLIX, "Coolix", ir_coolix_decode },
    { IR_CODEC_COOLIX48, "Coolix48", ir_coolix48_decode },
    { IR_CODEC_CORONA_AC, "Corona AC", ir_corona_ac_decode },
    { IR_CODEC_DAIKIN64, "Daikin 64", ir_daikin64_decode },
    { IR_CODEC_DAIKIN128, "Daikin 128", ir_daikin128_decode },
    { IR_CODEC_DAIKIN152, "Daikin 152", ir_daikin152_decode },
    { IR_CODEC_DAIKIN160, "Daikin 160", ir_daikin160_decode },
    { IR_CODEC_DAIKIN176, "Daikin 176", ir_daikin176_decode },
    { IR_CODEC_DAIKIN200, "Daikin 200", ir_daikin200_decode },
    { IR_CODEC_DAIKIN216, "Daikin 216", ir_daikin216_decode },
    { IR_CODEC_DELONGHI_AC, "Delonghi AC", ir_delonghi_decode },
    { IR_CODEC_DENON, "Denon", ir_denon_decode },
    { IR_CODEC_DISH, "Dish", ir_dish_decode },
    { IR_CODEC_DOSHISHA, "Doshisha", ir_doshisha_decode },
    { IR_CODEC_ECOCLIM, "EcoClim", ir_ecoclim_decode },
    { IR_CODEC_ELECTRA_AC, "Electra AC", ir_electra_decode },
    { IR_CODEC_EPSON, "Epson", ir_epson_decode },
    { IR_CODEC_EUROM, "Eurom", ir_eurom_decode },
    { IR_CODEC_FUJITSU_AC, "Fujitsu AC", ir_fujitsu_ac_decode },
    { IR_CODEC_GICABLE, "G.I. Cable", ir_gicable_decode },
    { IR_CODEC_GORENJE, "Gorenje", ir_gorenje_decode },
    { IR_CODEC_GREE, "Gree", ir_gree_decode },
    { IR_CODEC_HAIER_AC, "Haier AC", ir_haier_ac_decode },
    { IR_CODEC_HAIER_AC_YRW02, "Haier YR-W02", ir_haier_ac_yrw02_decode },
    { IR_CODEC_HAIER_AC160, "Haier AC160", ir_haier_ac160_decode },
    { IR_CODEC_HAIER_AC176, "Haier AC176", ir_haier_ac176_decode },
    { IR_CODEC_HITACHI_AC1, "Hitachi AC1", ir_hitachi_ac1_decode },
    { IR_CODEC_HITACHI_AC3, "Hitachi AC3", ir_hitachi_ac3_decode },
    { IR_CODEC_INAX, "Inax", ir_inax_decode },
    { IR_CODEC_JVC, "JVC", ir_jvc_decode },
    { IR_CODEC_KELON, "Kelon", ir_kelon_decode },
    { IR_CODEC_KELVINATOR, "Kelvinator", ir_kelvinator_decode },
    { IR_CODEC_LEGOPF, "LEGO PF", ir_lego_decode },
    { IR_CODEC_LG, "LG", ir_lg_decode },
    { IR_CODEC_LG2, "LG2", ir_lg2_decode },
    { IR_CODEC_METZ, "Metz", ir_metz_decode },
    { IR_CODEC_MIDEA, "Midea", ir_midea_decode },
    { IR_CODEC_MIDEA24, "Midea 24", ir_midea24_decode },
    { IR_CODEC_MILESTAG2, "MilesTag2", ir_milestag2_decode },
    { IR_CODEC_MIRAGE, "Mirage", ir_mirage_decode },
    { IR_CODEC_MITSUBISHI, "Mitsubishi", ir_mitsubishi_decode },
    { IR_CODEC_MITSUBISHI2, "Mitsubishi2", ir_mitsubishi2_decode },
    { IR_CODEC_MITSUBISHI_AC, "Mitsubishi AC", ir_mitsubishi_ac_decode },
    { IR_CODEC_MITSUBISHI112, "Mitsubishi112", ir_mitsubishi112_decode },
    { IR_CODEC_MITSUBISHI136, "Mitsubishi136", ir_mitsubishi136_decode },
    { IR_CODEC_MITSUBISHI_HEAVY_88, "Mitsubishi Heavy 88", ir_mitsubishi_heavy_88_decode },
    { IR_CODEC_MITSUBISHI_HEAVY_152, "Mitsubishi Heavy 152", ir_mitsubishi_heavy_152_decode },
    { IR_CODEC_NEOCLIMA, "Neoclima", ir_neoclima_decode },
    { IR_CODEC_NIKAI, "Nikai", ir_nikai_decode },
    { IR_CODEC_PANASONIC, "Panasonic", ir_panasonic_decode },
    { IR_CODEC_PANASONIC_AC, "Panasonic AC", ir_panasonic_ac_decode },
    { IR_CODEC_RHOSS, "Rhoss", ir_rhoss_decode },
    { IR_CODEC_SAMSUNG36, "Samsung36", ir_samsung36_decode },
    { IR_CODEC_SAMSUNG_AC, "Samsung AC", ir_samsung_ac_decode },
    { IR_CODEC_SANYO_LC7461, "Sanyo LC7461", ir_sanyo_lc7461_decode },
    { IR_CODEC_SANYO_AC, "Sanyo AC", ir_sanyo_ac_decode },
    { IR_CODEC_SANYO_AC88, "Sanyo AC88", ir_sanyo_ac88_decode },
    { IR_CODEC_SANYO_AC152, "Sanyo AC152", ir_sanyo_ac152_decode },
    { IR_CODEC_SHARP, "Sharp", ir_sharp_decode },
    { IR_CODEC_SHARP_AC, "Sharp AC", ir_sharp_ac_decode },
    { IR_CODEC_SHERWOOD, "Sherwood", ir_sherwood_decode },
    { IR_CODEC_SYMPHONY, "Symphony", ir_symphony_decode },
    { IR_CODEC_TCL112, "TCL 112", ir_tcl112_decode },
    { IR_CODEC_TECHNIBEL_AC, "Technibel AC", ir_technibel_decode },
    { IR_CODEC_TECO, "Teco", ir_teco_decode },
    { IR_CODEC_TEKNOPOINT, "Teknopoint", ir_teknopoint_decode },
    { IR_CODEC_TOSHIBA_AC, "Toshiba AC", ir_toshiba_ac_decode },
    { IR_CODEC_TOTO, "Toto", ir_toto_decode },
    { IR_CODEC_TRANSCOLD, "Transcold", ir_transcold_decode },
    { IR_CODEC_TROTEC, "Trotec", ir_trotec_decode },
    { IR_CODEC_TROTEC_3550, "Trotec 3550", ir_trotec3550_decode },
    { IR_CODEC_TRUMA, "Truma", ir_truma_decode },
    { IR_CODEC_VESTEL_AC, "Vestel AC", ir_vestel_ac_decode },
    { IR_CODEC_VOLTAS, "Voltas", ir_voltas_decode },
    { IR_CODEC_WHIRLPOOL_AC, "Whirlpool AC", ir_whirlpool_ac_decode },
    { IR_CODEC_WHYNTER, "Whynter", ir_whynter_decode },
    { IR_CODEC_WOWWEE, "WowWee", ir_wowwee_decode },
    { IR_CODEC_XMP, "XMP", ir_xmp_decode },
    { IR_CODEC_YORK, "York", ir_york_decode },
    { IR_CODEC_ZEPEAL, "Zepeal", ir_zepeal_decode },
    { IR_CODEC_LASERTAG, "Lasertag", ir_lasertag_decode },
    { IR_CODEC_AIRWELL, "Airwell", ir_airwell_decode },
    { IR_CODEC_ARRIS, "Arris", ir_arris_decode },
    { IR_CODEC_CARRIER_AC84, "Carrier AC84", ir_carrier_ac84_decode },
    { IR_CODEC_DAIKIN, "Daikin", ir_daikin_decode },
    { IR_CODEC_DAIKIN2, "Daikin2", ir_daikin2_decode },
    { IR_CODEC_DAIKIN312, "Daikin 312", ir_daikin312_decode },
    { IR_CODEC_ELITESCREENS, "EliteScreens", ir_elitescreens_decode },
    { IR_CODEC_GOODWEATHER, "Goodweather", ir_goodweather_decode },
    { IR_CODEC_HITACHI_AC, "Hitachi AC", ir_hitachi_ac_decode },
    { IR_CODEC_HITACHI_AC2, "Hitachi AC2", ir_hitachi_ac2_decode },
    { IR_CODEC_HITACHI_AC264, "Hitachi AC264", ir_hitachi_ac264_decode },
    { IR_CODEC_HITACHI_AC296, "Hitachi AC296", ir_hitachi_ac296_decode },
    { IR_CODEC_HITACHI_AC344, "Hitachi AC344", ir_hitachi_ac344_decode },
    { IR_CODEC_HITACHI_AC424, "Hitachi AC424", ir_hitachi_ac424_decode },
    { IR_CODEC_KELON168, "Kelon 168", ir_kelon168_decode },
    { IR_CODEC_LUTRON, "Lutron", ir_lutron_decode },
    { IR_CODEC_MAGIQUEST, "MagiQuest", ir_magiquest_decode },
    { IR_CODEC_MULTIBRACKETS, "Multibrackets", ir_multibrackets_decode },
    { IR_CODEC_MWM, "MWM", ir_mwm_decode },
    { IR_CODEC_RCMM, "RC-MM", ir_rcmm_decode },
    { IR_CODEC_SAMSUNG_AC_EXTENDED, "Samsung AC Extended", ir_samsung_ac256_decode },
    { IR_CODEC_TCL96_AC, "TCL96 AC", ir_tcl96_ac_decode },
    { IR_CODEC_TOTO_LONG, "Toto Long", ir_toto_long_decode },
};

const size_t codec_db_table_len = sizeof(codec_db_table) / sizeof(codec_db_table[0]);

bool codec_db_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out, const char **out_name)
{
    if(!timings || !out) return false;

    for(size_t i = 0; i < codec_db_table_len; i++) {
        const ir_codec_db_entry_t *e = &codec_db_table[i];
        if(e->decode(timings, n_timings, out)) {
            if(out_name) *out_name = e->name;
            return true;
        }
    }
    return false;
}

const char *codec_db_name(ir_codec_id_t id)
{
    for(size_t i = 0; i < codec_db_table_len; i++) {
        if(codec_db_table[i].id == id) return codec_db_table[i].name;
    }
    return "Unknown";
}
