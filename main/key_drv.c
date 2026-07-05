#include "key_drv.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static QueueHandle_t key_queue = NULL;
static key_cb_t key_callback = NULL;

static void IRAM_ATTR key_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t)arg;   // arg 来自 gpio_isr_handler_add 的第三个参数
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(key_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void key_task(void* arg) {
    uint32_t gpio_num;
    for (;;) {
        if (xQueueReceive(key_queue, &gpio_num, portMAX_DELAY)) {
            // 按下消抖
            vTaskDelay(pdMS_TO_TICKS(KEY_DEBOUNCE_MS));
            if (gpio_get_level(gpio_num) == 0) {
                if (key_callback) {
                    key_callback(KEY_ID_BOOT, KEY_PRESS);
                }

                // 等待释放
                while (gpio_get_level(gpio_num) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                // 释放消抖
                vTaskDelay(pdMS_TO_TICKS(KEY_DEBOUNCE_MS));
                if (gpio_get_level(gpio_num) == 1) {
                    if (key_callback) {
                        key_callback(KEY_ID_BOOT, KEY_RELEASE);
                    }
                }
            }
        }
    }
}

void key_drv_init(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << KEY_BOOT_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    key_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(key_task, "key_task", 4096, NULL, 10, NULL);
    gpio_install_isr_service(0);
    // 第三个参数 (void*)KEY_BOOT_GPIO 传给 ISR 的 arg，ISR 里通过 arg 知道是哪个引脚
    gpio_isr_handler_add(KEY_BOOT_GPIO, key_isr_handler, (void*)KEY_BOOT_GPIO);
}

void key_drv_register_cb(key_cb_t cb) {
    key_callback = cb;
}
