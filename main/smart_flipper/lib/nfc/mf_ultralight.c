#include <mf_ultralight.h>
#include "nfc_trx.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include <errno.h>
#include <string.h>

static const char *TAG = "mfu";

#define MFU_TRX_TIMEOUT_MS  50

static inline int64_t uptime_ms(void) { return esp_timer_get_time() / 1000; }

const uint8_t mfu_default_3des_key[16] = {
    0x49, 0x45, 0x4D, 0x4B, 0x41, 0x45, 0x52, 0x42,
    0x21, 0x4E, 0x41, 0x43, 0x55, 0x4F, 0x59, 0x46,
};

const uint8_t mfu_default_pwd[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

enum mfu_type mfu_detect_type(const uint8_t *ver)
{
    if(ver[0] != 0x00 || ver[1] != 0x04) return MFU_TYPE_UNKNOWN;

    uint8_t type = ver[2];
    uint8_t subtype = ver[3];
    uint8_t storage = ver[6];

    if(type == 0x03 && subtype == 0x01) {
        if(storage == 0x0B) return MFU_TYPE_UL_EV1_11;
        if(storage == 0x0E) return MFU_TYPE_UL_EV1_21;
    }
    if(type == 0x03 && subtype == 0x02) return MFU_TYPE_UL_NANO;

    if(type == 0x04 && (subtype == 0x01 || subtype == 0x02)) {
        if(storage == 0x0B) return MFU_TYPE_NTAG210;
        if(storage == 0x0E) return MFU_TYPE_NTAG212;
        if(storage == 0x0F) {
            return (subtype == 0x02) ? MFU_TYPE_NTAG213F : MFU_TYPE_NTAG213;
        }
        if(storage == 0x11) return MFU_TYPE_NTAG215;
        if(storage == 0x13) {
            return (subtype == 0x02) ? MFU_TYPE_NTAG216F : MFU_TYPE_NTAG216;
        }
    }

    if(type == 0x04 && subtype == 0x04) {
        if(storage == 0x0F) return MFU_TYPE_NTAG213F;
        if(storage == 0x13) return MFU_TYPE_NTAG216F;
    }

    if(type == 0x04 && subtype == 0x05) {
        if(storage == 0x13) return MFU_TYPE_NTAG_I2C_1K;
        if(storage == 0x15) return MFU_TYPE_NTAG_I2C_2K;
        if(storage == 0x1B) return MFU_TYPE_NTAG_I2C_PLUS_1K;
        if(storage == 0x1F) return MFU_TYPE_NTAG_I2C_PLUS_2K;
    }

    return MFU_TYPE_UNKNOWN;
}

int mfu_get_version(uint8_t *version)
{
    uint8_t cmd = MFU_CMD_GET_VERSION;
    uint16_t rx_len = 0;
    int ret = nfc_trx(&cmd, 1, version, 8, &rx_len, MFU_TRX_TIMEOUT_MS, true, true);
    return (ret < 0 || rx_len < 8) ? -EIO : 0;
}

int mfu_read_pages(uint8_t start, uint8_t *data, uint16_t *rx_len)
{
    uint8_t cmd[2] = { MFU_CMD_READ, start };
    int ret = nfc_trx(cmd, 2, data, 16, rx_len, MFU_TRX_TIMEOUT_MS, true, true);
    return (ret < 0 || *rx_len < 16) ? -EIO : 0;
}

int mfu_fast_read(uint8_t start, uint8_t end, uint8_t *data, uint16_t *rx_len)
{
    uint8_t cmd[3] = { MFU_CMD_FAST_READ, start, end };
    uint16_t expected = (uint16_t)(end - start + 1) * MFU_PAGE_SIZE;
    int ret = nfc_trx(cmd, 3, data, expected, rx_len, MFU_TRX_TIMEOUT_MS, true, true);
    return (ret < 0 || *rx_len < expected) ? -EIO : 0;
}

int mfu_write_page(uint8_t page, const uint8_t *data)
{
    uint8_t cmd[6] = { MFU_CMD_WRITE, page, data[0], data[1], data[2], data[3] };
    uint8_t rx[4];
    uint16_t rx_len = 0;
    int ret = nfc_trx(cmd, 6, rx, sizeof(rx), &rx_len, MFU_TRX_TIMEOUT_MS, true, true);
    return (ret < 0) ? -EIO : 0;
}

int mfu_read_sig(uint8_t *sig)
{
    uint8_t cmd[2] = { MFU_CMD_READ_SIG, 0x00 };
    uint16_t rx_len = 0;
    int ret = nfc_trx(cmd, 2, sig, 32, &rx_len, MFU_TRX_TIMEOUT_MS, true, true);
    return (ret < 0 || rx_len < 32) ? -EIO : 0;
}

int mfu_read_cnt(uint8_t index, uint8_t *out_3)
{
    uint8_t cmd[2] = { MFU_CMD_READ_CNT, index };
    uint8_t rx[3];
    uint16_t rx_len = 0;
    int ret = nfc_trx(cmd, 2, rx, sizeof(rx), &rx_len, MFU_TRX_TIMEOUT_MS, true, true);
    if(ret < 0 || rx_len < 3) return -EIO;
    memcpy(out_3, rx, 3);
    return 0;
}

int mfu_pwd_auth(const uint8_t pwd[4], uint8_t pack_out[2])
{
    uint8_t cmd[5] = { MFU_CMD_PWD_AUTH, pwd[0], pwd[1], pwd[2], pwd[3] };
    uint8_t rx[4];
    uint16_t rx_len = 0;
    int ret = nfc_trx(cmd, 5, rx, sizeof(rx), &rx_len, MFU_TRX_TIMEOUT_MS, true, true);
    if(ret < 0 || rx_len < 2) return -EACCES;
    pack_out[0] = rx[0]; pack_out[1] = rx[1];
    return 0;
}

int mfu_auth_3des(const uint8_t key[16])
{
    (void)key;
    ESP_LOGW(TAG, "mfu_auth_3des: not implemented (Phase 1 stub)");
    return -ENOTSUP;
}

int mfu_sector_select(uint8_t sector)
{
    uint8_t cmd1[2] = { MFU_CMD_SECTOR_SEL, 0xFF };
    uint8_t rx[4];
    uint16_t rx_len = 0;
    int ret = nfc_trx(cmd1, sizeof(cmd1), rx, sizeof(rx), &rx_len,
                      MFU_TRX_TIMEOUT_MS, true, true);
    if(ret < 0) return ret;
    if(rx_len < 1 || (rx[0] & 0x0F) != 0x0A) return -EIO;

    uint8_t cmd2[4] = { sector, 0x00, 0x00, 0x00 };
    rx_len = 0;
    ret = nfc_trx(cmd2, sizeof(cmd2), rx, sizeof(rx), &rx_len,
                  MFU_TRX_TIMEOUT_MS, true, true);
    if(ret == -ETIMEDOUT) return 0;
    return ret;
}

int mfu_dump_card(struct mfu_dump *dump)
{
    int ret = nfc_detect_card(&dump->card);
    if(ret) return ret;

    int64_t t_start = uptime_ms();
    memset(dump->page_read, 0, sizeof(dump->page_read));
    dump->version_valid = false;
    dump->signature_valid = false;
    dump->type = MFU_TYPE_UNKNOWN;

    if(mfu_get_version(dump->version) == 0) {
        dump->version_valid = true;
        dump->type = mfu_detect_type(dump->version);
    } else {
        (void)nfc_reactivate(&dump->card);
    }

    if(dump->type == MFU_TYPE_UNKNOWN) {
        uint8_t probe0[16];
        uint8_t probe16[16];
        uint16_t plen0 = 0, plen16 = 0;
        bool beyond16 = false;

        int r0 = mfu_read_pages(0, probe0, &plen0);
        if(r0 < 0) {
            (void)nfc_reactivate(&dump->card);
            r0 = mfu_read_pages(0, probe0, &plen0);
        }

        if(r0 == 0 && plen0 >= 16) {
            int r16 = mfu_read_pages(16, probe16, &plen16);
            if(r16 == 0 && plen16 >= 16) {
                if(memcmp(probe0, probe16, 16) != 0) beyond16 = true;
            } else {
                (void)nfc_reactivate(&dump->card);
            }
        }

        if(beyond16) {
            uint8_t auth_tx[2] = { MFU_CMD_AUTH_3DES, 0x00 };
            uint8_t auth_rx[16];
            uint16_t auth_rx_len = 0;
            int auth_ret = nfc_trx(auth_tx, 2, auth_rx, sizeof(auth_rx),
                                   &auth_rx_len, 50, true, true);
            if(auth_ret == 0 && auth_rx_len >= 9 && auth_rx[0] == MFU_CMD_AF) {
                dump->type = MFU_TYPE_UL_C;
            } else {
                dump->type = MFU_TYPE_NTAG203;
            }
            (void)nfc_reactivate(&dump->card);
        } else {
            dump->type = MFU_TYPE_UL;
        }
    }

    dump->total_pages = mfu_type_pages(dump->type);
    ESP_LOGI(TAG, "%s: %u pages", mfu_type_str(dump->type), dump->total_pages);

    dump->auth_kind = MFU_AUTH_NONE;
    dump->auth_ok = false;
    memset(dump->auth_key, 0, sizeof(dump->auth_key));
    memset(dump->pack, 0, sizeof(dump->pack));

    if(dump->type == MFU_TYPE_UL_C) {
        ret = mfu_auth_3des(mfu_default_3des_key);
        dump->auth_kind = MFU_AUTH_3DES_ULC;
        if(ret == 0) {
            dump->auth_ok = true;
            memcpy(dump->auth_key, mfu_default_3des_key, 16);
        } else {
            (void)nfc_reactivate(&dump->card);
        }
    }

#define MFU_FAST_READ_CHUNK 60
    uint8_t buf[MFU_FAST_READ_CHUNK * MFU_PAGE_SIZE];
    uint16_t rx_len = 0;

    bool fast_ok = false;
    if(dump->version_valid && dump->total_pages > 4) {
        fast_ok = true;
        for(uint16_t p = 0; p < dump->total_pages; p += MFU_FAST_READ_CHUNK) {
            uint16_t end = p + MFU_FAST_READ_CHUNK - 1;
            if(end >= dump->total_pages) end = dump->total_pages - 1;
            uint16_t want = (end - p + 1) * MFU_PAGE_SIZE;
            rx_len = 0;
            ret = mfu_fast_read((uint8_t)p, (uint8_t)end, buf, &rx_len);
            if(ret != 0 || rx_len < want) {
                (void)nfc_reactivate(&dump->card);
                fast_ok = false;
                break;
            }
            for(uint16_t i = 0; i + p <= end; i++) {
                memcpy(dump->pages[p + i], &buf[i * MFU_PAGE_SIZE], MFU_PAGE_SIZE);
                dump->page_read[p + i] = true;
            }
        }
    }

    if(!fast_ok) {
        bool pwd_tried = false;
        for(uint16_t p = 0; p < dump->total_pages; p += 4) {
            uint8_t page_buf[16];
            rx_len = 0;
            ret = mfu_read_pages(p, page_buf, &rx_len);

            if(ret < 0 && dump->version_valid && !pwd_tried &&
               dump->type != MFU_TYPE_UL_C) {
                pwd_tried = true;
                (void)nfc_reactivate(&dump->card);
                uint8_t pack[2];
                if(mfu_pwd_auth(mfu_default_pwd, pack) == 0) {
                    dump->auth_kind = MFU_AUTH_PWD_NTAG;
                    dump->auth_ok = true;
                    memcpy(dump->auth_key, mfu_default_pwd, 4);
                    memcpy(dump->pack, pack, 2);
                    rx_len = 0;
                    ret = mfu_read_pages(p, page_buf, &rx_len);
                } else {
                    (void)nfc_reactivate(&dump->card);
                }
            }

            if(ret < 0) {
                (void)nfc_reactivate(&dump->card);
                if(dump->auth_ok && dump->auth_kind == MFU_AUTH_PWD_NTAG) {
                    uint8_t pack[2];
                    (void)mfu_pwd_auth(mfu_default_pwd, pack);
                }
                continue;
            }

            uint16_t pages_got = rx_len / MFU_PAGE_SIZE;
            for(uint16_t i = 0; i < pages_got && (p + i) < dump->total_pages; i++) {
                memcpy(dump->pages[p + i], &page_buf[i * MFU_PAGE_SIZE], MFU_PAGE_SIZE);
                dump->page_read[p + i] = true;
            }
        }
    }

    if(dump->version_valid) {
        if(mfu_read_sig(dump->signature) == 0) {
            dump->signature_valid = true;
        } else {
            (void)nfc_reactivate(&dump->card);
        }
    }

    dump->read_time_ms = (uint32_t)(uptime_ms() - t_start);

    uint16_t read_count = 0;
    for(uint16_t p = 0; p < dump->total_pages; p++) {
        if(dump->page_read[p]) read_count++;
    }
    ESP_LOGI(TAG, "dump complete: %u/%u pages in %u ms",
             read_count, dump->total_pages, (unsigned)dump->read_time_ms);

    (void)esp_fill_random;
    return 0;
}
