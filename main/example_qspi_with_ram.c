#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "lvgl.h"
#include "esp_lcd_sh8601.h"
#include "touch_bsp.h"
#include "read_lcd_id_bsp.h"
#include "smart_flipper/smart_flipper.h"
#include "smart_flipper/hw/hw_rgb.h"
#include "smart_flipper/hw/hw_ir.h"
#include "smart_flipper/hw/hw_selftest.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"

#define SD_PIN_CS    GPIO_NUM_38
#define SD_PIN_MOSI  GPIO_NUM_39
#define SD_PIN_MISO  GPIO_NUM_40
#define SD_PIN_SCLK  GPIO_NUM_41
#define SD_HOST      SPI3_HOST
#define SD_MOUNT     "/sdcard"

static const char *TAG = "example";
static SemaphoreHandle_t lvgl_mux = NULL;
static lv_display_t *lvgl_disp = NULL;
static esp_lcd_panel_io_handle_t s_panel_io = NULL;

/* 0x51 = SH8601 write_display_brightness (1 byte, 0x00..0xFF). Ramped per
 * IDLE_TICK_MS to mask AMOLED step changes. */
#define IDLE_DIM_THRESHOLD_MS  30000
#define IDLE_DIM_LEVEL         0x1A
#define IDLE_FULL_LEVEL        0xFF
#define IDLE_RAMP_STEP         8
#define IDLE_TICK_MS           50

static void idle_dim_set_level(uint8_t level)
{
    if (!s_panel_io) return;
    uint8_t p = level;
    esp_lcd_panel_io_tx_param(s_panel_io, 0x51, &p, 1);
}

static void idle_dim_tick(lv_timer_t *t)
{
    (void)t;
    static uint8_t current = IDLE_FULL_LEVEL;
    if (!lvgl_disp) return;
    uint32_t inactive = lv_display_get_inactive_time(lvgl_disp);
    uint8_t target = (inactive >= IDLE_DIM_THRESHOLD_MS) ? IDLE_DIM_LEVEL : IDLE_FULL_LEVEL;
    if (current == target) return;
    if (target > current) {
        uint16_t next = (uint16_t)current + IDLE_RAMP_STEP;
        current = (next >= target) ? target : (uint8_t)next;
    } else {
        int16_t next = (int16_t)current - IDLE_RAMP_STEP;
        current = (next <= target) ? target : (uint8_t)next;
    }
    idle_dim_set_level(current);
}

#define LCD_HOST    SPI2_HOST

#define SH8601_ID 0x86
#define CO5300_ID 0xff
static uint8_t READ_LCD_ID = 0x00;

