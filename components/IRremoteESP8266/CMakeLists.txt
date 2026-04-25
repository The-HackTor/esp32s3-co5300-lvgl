# IRremoteESP8266 (LGPL-2.1) vendored at v2.9.0 (936f5e00).
# Source tree under src/ is verbatim upstream; Arduino.h + arduino_shim.cpp
# in the component root provide the minimal Arduino-compat boundary so the
# C++ codecs link against ESP-IDF instead of the Arduino runtime.
#
# This component intentionally exposes its full src/ as INCLUDE_DIRS plus
# the component root (so user code does `#include "ir_NEC.h"` etc., and the
# library's `#include <Arduino.h>` resolves to our shim).

file(GLOB IRREMOTE_SRCS "src/*.cpp")

idf_component_register(
    SRCS ${IRREMOTE_SRCS} "arduino_shim.cpp"
    INCLUDE_DIRS "." "src"
    REQUIRES esp_timer
    PRIV_REQUIRES freertos esp_rom
)

# UNIT_TEST is the library's "compiling outside Arduino" gate -- with it
# defined, IRrecv.cpp/IRsend.cpp/IRtimer.cpp skip the interrupt-driven RX
# path, GPIO-bitbang TX path, ESP.restart(), and Arduino hw_timer_t glue.
# We never invoke those at runtime: hw_ir.c drives RMT directly. Per-
# protocol decodeXxx/sendXxx functions live in ir_*.cpp and are unaffected.
target_compile_definitions(${COMPONENT_LIB} PRIVATE UNIT_TEST=1)

# Library uses placement-new style, NaN comparisons, and intentional
# unused-parameter patterns in many AC codecs. Suppress the noisier warnings
# without hiding real defects -- LGPL boundary so we don't modify sources.
set_source_files_properties(
    ${IRREMOTE_SRCS}
    PROPERTIES COMPILE_OPTIONS
    "-Wno-unused-parameter;-Wno-missing-field-initializers;-Wno-unused-variable;-Wno-unused-but-set-variable"
)
