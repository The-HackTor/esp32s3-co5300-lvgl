/*
 * UART Bridge Protocol
 *
 * Wire format for communication between STM32U5 (watch UI)
 * and ESP32-C5 (BLE + WiFi) over UART DMA.
 *
 * Shared by both firmware targets. No framework dependencies
 * (Zephyr or ESP-IDF) -- only stdint.h.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_BRIDGE_PROTOCOL_H_
#define APP_BRIDGE_PROTOCOL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BRIDGE_MAGIC        0xB5
#define BRIDGE_VERSION      1
#define BRIDGE_MAX_PAYLOAD  128
#define BRIDGE_BAUD_RATE    921600

/* Message types (STM32 <-> ESP32) */
enum bridge_msg_type {
	/* Time (ESP -> STM) */
	BRIDGE_MSG_TIME_SYNC      = 0x01,

	/* Notifications (ESP -> STM) */
	BRIDGE_MSG_NOTIFICATION   = 0x10,
	BRIDGE_MSG_NOTIF_CLEAR    = 0x11,

	/* Weather (ESP -> STM) */
	BRIDGE_MSG_WEATHER        = 0x20,
	BRIDGE_MSG_FORECAST       = 0x21,

	/* System (bidirectional) */
	BRIDGE_MSG_BATTERY        = 0x40,
	BRIDGE_MSG_BLE_STATE      = 0x41,
	BRIDGE_MSG_WIFI_STATE     = 0x42,
	BRIDGE_MSG_BRIGHTNESS     = 0x43,

	/* Control (STM -> ESP) */
	BRIDGE_MSG_FIND_PHONE     = 0x50,
	BRIDGE_MSG_MUSIC_CTRL     = 0x51,
	BRIDGE_MSG_CAMERA_CTRL    = 0x52,
	BRIDGE_MSG_MUSIC_INFO     = 0x53,

	/* Firmware update (ESP -> STM unless noted) */
	BRIDGE_MSG_FW_START       = 0x60,
	BRIDGE_MSG_FW_CHUNK       = 0x61,
	BRIDGE_MSG_FW_DONE        = 0x62,
	BRIDGE_MSG_FW_REBOOT      = 0x63,
	BRIDGE_MSG_FW_ACK         = 0x64, /* STM -> ESP */

	/* Version query (ESP -> STM, response STM -> ESP) */
	BRIDGE_MSG_VERSION_REQ    = 0x70,
	BRIDGE_MSG_VERSION_RSP    = 0x71,

	/* NFC library sync (bidirectional).
	 * Phone app -> U5: LIST (enumerate /lfs/nfc/library), GET (read),
	 *                  PUT (write), DELETE. U5 responds with ENT or OK.
	 * Also used by U5 to push trace chunks back to phone for offline
	 * analysis. See docs/nfc_port_plan.md Phase 13. */
	BRIDGE_MSG_NFC_LIST       = 0x80,
	BRIDGE_MSG_NFC_ENT        = 0x81,
	BRIDGE_MSG_NFC_GET        = 0x82,
	BRIDGE_MSG_NFC_CHUNK      = 0x83,
	BRIDGE_MSG_NFC_PUT        = 0x84,
	BRIDGE_MSG_NFC_DELETE     = 0x85,
	BRIDGE_MSG_NFC_OK         = 0x86,
	BRIDGE_MSG_NFC_ERR        = 0x87,
	BRIDGE_MSG_NFC_TRACE_CHUNK = 0x88,
};

#define BRIDGE_NFC_NAME_MAX    48
#define BRIDGE_NFC_CHUNK_MAX   96

enum bridge_nfc_kind {
	BRIDGE_NFC_KIND_CARD     = 0,
	BRIDGE_NFC_KIND_TRACE    = 1,
	BRIDGE_NFC_KIND_SIGNAL   = 2,
	BRIDGE_NFC_KIND_SCRIPT   = 3,
};

enum bridge_nfc_err {
	BRIDGE_NFC_ERR_OK         = 0,
	BRIDGE_NFC_ERR_NOT_FOUND  = 1,
	BRIDGE_NFC_ERR_IO         = 2,
	BRIDGE_NFC_ERR_NO_SPACE   = 3,
	BRIDGE_NFC_ERR_BAD_ARG    = 4,
	BRIDGE_NFC_ERR_BUSY       = 5,
};

struct __attribute__((packed)) bridge_nfc_list_req {
	uint8_t  kind;
};

struct __attribute__((packed)) bridge_nfc_ent {
	uint8_t  kind;
	uint32_t size;
	uint8_t  name_len;
};

