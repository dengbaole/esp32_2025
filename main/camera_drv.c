#include "camera_drv.h"
#include "pca9557_drv.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "camera";

// ---- DVP 8-bit 并口引脚 ----
#define CAM_PIN_D0      16
#define CAM_PIN_D1      18
#define CAM_PIN_D2      8
#define CAM_PIN_D3      17
#define CAM_PIN_D4      15
#define CAM_PIN_D5      6
#define CAM_PIN_D6      4
#define CAM_PIN_D7      9
#define CAM_PIN_XCLK    5
#define CAM_PIN_PCLK    7
#define CAM_PIN_VSYNC   3
#define CAM_PIN_HREF    46
#define CAM_PIN_SIOD    1
#define CAM_PIN_SIOC    2

static bool running = false;
static camera_frame_cb_t frame_cb = NULL;
static QueueHandle_t frame_queue = NULL;

// ---- 摄像头初始化 ----
void camera_drv_init(void)
{
    // 给摄像头上电（PCA9557 DVP_PWDN 拉低）
    pca9557_drv_set_bit(PCA9557_IO_DVP_PWDN, 0);

    camera_config_t cfg = {
        .pin_d0 = CAM_PIN_D0,
        .pin_d1 = CAM_PIN_D1,
        .pin_d2 = CAM_PIN_D2,
        .pin_d3 = CAM_PIN_D3,
        .pin_d4 = CAM_PIN_D4,
        .pin_d5 = CAM_PIN_D5,
        .pin_d6 = CAM_PIN_D6,
        .pin_d7 = CAM_PIN_D7,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_pclk = CAM_PIN_PCLK,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_sccb_sda = -1,            // 复用已有 I2C 总线
        .pin_sccb_scl = CAM_PIN_SIOC,
        .sccb_i2c_port = 0,
        .pin_pwdn = -1,                // PWDN 走 PCA9557
        .pin_reset = -1,               // 不复位
        .xclk_freq_hz = 24000000,      // 24MHz
        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QVGA,  // 320x240
        .jpeg_quality = 12,
        .fb_count = 2,                 // 双缓冲
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x", err);
        return;
    }

    // GC0308 镜像设置
    sensor_t *s = esp_camera_sensor_get();
    if (s && s->id.PID == GC0308_PID) {
        s->set_hmirror(s, 1);
    }

    ESP_LOGI(TAG, "Camera ready");
}

// ---- 取帧任务 ----
static void task_camera(void *arg)
{
    while (running) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            if (frame_cb) {
                frame_cb((const uint16_t *)fb->buf, fb->width, fb->height);
            }
            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void camera_drv_start(camera_frame_cb_t on_frame)
{
    frame_cb = on_frame;
    running = true;
    xTaskCreatePinnedToCore(task_camera, "camera", 4096, NULL, 5, NULL, 1);
    ESP_LOGI(TAG, "Preview started");
}

void camera_drv_stop(void)
{
    running = false;
    frame_cb = NULL;
}
