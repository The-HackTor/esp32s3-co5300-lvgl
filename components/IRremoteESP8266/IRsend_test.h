/*
 * Minimal IRsend_test.h shim for IRremoteESP8266 on ESP-IDF.
 *
 * Upstream IRsend_test.h is a Google-test mock that subclasses IRsend and
 * captures every mark/space into iostream-backed buffers. We define UNIT_TEST
 * to gate out the GPIO-bitbang / hw_timer paths in IRrecv.cpp / IRsend.cpp /
 * IRtimer.cpp, but we do NOT want the iostream test plumbing pulled in --
 * the per-protocol AC headers under UNIT_TEST inherit from IRsendTest
 * purely so they can be `IRsend`-like without locking to a real GPIO at
 * construction time.
 *
 * This stub provides the minimum interface needed for the codec headers to
 * compile and link: an IRsendTest class that forwards to IRsend, no iostream.
 *
 * Note: ESP-IDF flag is `UNIT_TEST=1`; see components/IRremoteESP8266/CMakeLists.txt.
 */

#ifndef IRREMOTEESP8266_IRSEND_TEST_SHIM_H_
#define IRREMOTEESP8266_IRSEND_TEST_SHIM_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include "IRsend.h"
#include "IRrecv.h"   /* for decode_results */

class IRsendTest : public IRsend {
 public:
    /* Upstream's IRsendTest also exposes `capture` (a decode_results)
     * referenced by IRac.cpp's OUTPUT_DECODE_RESULTS_FOR_UT macro. We don't
     * actually drive that test path -- _utReceiver is always null at
     * runtime -- but the field has to exist for compilation. */
    decode_results capture{};

    explicit IRsendTest(uint16_t pin, bool inverted = false, bool use_modulation = true)
        : IRsend(pin, inverted, use_modulation) {}

    /* No-op stubs so codec/test-shim macro expansions link. */
    void reset() {}
    void makeDecodeResult(uint16_t offset = 0) { (void)offset; }
};

#endif  /* IRREMOTEESP8266_IRSEND_TEST_SHIM_H_ */
