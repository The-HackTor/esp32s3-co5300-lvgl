file(GLOB_RECURSE SMART_FLIPPER_SRCS
    "smart_flipper/*.c"
)

idf_component_register(
    SRCS "example_qspi_with_ram.c" ${SMART_FLIPPER_SRCS}
    INCLUDE_DIRS
        "."
        "smart_flipper"
        "smart_flipper/include"
        "smart_flipper/app"
        "smart_flipper/ui"
        "smart_flipper/ui/modules"
        "smart_flipper/ui/widgets"
        "smart_flipper/ui/screens"
        "smart_flipper/apps"
        "smart_flipper/apps/nfc"
        "smart_flipper/apps/subghz"
        "smart_flipper/apps/ir"
        "smart_flipper/hw"
        "smart_flipper/store"
        "smart_flipper/bridge"
        "smart_flipper/lib/infrared"
        "smart_flipper/lib/infrared/encoder_decoder"
        "smart_flipper/lib/infrared/encoder_decoder/common"
        "smart_flipper/lib/infrared/encoder_decoder/nec"
        "smart_flipper/lib/infrared/encoder_decoder/rc5"
        "smart_flipper/lib/infrared/encoder_decoder/rc6"
        "smart_flipper/lib/infrared/encoder_decoder/samsung"
        "smart_flipper/lib/infrared/encoder_decoder/sirc"
        "smart_flipper/lib/infrared/encoder_decoder/kaseikyo"
        "smart_flipper/lib/infrared/encoder_decoder/pioneer"
        "smart_flipper/lib/infrared/encoder_decoder/rca"
        "smart_flipper/lib/infrared/codec_db"
)

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)