struct __attribute__((packed)) bridge_nfc_get_req {
	uint8_t  kind;
	uint32_t offset;
	uint32_t length;
	uint8_t  name_len;
};

struct __attribute__((packed)) bridge_nfc_chunk {
	uint32_t offset;
	uint16_t length;
	uint8_t  last;
};

struct __attribute__((packed)) bridge_nfc_put_req {
	uint8_t  kind;
	uint32_t offset;
	uint16_t length;
	uint8_t  last;
	uint8_t  name_len;
};

struct __attribute__((packed)) bridge_nfc_delete_req {
	uint8_t  kind;
	uint8_t  name_len;
};

struct __attribute__((packed)) bridge_nfc_err_rsp {
	uint8_t  code;
};

#define FW_CHUNK_DATA_MAX  112  /* 7 * 16, aligned to STM32U5 quadword */

enum bridge_fw_status {
	FW_STATUS_OK        = 0,
	FW_STATUS_ERR_FLASH = 1,
	FW_STATUS_ERR_SEQ   = 2,
	FW_STATUS_ERR_SIZE  = 3,
	FW_STATUS_ERR_HASH  = 4,
	FW_STATUS_ERR_STATE = 5,
};

struct __attribute__((packed)) bridge_fw_start {
	uint32_t fw_size;
	uint8_t  sha256[32];
};

struct __attribute__((packed)) bridge_fw_chunk {
	uint32_t offset;
	uint8_t  data[];
};

struct __attribute__((packed)) bridge_fw_ack {
	uint8_t  status;
	uint32_t offset;
};

struct __attribute__((packed)) bridge_version_rsp {
	uint8_t  major;
	uint8_t  minor;
	uint16_t revision;
	uint32_t build_num;
};

/*
 * Frame format (UART wire):
 *
 *   [MAGIC:1][VERSION:1][TYPE:1][LEN:2 LE][PAYLOAD:0..128][CRC8:1]
 *
 * Total overhead: 6 bytes. Max frame: 134 bytes.
 * LEN is the payload length (0 for payloads with no data).
 * CRC8 covers TYPE + LEN + PAYLOAD (not MAGIC/VERSION).
 */

struct __attribute__((packed)) bridge_frame_header {
	uint8_t  magic;
	uint8_t  version;
	uint8_t  type;
	uint16_t payload_len;
};

#define BRIDGE_HEADER_SIZE  sizeof(struct bridge_frame_header)
#define BRIDGE_CRC_SIZE     1
#define BRIDGE_FRAME_OVERHEAD (BRIDGE_HEADER_SIZE + BRIDGE_CRC_SIZE)
#define BRIDGE_MAX_FRAME    (BRIDGE_FRAME_OVERHEAD + BRIDGE_MAX_PAYLOAD)

/* Time sync payload (ESP -> STM) */
struct __attribute__((packed)) bridge_time_sync {
	uint8_t  hour;       /* 0-23 */
	uint8_t  minute;     /* 0-59 */
	uint8_t  second;     /* 0-59 */
	uint8_t  weekday;    /* 0=Sun, 6=Sat */
	uint8_t  day;        /* 1-31 */
	uint8_t  month;      /* 1-12 */
	uint16_t year;       /* e.g. 2026 */
};

/* Notification payload (ESP -> STM) */
struct __attribute__((packed)) bridge_notification {
	uint8_t  icon_id;    /* app icon enum */
	uint8_t  title_len;
	uint8_t  body_len;
	/* followed by: title[title_len] + body[body_len] */
};

/* Weather payload (ESP -> STM) */
struct __attribute__((packed)) bridge_weather {
	int8_t   temp;           /* current temperature */
	uint8_t  icon;           /* weather icon id (0-7) */
	int8_t   temp_high;
	int8_t   temp_low;
	uint8_t  humidity;       /* 0-100 % */
	uint8_t  city_len;
	/* followed by: city[city_len] */
};

/* Forecast day (ESP -> STM), sent as array */
struct __attribute__((packed)) bridge_forecast_day {
	uint8_t  weekday;        /* 0=Sun */
	uint8_t  icon;
	int8_t   temp_high;
	int8_t   temp_low;
};

/* Battery status (bidirectional) */
struct __attribute__((packed)) bridge_battery {
	uint8_t  level;          /* 0-100 % */
	uint8_t  charging;       /* 0 or 1 */
};

/* BLE state (ESP -> STM) */
struct __attribute__((packed)) bridge_ble_state {
	uint8_t  state;          /* 0=off, 1=advertising, 2=connected */
};

