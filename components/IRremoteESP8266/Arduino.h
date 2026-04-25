/*
 * Minimal Arduino.h shim for the IRremoteESP8266 library on ESP-IDF.
 *
 * IRremoteESP8266 is LGPL-2.1; this shim is the boundary between it and
 * smart_flipper. We do NOT use the library's IRrecv (interrupt-driven over
 * legacy ESP RMT) or IRsend (raw GPIO toggling) classes -- our hw_ir.c
 * drives RMT v6.1 directly. The codecs themselves (decodeXxx / sendXxx
 * helpers operating on a uint16_t timing buffer) compile against this shim
 * but never reach the GPIO/interrupt paths at runtime, so the pinMode/
 * digitalWrite/attachInterrupt declarations below are no-op stubs.
 */

#ifndef ARDUINO_H_SHIM_
#define ARDUINO_H_SHIM_

#ifdef __cplusplus
#include <string>
#include <cstdint>
#include <cstddef>
#include <cmath>      /* double_t referenced by IRsend::calibrate */
#else
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#endif

/* --- Arduino pin / IO modes -------------------------------------------- */
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif
#ifndef INPUT
#define INPUT        0x0
#define OUTPUT       0x1
#define INPUT_PULLUP 0x2
#endif
#ifndef RISING
#define RISING  1
#define FALLING 2
#define CHANGE  3
#endif

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef F
#define F(x) (x)         /* No flash-string magic; just identity. */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* --- Time + delay ------------------------------------------------------ */
void     delay(uint32_t ms);
void     delayMicroseconds(uint32_t us);
uint32_t millis(void);
uint32_t micros(void);
void     yield(void);

/* --- GPIO no-op stubs (RMT path bypasses these) ----------------------- */
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);
void attachInterrupt(uint8_t pin, void (*isr)(void), int mode);
void detachInterrupt(uint8_t pin);

/* Compat no-ops; we don't gate critical sections via Arduino API. */
static inline void interrupts(void)   {}
static inline void noInterrupts(void) {}

#ifdef __cplusplus
}  /* extern "C" */

/* --- Arduino String compatibility ------------------------------------- */
/* IRremoteESP8266 uses Arduino's String almost exclusively for `+=`,
 * `c_str()`, `length()`, `reserve()` -- all of which std::string supports.
 * Adding `substring()` (used in one site -- IRutils.cpp) keeps the
 * library source unmodified. */
class String : public std::string {
 public:
    using std::string::string;
    String() = default;
    String(const std::string& s) : std::string(s) {}
    String(const char* s) : std::string(s ? s : "") {}

    /* Arduino's substring(start[, end]) takes a *past-the-end* index, not a
     * length. std::string::substr takes (pos, count). Translate. */
    String substring(size_t start) const {
        if(start >= size()) return String();
        return String(substr(start));
    }
    String substring(size_t start, size_t end) const {
        if(start >= size() || end <= start) return String();
        if(end > size()) end = size();
        return String(substr(start, end - start));
    }
};

#endif  /* __cplusplus */

#endif  /* ARDUINO_H_SHIM_ */
