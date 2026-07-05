#include "platform.h"

static const char* TAG = "main";
static t_sQMI8658 imu;

static void on_key_event(key_id_t id, key_event_t event)
{
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
    imu_drv_init();

    task_init();
    while(1) {
        imu_drv_read_angle(&imu);
        ESP_LOGI(TAG, "angle_x=%.1f  angle_y=%.1f  angle_z=%.1f",
                 imu.AngleX, imu.AngleY, imu.AngleZ);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
