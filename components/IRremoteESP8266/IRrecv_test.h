/*
 * Minimal IRrecv_test.h shim for IRremoteESP8266 on ESP-IDF. See sibling
 * IRsend_test.h for rationale.
 */

#ifndef IRREMOTEESP8266_IRRECV_TEST_SHIM_H_
#define IRREMOTEESP8266_IRRECV_TEST_SHIM_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include "IRrecv.h"

class IRrecvTest : public IRrecv {
 public:
    explicit IRrecvTest(uint16_t recvpin, uint16_t bufsize = kRawBuf,
                        uint8_t timeout = kTimeoutMs, bool save_buffer = false)
        : IRrecv(recvpin, bufsize, timeout, save_buffer) {}
};

#endif  /* IRREMOTEESP8266_IRRECV_TEST_SHIM_H_ */