/* WiFi state (ESP -> STM) */
struct __attribute__((packed)) bridge_wifi_state {
	uint8_t  state;          /* 0=off, 1=connecting, 2=connected */
	int8_t   rssi;
};

/* Music control (STM -> ESP) */
enum bridge_music_action {
	MUSIC_PLAY_PAUSE = 0,
	MUSIC_NEXT       = 1,
	MUSIC_PREV       = 2,
	MUSIC_VOL_UP     = 3,
	MUSIC_VOL_DOWN   = 4,
};

struct __attribute__((packed)) bridge_music_ctrl {
	uint8_t action;          /* enum bridge_music_action */
};

/* Music now-playing info (ESP -> STM) */
struct __attribute__((packed)) bridge_music_info {
	uint8_t playing;         /* 0=paused, 1=playing */
	uint8_t title_len;
	uint8_t artist_len;
	/* followed by: title[title_len] + artist[artist_len] */
};

/* Brightness (ESP -> STM) */
struct __attribute__((packed)) bridge_brightness {
	uint8_t level;           /* 10-255 */
};

/* CRC8 lookup table (polynomial 0x07, init 0x00) */
static const uint8_t bridge_crc8_lut[256] = {
	0x00,0x07,0x0e,0x09,0x1c,0x1b,0x12,0x15,0x38,0x3f,0x36,0x31,0x24,0x23,0x2a,0x2d,
	0x70,0x77,0x7e,0x79,0x6c,0x6b,0x62,0x65,0x48,0x4f,0x46,0x41,0x54,0x53,0x5a,0x5d,
	0xe0,0xe7,0xee,0xe9,0xfc,0xfb,0xf2,0xf5,0xd8,0xdf,0xd6,0xd1,0xc4,0xc3,0xca,0xcd,
	0x90,0x97,0x9e,0x99,0x8c,0x8b,0x82,0x85,0xa8,0xaf,0xa6,0xa1,0xb4,0xb3,0xba,0xbd,
	0xc7,0xc0,0xc9,0xce,0xdb,0xdc,0xd5,0xd2,0xff,0xf8,0xf1,0xf6,0xe3,0xe4,0xed,0xea,
	0xb7,0xb0,0xb9,0xbe,0xab,0xac,0xa5,0xa2,0x8f,0x88,0x81,0x86,0x93,0x94,0x9d,0x9a,
	0x27,0x20,0x29,0x2e,0x3b,0x3c,0x35,0x32,0x1f,0x18,0x11,0x16,0x03,0x04,0x0d,0x0a,
	0x57,0x50,0x59,0x5e,0x4b,0x4c,0x45,0x42,0x6f,0x68,0x61,0x66,0x73,0x74,0x7d,0x7a,
	0x89,0x8e,0x87,0x80,0x95,0x92,0x9b,0x9c,0xb1,0xb6,0xbf,0xb8,0xad,0xaa,0xa3,0xa4,
	0xf9,0xfe,0xf7,0xf0,0xe5,0xe2,0xeb,0xec,0xc1,0xc6,0xcf,0xc8,0xdd,0xda,0xd3,0xd4,
	0x69,0x6e,0x67,0x60,0x75,0x72,0x7b,0x7c,0x51,0x56,0x5f,0x58,0x4d,0x4a,0x43,0x44,
	0x19,0x1e,0x17,0x10,0x05,0x02,0x0b,0x0c,0x21,0x26,0x2f,0x28,0x3d,0x3a,0x33,0x34,
	0x4e,0x49,0x40,0x47,0x52,0x55,0x5c,0x5b,0x76,0x71,0x78,0x7f,0x6a,0x6d,0x64,0x63,
	0x3e,0x39,0x30,0x37,0x22,0x25,0x2c,0x2b,0x06,0x01,0x08,0x0f,0x1a,0x1d,0x14,0x13,
	0xae,0xa9,0xa0,0xa7,0xb2,0xb5,0xbc,0xbb,0x96,0x91,0x98,0x9f,0x8a,0x8d,0x84,0x83,
	0xde,0xd9,0xd0,0xd7,0xc2,0xc5,0xcc,0xcb,0xe6,0xe1,0xe8,0xef,0xfa,0xfd,0xf4,0xf3,
};

static inline uint8_t bridge_crc8(const uint8_t *data, uint16_t len)
{
	uint8_t crc = 0x00;

	for (uint16_t i = 0; i < len; i++) {
		crc = bridge_crc8_lut[crc ^ data[i]];
	}
	return crc;
}

#ifdef __cplusplus
}
#endif

#endif /* APP_BRIDGE_PROTOCOL_H_ */
