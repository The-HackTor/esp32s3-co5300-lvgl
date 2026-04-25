#ifndef HW_SUBGHZ_H
#define HW_SUBGHZ_H

#include "hw_types.h"
#include <stdbool.h>

typedef void (*hw_subghz_analyzer_cb_t)(const SimSubghzFreqResult *result, void *ctx);
typedef void (*hw_subghz_capture_cb_t)(bool success, const SimSubghzRaw *raw, void *ctx);
typedef void (*hw_subghz_decode_cb_t)(const SimSubghzDecoded *decoded, void *ctx);
typedef void (*hw_subghz_replay_cb_t)(bool success, void *ctx);
typedef void (*hw_subghz_ping_cb_t)(bool success, void *ctx);
typedef void (*hw_subghz_relay_cb_t)(bool success, int progress, void *ctx);
typedef void (*hw_subghz_read_cb_t)(const SubghzHistoryEntry *entry,
                                     uint16_t total_count, void *ctx);

void hw_subghz_init(const struct device *radio, bool radio_available);

void hw_subghz_start_analyzer(hw_subghz_analyzer_cb_t cb, void *ctx);
void hw_subghz_stop_analyzer(void);

void hw_subghz_start_capture(hw_subghz_capture_cb_t cb, void *ctx);
void hw_subghz_start_capture_ex(uint32_t duration_ms, float rssi_threshold,
                                 hw_subghz_capture_cb_t cb, void *ctx);
void hw_subghz_start_capture_continuous(float rssi_threshold,
                                        hw_subghz_capture_cb_t cb, void *ctx);
void hw_subghz_stop_capture(void);
bool hw_subghz_is_capturing(void);
float hw_subghz_get_live_rssi(void);
uint16_t hw_subghz_capture_sample_count(void);
const SimSubghzDecoded *hw_subghz_capture_decoded(void);

void hw_subghz_start_replay(const SimSubghzRaw *raw, hw_subghz_replay_cb_t cb, void *ctx);
void hw_subghz_stop_replay(void);
void hw_subghz_start_encode_tx(const char *protocol, uint64_t data,
                                uint8_t bits, uint32_t te,
                                uint8_t repeat_count,
                                hw_subghz_replay_cb_t cb, void *ctx);

void hw_subghz_start_decode(hw_subghz_decode_cb_t cb, void *ctx);
void hw_subghz_stop_decode(void);

void hw_subghz_start_read(hw_subghz_read_cb_t cb, void *ctx);
void hw_subghz_start_read_hopping(hw_subghz_read_cb_t cb, void *ctx);
void hw_subghz_stop_read(void);
bool hw_subghz_is_hopping(void);
uint32_t hw_subghz_hop_current_freq(void);
bool hw_subghz_hop_is_locked(void);

void hw_subghz_ping(hw_subghz_ping_cb_t cb, void *ctx);
void hw_subghz_send_dump(hw_subghz_relay_cb_t cb, void *ctx);
void hw_subghz_recv_dump(hw_subghz_relay_cb_t cb, void *ctx);
void hw_subghz_stop_relay(void);

uint32_t hw_subghz_get_frequency(void);
void hw_subghz_set_frequency(uint32_t freq_khz);
uint8_t hw_subghz_get_preset(void);
void hw_subghz_set_preset(uint8_t preset);

struct mfc_dump *hw_subghz_get_relay_dump(void);

const SubghzHistoryEntry *hw_subghz_get_read_history(uint16_t *count);

#define sim_subghz_analyzer_cb_t  hw_subghz_analyzer_cb_t
#define sim_subghz_capture_cb_t   hw_subghz_capture_cb_t
#define sim_subghz_decode_cb_t    hw_subghz_decode_cb_t
#define sim_subghz_replay_cb_t    hw_subghz_replay_cb_t
#define sim_subghz_ping_cb_t      hw_subghz_ping_cb_t
#define sim_subghz_relay_cb_t     hw_subghz_relay_cb_t

#define sim_subghz_start_analyzer hw_subghz_start_analyzer
#define sim_subghz_stop_analyzer  hw_subghz_stop_analyzer
#define sim_subghz_start_capture              hw_subghz_start_capture
#define sim_subghz_start_capture_ex           hw_subghz_start_capture_ex
#define sim_subghz_start_capture_continuous   hw_subghz_start_capture_continuous
#define sim_subghz_stop_capture               hw_subghz_stop_capture
#define sim_subghz_start_replay   hw_subghz_start_replay
#define sim_subghz_start_decode   hw_subghz_start_decode
#define sim_subghz_stop_decode    hw_subghz_stop_decode
#define sim_subghz_start_read          hw_subghz_start_read
#define sim_subghz_start_read_hopping  hw_subghz_start_read_hopping
#define sim_subghz_stop_read           hw_subghz_stop_read
#define sim_subghz_ping           hw_subghz_ping
#define sim_subghz_send_dump      hw_subghz_send_dump
#define sim_subghz_recv_dump      hw_subghz_recv_dump
#define sim_subghz_stop_relay     hw_subghz_stop_relay

#endif
