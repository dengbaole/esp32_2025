#include "platform.h"

static const char* TAG = "main";

#ifdef ENABLE_IMU
static t_sQMI8658 imu;
#endif

#ifdef ENABLE_KEY
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
#endif

void app_main(void) {
#ifdef ENABLE_KEY
    key_drv_init();
    key_drv_register_cb(on_key_event);
#endif

#ifdef ENABLE_IMU
    imu_drv_init();
#endif

#ifdef ENABLE_SD
    if (sd_drv_init() == ESP_OK) {
        FILE *f = fopen(SD_MOUNT_POINT "/hello.txt", "w");
        if (f) {
            fprintf(f, "ESP32-S3 SD card test!\n");
            fclose(f);
            ESP_LOGI(TAG, "Test file written to SD card");
        }

        f = fopen(SD_MOUNT_POINT "/hello.txt", "r");
        if (f) {
            char line[64];
            if (fgets(line, sizeof(line), f)) {
                ESP_LOGI(TAG, "SD read: %s", line);
            }
            fclose(f);
        }
    }
#endif

#ifdef ENABLE_TASK_HANDLE
    task_init();
#endif

    while(1) {
#ifdef ENABLE_IMU
        imu_drv_read_angle(&imu);
        ESP_LOGI(TAG, "angle_x=%.1f  angle_y=%.1f  angle_z=%.1f",
                 imu.AngleX, imu.AngleY, imu.AngleZ);
#endif
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
