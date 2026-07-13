#include "platform.h"
#ifdef ENABLE_LCD
#include "yingwu.h"
#endif

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

#ifdef ENABLE_LCD
    lcd_drv_init();
    lcd_drv_fill(0x001F);  // 蓝色背景
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_drv_fill(0x07E0);  // 绿色
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_drv_fill(0xF800);  // 红色
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_drv_fill(0x0000);  // 黑屏
    vTaskDelay(pdMS_TO_TICKS(500));
    lcd_drv_draw_bitmap(0, 0, 320, 240, (const uint16_t *)gImage_yingwu);
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

// 顺序执行：先录音再播放（共用 I2S 时钟线，不能同时）
#if defined(ENABLE_AUDIO) && defined(ENABLE_SPEAKER)
        audio_drv_init();
        audio_drv_record("/RECORD.WAV", 5);
        audio_drv_deinit();

        speaker_drv_init();
        speaker_drv_set_volume(80);
        speaker_drv_play("/RECORD.WAV");   // 播放刚才录的
        speaker_drv_deinit();
#elif defined(ENABLE_AUDIO)
        audio_drv_init();
        audio_drv_record("/RECORD.WAV", 5);
        audio_drv_deinit();
#elif defined(ENABLE_SPEAKER)
        speaker_drv_init();
        speaker_drv_play("/canon.pcm");
        speaker_drv_deinit();
#endif
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
