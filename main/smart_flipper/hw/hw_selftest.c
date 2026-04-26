#include "hw_selftest.h"
#include "hw_ir.h"
#include "hw_rgb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TAG "hw_selftest"

#define SELFTEST_DURATION_US  (60ULL * 1000ULL * 1000ULL)

/* High-duty mark/space pattern at the 38 kHz carrier: 6 ms mark + 0.6 ms space
 * x 64 pairs = ~423 ms per call (~91% mark duty). Loop back-to-back; the IR
 * LEDs should glow continuously on a phone camera. */
static const uint16_t IR_BURST[128] = {
    [0]   = 6000, [1]   = 600,  [2]   = 6000, [3]   = 600,
    [4]   = 6000, [5]   = 600,  [6]   = 6000, [7]   = 600,
    [8]   = 6000, [9]   = 600,  [10]  = 6000, [11]  = 600,
    [12]  = 6000, [13]  = 600,  [14]  = 6000, [15]  = 600,
    [16]  = 6000, [17]  = 600,  [18]  = 6000, [19]  = 600,
    [20]  = 6000, [21]  = 600,  [22]  = 6000, [23]  = 600,
    [24]  = 6000, [25]  = 600,  [26]  = 6000, [27]  = 600,
    [28]  = 6000, [29]  = 600,  [30]  = 6000, [31]  = 600,
    [32]  = 6000, [33]  = 600,  [34]  = 6000, [35]  = 600,
    [36]  = 6000, [37]  = 600,  [38]  = 6000, [39]  = 600,
    [40]  = 6000, [41]  = 600,  [42]  = 6000, [43]  = 600,
    [44]  = 6000, [45]  = 600,  [46]  = 6000, [47]  = 600,
    [48]  = 6000, [49]  = 600,  [50]  = 6000, [51]  = 600,
    [52]  = 6000, [53]  = 600,  [54]  = 6000, [55]  = 600,
    [56]  = 6000, [57]  = 600,  [58]  = 6000, [59]  = 600,
    [60]  = 6000, [61]  = 600,  [62]  = 6000, [63]  = 600,
    [64]  = 6000, [65]  = 600,  [66]  = 6000, [67]  = 600,
    [68]  = 6000, [69]  = 600,  [70]  = 6000, [71]  = 600,
    [72]  = 6000, [73]  = 600,  [74]  = 6000, [75]  = 600,
    [76]  = 6000, [77]  = 600,  [78]  = 6000, [79]  = 600,
    [80]  = 6000, [81]  = 600,  [82]  = 6000, [83]  = 600,
    [84]  = 6000, [85]  = 600,  [86]  = 6000, [87]  = 600,
    [88]  = 6000, [89]  = 600,  [90]  = 6000, [91]  = 600,
    [92]  = 6000, [93]  = 600,  [94]  = 6000, [95]  = 600,
    [96]  = 6000, [97]  = 600,  [98]  = 6000, [99]  = 600,
    [100] = 6000, [101] = 600,  [102] = 6000, [103] = 600,
    [104] = 6000, [105] = 600,  [106] = 6000, [107] = 600,
    [108] = 6000, [109] = 600,  [110] = 6000, [111] = 600,
    [112] = 6000, [113] = 600,  [114] = 6000, [115] = 600,
    [116] = 6000, [117] = 600,  [118] = 6000, [119] = 600,
    [120] = 6000, [121] = 600,  [122] = 6000, [123] = 600,
    [124] = 6000, [125] = 600,  [126] = 6000, [127] = 600,
};

void hw_selftest_run(void)
{
    hw_rgb_off();
    ESP_LOGW(TAG, "BEGIN IR-only stress test (60s) -- RGB off");

    int64_t t_start = esp_timer_get_time();
    uint32_t bursts = 0;
    while((esp_timer_get_time() - t_start) < (int64_t)SELFTEST_DURATION_US) {
        esp_err_t err = hw_ir_send_raw(IR_BURST,
                                       sizeof(IR_BURST)/sizeof(IR_BURST[0]), 38000);
        if(err != ESP_OK) {
            ESP_LOGE(TAG, "IR TX err: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        bursts++;
    }

    ESP_LOGW(TAG, "END IR stress test -- %lu bursts fired", (unsigned long)bursts);
}