#define LCD_BIT_PER_PIXEL       (16)

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_9)
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_10)
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1         (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2         (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3         (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT          (-1)

#define EXAMPLE_LCD_H_RES              466
#define EXAMPLE_LCD_V_RES              466

#define EXAMPLE_LCD_BYTES_PER_PIXEL    (2)
#define EXAMPLE_LVGL_BUF_HEIGHT        (EXAMPLE_LCD_V_RES / 10)
#define EXAMPLE_LVGL_BUF_BYTES         (EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * EXAMPLE_LCD_BYTES_PER_PIXEL)
#define EXAMPLE_LVGL_PSRAM_POOL_BYTES  (4u * 1024u * 1024u)

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (8 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

static const sh8601_lcd_init_cmd_t sh8601_lcd_init_cmds[] =
{
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x44, (uint8_t []){0x01, 0xD1}, 2, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 10},
    {0x51, (uint8_t []){0x00}, 1, 10},
    {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};
static const sh8601_lcd_init_cmd_t co5300_lcd_init_cmds[] =
{
    {0x11, (uint8_t []){0x00}, 0, 80},
    {0xC4, (uint8_t []){0x80}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 1},
    {0x63, (uint8_t []){0xFF}, 1, 1},
    {0x51, (uint8_t []){0x00}, 1, 1},
    {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                            esp_lcd_panel_io_event_data_t *edata,
                                            void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    const int offsetx1 = (READ_LCD_ID == SH8601_ID) ? area->x1 : area->x1 + 0x06;
    const int offsetx2 = (READ_LCD_ID == SH8601_ID) ? area->x2 : area->x2 + 0x06;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

/* SH8601/CO5300 require even CASET/PASET windows for 16bpp data; odd-aligned
 * invalidations (e.g. a colon glyph) get silently re-aligned by the panel and
 * each row drifts. v9 equivalent of v8's disp_drv.rounder_cb. */
static void example_lvgl_invalidate_area_cb(lv_event_t *e)
{
    lv_area_t *area = lv_event_get_invalidated_area(e);
    area->x1 = area->x1 & ~1;
    area->y1 = area->y1 & ~1;
    area->x2 = area->x2 | 1;
    area->y2 = area->y2 | 1;
}

static void example_lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t tp_x;
    uint16_t tp_y;
    uint8_t win = getTouch(&tp_x, &tp_y);
    if (win) {
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static bool example_lvgl_lock(int timeout_ms)
{
    assert(lvgl_mux);
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void example_lvgl_unlock(void)
{
    assert(lvgl_mux);
    xSemaphoreGive(lvgl_mux);
}

/* Mount microSD via SDSPI on SPI3_HOST. SPI2 is owned by the QSPI AMOLED at
 * 40 MHz; sharing would hurt LCD flush latency. Failure is non-fatal so the
 * UI still boots without a card -- IR app will show "Insert SD" on save. */
static void example_mount_sdcard(void)
{
    const spi_bus_config_t bus = {
        .mosi_io_num     = SD_PIN_MOSI,
        .miso_io_num     = SD_PIN_MISO,
        .sclk_io_num     = SD_PIN_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t err = spi_bus_initialize(SD_HOST, &bus, SDSPI_DEFAULT_DMA);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "SD: spi_bus_initialize failed: %s", esp_err_to_name(err));
        return;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot          = SD_HOST;
    /* 20 MHz: 40 MHz is the IDF ceiling but HS-mode negotiation failed at
     * that rate on this board+card. SDSPI throughput is irrelevant for
     * KB-scale .ir files; correctness over headline speed. */
    host.max_freq_khz  = 20000;

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = SD_PIN_CS;
    slot.host_id = SD_HOST;

    const esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files              = 8,
        .allocation_unit_size   = 16 * 1024,
    };

    sdmmc_card_t *card = NULL;
    err = esp_vfs_fat_sdspi_mount(SD_MOUNT, &host, &slot, &mount, &card);
    if(err != ESP_OK) {
        ESP_LOGW(TAG, "SD: mount %s failed: %s (continuing without SD)",
                 SD_MOUNT, esp_err_to_name(err));
        spi_bus_free(SD_HOST);
        return;
    }
    ESP_LOGI(TAG, "SD: mounted at %s", SD_MOUNT);
    sdmmc_card_print_info(stdout, card);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        if (example_lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            example_lvgl_unlock();
        }
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

void app_main(void)
{
    READ_LCD_ID = read_lcd_id();

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_PCLK,
                                                                  EXAMPLE_PIN_NUM_LCD_DATA0,
                                                                  EXAMPLE_PIN_NUM_LCD_DATA1,
                                                                  EXAMPLE_PIN_NUM_LCD_DATA2,
                                                                  EXAMPLE_PIN_NUM_LCD_DATA3,
                                                                  EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * LCD_BIT_PER_PIXEL / 8);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_CS,
                                                                                 NULL,
                                                                                 NULL);
    sh8601_vendor_config_t vendor_config = {
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    s_panel_io = io_handle;

    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    vendor_config.init_cmds = (READ_LCD_ID == SH8601_ID) ? sh8601_lcd_init_cmds : co5300_lcd_init_cmds;
    vendor_config.init_cmds_size = (READ_LCD_ID == SH8601_ID)
        ? sizeof(sh8601_lcd_init_cmds) / sizeof(sh8601_lcd_init_cmds[0])
        : sizeof(co5300_lcd_init_cmds) / sizeof(co5300_lcd_init_cmds[0]);
    ESP_LOGI(TAG, "Install SH8601 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    idle_dim_set_level(IDLE_FULL_LEVEL);

    Touch_Init();

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
#endif

    /* RGB activity LEDs (R=GPIO17, G=GPIO7, B=GPIO0). LEDC PWM with 15% duty
     * cap; safe before LVGL. Boot-time channel sweep so a flash quickly proves
     * all three colors light independently. */
    hw_rgb_init();
    hw_rgb_pulse(255,   0,   0, 300); vTaskDelay(pdMS_TO_TICKS(350));
    hw_rgb_pulse(  0, 255,   0, 300); vTaskDelay(pdMS_TO_TICKS(350));
    hw_rgb_pulse(  0,   0, 255, 300); vTaskDelay(pdMS_TO_TICKS(350));
    hw_rgb_off();

    /* microSD on SPI3 -- mounted before LVGL so any module can use POSIX FS. */
    example_mount_sdcard();

    /* TEMP: GPIO discovery probe -- run BEFORE hw_ir_init so it can grab
     * candidate pins via LEDC without contending with RMT. Remove this
     * line + the hw_selftest.h include when IR_TX is identified. */
    hw_selftest_run();

    hw_ir_init();

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    /* PSRAM pool for widget trees / transient compositor layers. add_pool
     * silently returns NULL if size > LV_MEM_SIZE + LV_MEM_POOL_EXPAND_SIZE
     * (sdkconfig: LV_MEM_POOL_EXPAND_SIZE_KILOBYTES=4096). */
    void *psram_pool = heap_caps_malloc(EXAMPLE_LVGL_PSRAM_POOL_BYTES, MALLOC_CAP_SPIRAM);
    assert(psram_pool);
    if (lv_mem_add_pool(psram_pool, EXAMPLE_LVGL_PSRAM_POOL_BYTES) == NULL) {
        ESP_LOGE(TAG, "lv_mem_add_pool returned NULL — check CONFIG_LV_MEM_POOL_EXPAND_SIZE_KILOBYTES");
        abort();
    }

    /* Internal SRAM (DMA-capable) draw buffers. PSRAM-sourced QSPI DMA at
     * 40 MHz underflows under CPU I-fetch contention. */
    void *buf1 = heap_caps_malloc(EXAMPLE_LVGL_BUF_BYTES, MALLOC_CAP_DMA);
    assert(buf1);
    void *buf2 = heap_caps_malloc(EXAMPLE_LVGL_BUF_BYTES, MALLOC_CAP_DMA);
    assert(buf2);

    lvgl_disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_display_set_color_format(lvgl_disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_flush_cb(lvgl_disp, example_lvgl_flush_cb);
    lv_display_set_buffers(lvgl_disp, buf1, buf2, EXAMPLE_LVGL_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(lvgl_disp, panel_handle);
    lv_display_add_event_cb(lvgl_disp, example_lvgl_invalidate_area_cb, LV_EVENT_INVALIDATE_AREA, NULL);

    esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, lvgl_disp);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, example_lvgl_touch_cb);
    lv_indev_set_display(indev, lvgl_disp);

    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Start smart_flipper UI");
    if (example_lvgl_lock(-1)) {
        smart_flipper_start();
        lv_timer_create(idle_dim_tick, IDLE_TICK_MS, NULL);
        example_lvgl_unlock();
    }
}
