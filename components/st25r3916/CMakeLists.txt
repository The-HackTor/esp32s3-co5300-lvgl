idf_component_register(
    SRCS
        "src/platform.c"
        "src/st25r_spi.c"
        "src/st25r_trigger.c"
        "st25r3916/st25r3916.c"
        "st25r3916/st25r3916_com.c"
        "st25r3916/st25r3916_irq.c"
        "st25r3916/st25r3916_led.c"
        "st25r3916/st25r3916_aat.c"
        "st25r3916/timer.c"
    INCLUDE_DIRS
        "include"
        "st25r3916"
        "st25r_common"
    REQUIRES
        esp_driver_spi
        esp_driver_gpio
        esp_timer
        log
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=type-limits)
