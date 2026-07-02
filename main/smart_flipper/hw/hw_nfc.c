#include "hw_nfc.h"
#include "hw/hw_spi3.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r_spi.h"
#include "st25r_trigger.h"
#include "nfc_poller.h"
#include "mf_ultralight_listener.h"
#include "mf_classic_listener.h"
#include "lvgl.h"
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "hw_nfc";
static bool s_nfc_available;
static bool s_emulating;

#define NFC_TASK_STACK   8192
#define NFC_TASK_PRIO    6
#define NFC_READ_BUDGET_MS  5000
#define NFC_DETECT_SLICE_MS 300

typedef struct {
    struct mfc_dump *mfc_out;
    struct mfu_dump *mfu_out;
    hw_nfc_read_cb_t cb;
    void            *ctx;
} nfc_read_job_t;

struct nfc_cb_payload {
    hw_nfc_read_cb_t   cb;
    void              *ctx;
    bool               success;
    enum nfc_card_type type;
};

static QueueHandle_t      s_job_queue;
static TaskHandle_t       s_worker_task;
static atomic_int         s_read_cancel;
static struct mfu_ce_context s_mfu_ce;
static struct mfc_ce_context s_mfc_ce;

struct nonce_payload {
    hw_nfc_nonce_cb_t  cb;
    void              *ctx;
    struct mfkey_nonce nonce;
};

static void deliver_nonce_cb(void *p)
{
    struct nonce_payload *pl = p;
    if(pl->cb) pl->cb(&pl->nonce, pl->ctx);
    free(pl);
}

static struct {
    hw_nfc_nonce_cb_t cb;
    void             *ctx;
} s_nonce_target;

static void listener_nonce_cb(const struct mfkey_nonce *nonce, void *unused)
{
    (void)unused;
    if(!s_nonce_target.cb) return;
    struct nonce_payload *pl = malloc(sizeof(*pl));
    if(!pl) return;
    pl->cb    = s_nonce_target.cb;
    pl->ctx   = s_nonce_target.ctx;
    pl->nonce = *nonce;
    if(lv_async_call(deliver_nonce_cb, pl) != LV_RESULT_OK) free(pl);
}

#define NFC_SPI_CS_GPIO    GPIO_NUM_46
#define NFC_IRQ_GPIO       GPIO_NUM_18

#define NFC_SPI_CLOCK_HZ   1000000

static atomic_int s_attack_cancel;

static void deliver_read_cb(void *p)
{
    struct nfc_cb_payload *pl = p;
    if(pl->cb) pl->cb(pl->success, pl->type, pl->ctx);
    free(pl);
}

static void dispatch_cb(const nfc_read_job_t *job, bool success,
                        enum nfc_card_type type)
{
    if(!job->cb) return;
    struct nfc_cb_payload *pl = malloc(sizeof(*pl));
    if(!pl) return;
    pl->cb = job->cb; pl->ctx = job->ctx;
    pl->success = success; pl->type = type;
    if(lv_async_call(deliver_read_cb, pl) != LV_RESULT_OK) free(pl);
}

