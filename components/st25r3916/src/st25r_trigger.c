#include "st25r_trigger.h"
#include "platform.h"
#include "st25r3916_irq.h"
#include "esp_log.h"

static const char *TAG = "st25r_trigger";

#define IRQ_TASK_STACK   4096
#define IRQ_TASK_PRIO    18

static SemaphoreHandle_t       s_irq_sem;
static struct st25r_kern_sem   s_wait_sem;
static gpio_num_t              s_irq_pin = -1;
static TaskHandle_t            s_irq_task;
static bool                    s_isr_service_installed;

struct st25r_kern_sem *st25r_irq_wait_sem(void)
{
    return &s_wait_sem;
}

static void IRAM_ATTR irq_gpio_isr(void *arg)
{
    (void)arg;
    BaseType_t hpw = pdFALSE;
    xSemaphoreGiveFromISR(s_irq_sem, &hpw);
    portYIELD_FROM_ISR(hpw);
}

static void irq_task(void *arg)
{
    (void)arg;
    for(;;) {
        if(xSemaphoreTake(s_irq_sem, portMAX_DELAY) != pdTRUE) continue;
        st25r3916Isr();
        if(s_wait_sem.handle) {
            xSemaphoreGive(s_wait_sem.handle);
        }
    }
}

void st25r_trigger_disable(void)
{
    if(s_irq_pin >= 0) {
        gpio_intr_disable(s_irq_pin);
    }
}

void st25r_trigger_enable(void)
{
    if(s_irq_pin >= 0) {
        gpio_intr_enable(s_irq_pin);
    }
}

esp_err_t st25r_trigger_init(gpio_num_t irq_gpio)
{
    if(s_irq_task) return ESP_OK;

    s_irq_pin = irq_gpio;

    s_irq_sem        = xSemaphoreCreateBinary();
    s_wait_sem.handle = xSemaphoreCreateBinary();
    if(!s_irq_sem || !s_wait_sem.handle) {
        ESP_LOGE(TAG, "semaphore alloc failed");
        return ESP_ERR_NO_MEM;
    }

    const gpio_config_t pin_cfg = {
        .pin_bit_mask = 1ULL << irq_gpio,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_POSEDGE,
    };
    esp_err_t err = gpio_config(&pin_cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "irq gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }

    if(!s_isr_service_installed) {
        err = gpio_install_isr_service(0);
        if(err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
            s_isr_service_installed = true;
        } else {
            ESP_LOGE(TAG, "gpio_install_isr_service: %s", esp_err_to_name(err));
            return err;
        }
    }

    err = gpio_isr_handler_add(irq_gpio, irq_gpio_isr, NULL);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "isr_handler_add: %s", esp_err_to_name(err));
        return err;
    }

    BaseType_t ok = xTaskCreate(irq_task, "st25r_irq", IRQ_TASK_STACK,
                                NULL, IRQ_TASK_PRIO, &s_irq_task);
    if(ok != pdPASS) {
        ESP_LOGE(TAG, "irq task create failed");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "IRQ on GPIO%d (rising), task @ prio %d",
             irq_gpio, IRQ_TASK_PRIO);
    return ESP_OK;
}
