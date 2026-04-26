#include "hw_selftest.h"
#include "hw_ir.h"
#include "hw_rgb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "hw_selftest"

#define SELFTEST_DURATION_MS  60000
#define STEP_PERIOD_MS        200

/* Long, dense burst: 64 mark/space pairs at ~1ms each => ~128ms of carrier
 * activity per call. Camera sees a steady purplish glow; the IR detector on
 * any nearby remote will see this as random noise (intentional). */
static const uint16_t IR_BURST[128] = {
    [0 ... 127] = 1000,
};

void hw_selftest_run(void)
{
    ESP_LOGW(TAG, "BEGIN aggressive HW test (%d ms) -- RGB + IR TX", SELFTEST_DURATION_MS);

    uint32_t elapsed = 0;
    uint8_t  phase   = 0;
    while(elapsed < SELFTEST_DURATION_MS) {
        switch(phase % 3) {
        case 0: hw_rgb_set(255,   0,   0); break;
        case 1: hw_rgb_set(  0, 255,   0); break;
        case 2: hw_rgb_set(  0,   0, 255); break;
        }
        phase++;

        esp_err_t err = hw_ir_send_raw(IR_BURST, sizeof(IR_BURST)/sizeof(IR_BURST[0]), 38000);
        if(err != ESP_OK) {
            ESP_LOGE(TAG, "IR TX err: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(STEP_PERIOD_MS));
        elapsed += STEP_PERIOD_MS;
    }

    hw_rgb_off();
    ESP_LOGW(TAG, "END HW test -- resuming normal operation");
}
