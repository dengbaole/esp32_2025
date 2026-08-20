// LVGL 集成层
// 复用 lcd_drv 已经初始化好的 ST7789 panel/io，不重新初始化 SPI/LCD，
// 避免和现有 lcd_drv / 摄像头 / 背光配置冲突。

#include "lvgl_drv.h"

#include "platform.h"
#include "lcd_drv.h"
#include "i2c_bus.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lvgl_port.h"
#include "demos/lv_demos.h"

static const char *TAG = "lvgl";
static bool s_started = false;

void lvgl_drv_start(void)
{
    if (s_started) {
        return;
    }
    s_started = true;

    // 1. 初始化 LVGL 自带的运行任务和 tick
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "lvgl_port_init failed: 0x%x", err);
        return;
    }

    // 2. 把当前 ST7789 panel 挂到 LVGL 上（不重新初始化 LCD）
    esp_lcd_panel_io_handle_t io_handle = lcd_drv_get_io();
    esp_lcd_panel_handle_t panel_handle = lcd_drv_get_panel();
    if (!io_handle || !panel_handle) {
        ESP_LOGE(TAG, "LCD not initialized");
        return;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = 320 * 20,                 // 20 行缓冲
        .double_buffer = true,
        .hres = 320,
        .vres = 240,
        .monochrome = false,
        .rotation = {
            .swap_xy = true,                     // 必须和 lcd_drv 里的设置一致
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,                 // LVGL 缓冲放 PSRAM
        },
    };
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
        return;
    }
    ESP_LOGI(TAG, "LVGL display added");

    // 3. 初始化 FT5x06 触摸（走共享新 I2C 总线，避免旧 I2C API 冲突）
    i2c_bus_init();
    esp_lcd_touch_handle_t tp = NULL;
    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    tp_io_cfg.scl_speed_hz = 100000;

    err = esp_lcd_new_panel_io_i2c_v2(i2c_bus_get(), &tp_io_cfg, &tp_io);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "touch panel io init failed: 0x%x", err);
    } else {
        esp_lcd_touch_config_t tp_cfg = {
            .x_max = 240,
            .y_max = 320,
            .rst_gpio_num = GPIO_NUM_NC,
            .int_gpio_num = GPIO_NUM_NC,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = 1,
                .mirror_x = 1,
                .mirror_y = 0,
            },
        };
        err = esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, &tp);
        if (err != ESP_OK || tp == NULL) {
            ESP_LOGW(TAG, "FT5x06 init failed: 0x%x", err);
        } else {
            const lvgl_port_touch_cfg_t touch_cfg = {
                .disp = disp,
                .handle = tp,
            };
            lvgl_port_add_touch(&touch_cfg);
            ESP_LOGI(TAG, "Touch ready");
        }
    }

    // 4. 启动示例界面（v8 widgets demo）
    lv_demo_widgets();
    ESP_LOGI(TAG, "LVGL started");
}
