#include "platform.h"

static const char* TAG = "main";

static void on_key_event(key_id_t id, key_event_t event) {
   switch (id) {
    case KEY_ID_BOOT:
        if (event == KEY_PRESS) {
            ESP_LOGI(TAG, "BOOT key pressed");
        } else {
            ESP_LOGI(TAG, "BOOT key released");
        }
        break;
    default:
        break;
    }
}

void app_main(void) {
    key_drv_init();
    key_drv_register_cb(on_key_event);

    task_init();
    while(1) {
        // ESP_LOGI(TAG, "main loop running");
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
    }
}