static void worker_run_read(const nfc_read_job_t *job)
{
    bool               success = false;
    enum nfc_card_type type    = NFC_CARD_NONE;

    nfc_poller_field_on();

    struct iso14443a_card card = {0};
    int ret = -1;
    const int64_t deadline_ms = (esp_timer_get_time() / 1000) + NFC_READ_BUDGET_MS;
    while((esp_timer_get_time() / 1000) < deadline_ms) {
        if(atomic_exchange(&s_read_cancel, 0)) goto done;
        ret = nfc_poller_detect(&card, NFC_DETECT_SLICE_MS);
        if(ret == 0) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if(ret != 0) goto done;

    const bool is_mfc = (card.sak == 0x08 || card.sak == 0x09 || card.sak == 0x18);
    const bool is_mfu = (card.sak == 0x00);

    if(is_mfu && job->mfu_out) {

        nfc_poller_halt();
        memset(job->mfu_out, 0, sizeof(*job->mfu_out));
        if(mfu_dump_card(job->mfu_out) == 0) {
            success = true;
            type    = NFC_CARD_MFU;
        }
    } else if(is_mfc && job->mfc_out) {

        nfc_poller_halt();
        memset(job->mfc_out, 0, sizeof(*job->mfc_out));
        if(mfc_dump_card(job->mfc_out) == 0) {
            success = true;
            type    = NFC_CARD_MFC;
        } else {

            job->mfc_out->card = card;
            job->mfc_out->type = mfc_detect_type(card.sak);
            success = true;
            type    = NFC_CARD_MFC;
        }
    } else {
        ESP_LOGW(TAG, "card detected (SAK=0x%02X) but no matching out buffer", card.sak);
    }

done:
    nfc_poller_field_off();
    dispatch_cb(job, success, type);
}

static void nfc_worker_task(void *arg)
{
    (void)arg;
    for(;;) {
        nfc_read_job_t job;
        if(xQueueReceive(s_job_queue, &job, portMAX_DELAY) != pdTRUE) continue;
        worker_run_read(&job);
    }
}

void hw_nfc_init(bool nfc_available)
{
    s_nfc_available = nfc_available;
    ESP_LOGI(TAG, "init (avail=%d)", nfc_available);

    if(hw_spi3_init() != ESP_OK) {
        ESP_LOGE(TAG, "SPI3 init failed; NFC offline");
        s_nfc_available = false;
        return;
    }
    if(st25r_spi_init(HW_SPI3_HOST, NFC_SPI_CS_GPIO, NFC_SPI_CLOCK_HZ) != ESP_OK) {
        ESP_LOGE(TAG, "st25r_spi_init failed; NFC offline");
        s_nfc_available = false;
        return;
    }
    if(st25r_trigger_init(NFC_IRQ_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "st25r_trigger_init failed; NFC offline");
        s_nfc_available = false;
        return;
    }

    bool ok = st25r3916Initialize();
    uint8_t ic_id = 0xFF;
    st25r3916ReadRegister(ST25R3916_REG_IC_IDENTITY, &ic_id);
    ESP_LOGI(TAG, "ST25R3916 init=%d IC_IDENTITY=0x%02X (type=0x%02X rev=%u)",
             (int)ok, ic_id,
             (ic_id & ST25R3916_REG_IC_IDENTITY_ic_type_mask),
             (unsigned)(ic_id & 0x07));

    if(!ok) { s_nfc_available = false; return; }

    nfc_poller_init_chip();

    s_job_queue = xQueueCreate(2, sizeof(nfc_read_job_t));
    if(!s_job_queue) {
        ESP_LOGE(TAG, "job queue alloc failed; NFC offline");
        s_nfc_available = false;
        return;
    }
    if(xTaskCreate(nfc_worker_task, "nfc_worker", NFC_TASK_STACK,
                   NULL, NFC_TASK_PRIO, &s_worker_task) != pdPASS) {
        ESP_LOGE(TAG, "worker task create failed; NFC offline");
        s_nfc_available = false;
        return;
    }

    s_nfc_available = true;
    ESP_LOGI(TAG, "NFC online (Phase 1: NFC-A poll + MFU dump)");
}

void hw_nfc_generate_fake_dump(struct mfc_dump *dump)
{
    if (!dump) return;
    memset(dump, 0, sizeof(*dump));
    static const uint8_t uid[] = {0xDE, 0xAD, 0xBE, 0xEF};
    memcpy(dump->card.uid, uid, sizeof(uid));
    dump->card.uid_len = sizeof(uid);
    dump->card.sak = ISO14443A_SAK_MFC_1K;
    dump->card.atqa[0] = 0x04;
    dump->card.atqa[1] = 0x00;
    dump->type = MFC_TYPE_1K;
    for (int b = 0; b < MFC_1K_BLOCKS; b++) {
        dump->block_read[b] = true;
        for (int i = 0; i < MFC_BLOCK_SIZE; i++) {
            dump->blocks[b][i] = (uint8_t)(b * 16 + i);
        }
    }
    dump->key_a_mask = (1ULL << MFC_1K_SECTORS) - 1;
    dump->key_b_mask = 0;
    for (int s = 0; s < MFC_1K_SECTORS; s++) {
        memset(dump->key_a[s], 0xFF, 6);
    }
    dump->read_time_ms = 450;
}

void hw_nfc_start_read(struct mfc_dump *mfc_out, struct mfu_dump *mfu_out,
                       hw_nfc_read_cb_t cb, void *ctx)
{
    if(!s_nfc_available || !s_job_queue) {
        ESP_LOGW(TAG, "start_read: NFC not online");
        if(cb) cb(false, NFC_CARD_NONE, ctx);
        return;
    }
    atomic_store(&s_read_cancel, 0);
    const nfc_read_job_t job = {
        .mfc_out = mfc_out,
        .mfu_out = mfu_out,
        .cb      = cb,
        .ctx     = ctx,
    };
    if(xQueueSend(s_job_queue, &job, 0) != pdTRUE) {
        ESP_LOGW(TAG, "start_read: queue full, dropping request");
        if(cb) cb(false, NFC_CARD_NONE, ctx);
    }
}

void hw_nfc_cancel_read(void)
{

    atomic_store(&s_read_cancel, 1);
}

void hw_nfc_start_emulate(struct mfc_dump *src, hw_nfc_nonce_cb_t cb, void *ctx)
{
    if(!s_nfc_available) {
        ESP_LOGW(TAG, "start_emulate: NFC not online");
        return;
    }
    if(s_emulating) {
        ESP_LOGW(TAG, "start_emulate: already emulating, stopping first");
        hw_nfc_stop_emulate();
    }
    s_nonce_target.cb  = cb;
    s_nonce_target.ctx = ctx;
    int ret = mfc_ce_start(&s_mfc_ce, src, listener_nonce_cb, NULL);
    if(ret != 0) {
        ESP_LOGE(TAG, "mfc_ce_start failed: %d", ret);
        s_nonce_target.cb = NULL;
        return;
    }
    s_emulating = true;
    ESP_LOGI(TAG, "MFC emulation started (%s, UID len=%u)",
             mfc_type_str(src->type), src->card.uid_len);
}

void hw_nfc_start_emulate_mfu(struct mfu_dump *src)
{
    if(!s_nfc_available) {
        ESP_LOGW(TAG, "start_emulate_mfu: NFC not online");
        return;
    }
    if(s_emulating) {
        ESP_LOGW(TAG, "start_emulate_mfu: already emulating, stopping first");
        hw_nfc_stop_emulate();
    }
    int ret = mfu_ce_start(&s_mfu_ce, src);
    if(ret != 0) {
        ESP_LOGE(TAG, "mfu_ce_start failed: %d", ret);
        return;
    }
    s_emulating = true;
    ESP_LOGI(TAG, "MFU emulation started (%s, UID-%u)",
             mfu_type_str(src->type), src->card.uid_len);
}

void hw_nfc_stop_emulate(void)
{
    if(mfu_ce_is_running(&s_mfu_ce)) {
        mfu_ce_stop(&s_mfu_ce);
        ESP_LOGI(TAG, "MFU emulation stopped");
    }
    if(mfc_ce_is_running(&s_mfc_ce)) {
        mfc_ce_stop(&s_mfc_ce);
        ESP_LOGI(TAG, "MFC emulation stopped (nonces=%u)",
                 (unsigned)mfc_ce_nonce_count(&s_mfc_ce));
    }
    s_nonce_target.cb  = NULL;
    s_nonce_target.ctx = NULL;
    s_emulating = false;
}

bool hw_nfc_is_emulating(void)
{
    return s_emulating;
}

#include "mf_classic_mfkey32.h"
#include "mf_classic_nested.h"
#include "mf_classic_hardnested.h"

#define ATTACK_TASK_STACK 8192
#define ATTACK_TASK_PRIO  6

struct mfkey32_job {
    struct mfkey32_nonce_pair *pairs;
    size_t                     count;
    hw_nfc_mfkey32_cb_t        cb;
    void                      *ctx;
};

struct mfkey32_result_payload {
    hw_nfc_mfkey32_cb_t    cb;
    void                  *ctx;
    struct mfkey32_result *results;
    size_t                 count;
};

static void deliver_mfkey32_cb(void *p)
{
    struct mfkey32_result_payload *pl = p;
    if(pl->cb) pl->cb(pl->results, pl->count, pl->ctx);
    free(pl->results);
    free(pl);
}

static void mfkey32_attack_task(void *arg)
{
    struct mfkey32_job *job = arg;

    struct mfkey32_result *results = calloc(job->count, sizeof(*results));
    if(results) {
        mfkey32_solve_batch(job->pairs, job->count, results, job->count);
    }

    struct mfkey32_result_payload *pl = malloc(sizeof(*pl));
    if(pl) {
        pl->cb      = job->cb;
        pl->ctx     = job->ctx;
        pl->results = results;
        pl->count   = job->count;
        if(lv_async_call(deliver_mfkey32_cb, pl) != LV_RESULT_OK) {
            free(results);
            free(pl);
        }
    } else {
        free(results);
    }

    free(job->pairs);
    free(job);
    vTaskDelete(NULL);
}

void hw_nfc_solve_mfkey32(const struct mfkey32_nonce_pair *pairs, size_t count,
                          hw_nfc_mfkey32_cb_t cb, void *ctx)
{
    if(count == 0 || !pairs) { if(cb) cb(NULL, 0, ctx); return; }

    struct mfkey32_job *job = malloc(sizeof(*job));
    if(!job) { if(cb) cb(NULL, 0, ctx); return; }
    job->pairs = malloc(count * sizeof(*pairs));
    if(!job->pairs) { free(job); if(cb) cb(NULL, 0, ctx); return; }
    memcpy(job->pairs, pairs, count * sizeof(*pairs));
    job->count = count;
    job->cb    = cb;
    job->ctx   = ctx;

    if(xTaskCreate(mfkey32_attack_task, "mfkey32", ATTACK_TASK_STACK,
                   job, ATTACK_TASK_PRIO, NULL) != pdPASS) {
        ESP_LOGE(TAG, "mfkey32 task create failed");
        free(job->pairs); free(job);
        if(cb) cb(NULL, 0, ctx);
    }
}

struct nested_job {
    struct nested_params params;
    hw_nfc_nested_cb_t   cb;
    void                *ctx;
};

struct nested_result_payload {
    hw_nfc_nested_cb_t          cb;
    void                       *ctx;
    bool                        success;
    struct nested_result        result;
};

struct hardnested_job {
    struct nested_params       params;
    hw_nfc_hardnested_cb_t     cb;
    void                      *ctx;
};

struct hardnested_result_payload {
    hw_nfc_hardnested_cb_t        cb;
    void                         *ctx;
    bool                          success;
    struct hardnested_result      result;
};

static void deliver_nested_cb(void *p)
{
    struct nested_result_payload *pl = p;
    if(pl->cb) pl->cb(pl->success, &pl->result, pl->ctx);
    free(pl);
}

static void deliver_hardnested_cb(void *p)
{
    struct hardnested_result_payload *pl = p;
    if(pl->cb) pl->cb(pl->success, &pl->result, pl->ctx);
    free(pl);
}

static void nested_attack_task(void *arg)
{
    struct nested_job *job = arg;
    struct nested_result result = {0};

    nfc_poller_field_on();
    int ret = nested_attack(&job->params, &result, NULL, NULL);
    nfc_poller_field_off();

    struct nested_result_payload *pl = malloc(sizeof(*pl));
    if(pl) {
        pl->cb      = job->cb;
        pl->ctx     = job->ctx;
        pl->success = (ret == 0 && result.found);
        pl->result  = result;
        if(lv_async_call(deliver_nested_cb, pl) != LV_RESULT_OK) free(pl);
    }
    free(job);
    vTaskDelete(NULL);
}

static void hardnested_attack_task(void *arg)
{
    struct hardnested_job *job = arg;
    struct hardnested_result result = {0};

    nfc_poller_field_on();
    int ret = hardnested_attack(&job->params, &result, NULL, NULL);
    nfc_poller_field_off();

    struct hardnested_result_payload *pl = malloc(sizeof(*pl));
    if(pl) {
        pl->cb      = job->cb;
        pl->ctx     = job->ctx;
        pl->success = (ret == 0 && result.found);
        pl->result  = result;
        if(lv_async_call(deliver_hardnested_cb, pl) != LV_RESULT_OK) free(pl);
    }
    free(job);
    vTaskDelete(NULL);
}

void hw_nfc_start_nested(const struct nested_params *params,
                         hw_nfc_nested_cb_t cb, void *ctx)
{
    if(!s_nfc_available || !params) { if(cb) cb(false, NULL, ctx); return; }
    if(s_emulating) {
        ESP_LOGW(TAG, "start_nested: emulating; stop emulate first");
        if(cb) cb(false, NULL, ctx);
        return;
    }
    atomic_store(&s_attack_cancel, 0);

    struct nested_job *job = malloc(sizeof(*job));
    if(!job) { if(cb) cb(false, NULL, ctx); return; }
    job->params = *params;
    job->cb     = cb;
    job->ctx    = ctx;

    if(xTaskCreate(nested_attack_task, "nested", ATTACK_TASK_STACK,
                   job, ATTACK_TASK_PRIO, NULL) != pdPASS) {
        ESP_LOGE(TAG, "nested task create failed");
        free(job);
        if(cb) cb(false, NULL, ctx);
    }
}

void hw_nfc_start_hardnested(const struct nested_params *params,
                             hw_nfc_hardnested_cb_t cb, void *ctx)
{
    if(!s_nfc_available || !params) { if(cb) cb(false, NULL, ctx); return; }
    if(s_emulating) {
        ESP_LOGW(TAG, "start_hardnested: emulating; stop emulate first");
        if(cb) cb(false, NULL, ctx);
        return;
    }
    atomic_store(&s_attack_cancel, 0);

    struct hardnested_job *job = malloc(sizeof(*job));
    if(!job) { if(cb) cb(false, NULL, ctx); return; }
    job->params = *params;
    job->cb     = cb;
    job->ctx    = ctx;

    if(xTaskCreate(hardnested_attack_task, "hardnested", ATTACK_TASK_STACK,
                   job, ATTACK_TASK_PRIO, NULL) != pdPASS) {
        ESP_LOGE(TAG, "hardnested task create failed");
        free(job);
        if(cb) cb(false, NULL, ctx);
    }
}
