#include "key_drv.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define KEY_BOOT_GPIO       GPIO_NUM_0
#define KEY_DEBOUNCE_MS     20

static QueueHandle_t key_queue = NULL;
static key_cb_t key_callback = NULL;

// ISR 放在 IRAM 以快速响应
static void IRAM_ATTR key_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(key_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// 按键处理任务：接收中断 + 软件消抖
static void key_task(void* arg)
{
    uint32_t gpio_num;
    for (;;) {
        // 等待中断消息
        if (xQueueReceive(key_queue, &gpio_num, portMAX_DELAY)) {
            // 消抖：等 20ms 后再次读取电平
            vTaskDelay(pdMS_TO_TICKS(KEY_DEBOUNCE_MS));
            if (gpio_get_level(gpio_num) == 0) {
                // 确认按下
                if (key_callback) {
                    key_callback(KEY_ID_BOOT, KEY_PRESS);
                }

                // 等待按键释放
                while (gpio_get_level(gpio_num) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                // 消抖：等 20ms 确认释放
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

void key_drv_init(void)
{
    // 配置 GPIO0：输入 + 内部上拉 + 下降沿中断
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << KEY_BOOT_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    // 创建队列 + 任务 + 安装 ISR
    key_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(key_task, "key_task", 4096, NULL, 10, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(KEY_BOOT_GPIO, key_isr_handler, (void*)KEY_BOOT_GPIO);
}

void key_drv_register_cb(key_cb_t cb)
{
    key_callback = cb;
}
