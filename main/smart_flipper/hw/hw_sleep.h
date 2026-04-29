#ifndef HW_SLEEP_H
#define HW_SLEEP_H

#include <stdbool.h>
#include <stdint.h>
#include <lvgl.h>
#include "esp_timer.h"

/*
 * Sleep policy engine -- wraps the ESP32-S3 light-sleep entry point in
 * a high-level state machine that owns the wake sources, suspends the
 * LVGL tick during sleep, and respects per-scene inhibits.
 *
 * Tier ordering (configurable via hw_sleep_set_thresholds):
 *   t < blank      : panel FULL or DIM (handled by app_main idle_dim)
 *   blank <= t     : panel BLANK (handled by app_main idle_dim)
 *   light_sleep <= t AND !inhibited AND !charging : enter the
 *     light-sleep poll loop here. CPU is clock-gated; we wake every
 *     50 ms to poll the FT3168 touch controller (no hardware INT line
 *     wired on this board), or immediately on IR_RX (GPIO17) /
 *     RTC_INT (IO15) / IMU_INT (IO8) falling edge.
 *
 * Inhibits:
 *   The IR Learn / Brute / Macro scenes call hw_sleep_inhibit(true) on
 *   enter and (false) on exit. Light-sleep would clock-gate the RMT
 *   peripheral and silently break the active capture or burst.
 */

void hw_sleep_init(lv_display_t *disp, esp_timer_handle_t lvgl_tick_timer);

/* Threshold to enter the light-sleep poll loop, in milliseconds of
 * LVGL touch inactivity. The display-blank tier in app_main fires
 * earlier; this tier kicks in after the panel is already off.
 * Default 5 minutes. Pass 0 to disable light-sleep entirely. */
void hw_sleep_set_threshold(uint32_t threshold_ms);

/* Per-scene inhibit. Refcounted internally so nested holders compose:
 * Learn enters, calls inhibit(true); Macro within Learn calls (true)
 * again; both must call (false) before the engine resumes sleeping.
 * Safe to call from any task. */
void hw_sleep_inhibit(bool inhibit);

/* True while the engine has issued an esp_light_sleep_start since the
 * last wake. Used by the status bar to render a "Z" glyph and by the
 * sleep-aware IR worker (kept paused while we're asleep). */
bool hw_sleep_is_napping(void);

/* Threshold to escalate to deep-sleep after the device has been in the
 * light-sleep loop continuously for this many milliseconds.
 * Default 30 minutes. Pass 0 to disable deep-sleep entirely. */
void hw_sleep_set_deep_threshold(uint32_t threshold_ms);

/* Persist a small string into RTC slow memory; survives deep sleep but
 * not power-off. Use this to capture which remote was loaded so the
 * post-wake boot path can re-open it without making the user navigate
 * back. NULL or empty payload clears the slot. Max ~96 bytes. */
void hw_sleep_set_resume_payload(const char *path);

/* Returns the payload last set before deep-sleep, only when the
 * current boot is a deep-sleep wake AND the magic is intact. NULL on
 * cold boot or when no payload was set. */
const char *hw_sleep_get_resume_payload(void);

/* True if the current boot was caused by a deep-sleep wake. Read once
 * at app start to drive resume logic. */
bool hw_sleep_woke_from_deep(void);

#endif
