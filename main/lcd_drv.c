#include "lcd_drv.h"
#include "pca9557_drv.h"
#include "i2c_bus.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lcd_drv";

// ---- 硬件引脚 ----
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_MOSI        GPIO_NUM_40
#define LCD_SCLK        GPIO_NUM_41
#define LCD_DC          GPIO_NUM_39
#define LCD_RST         GPIO_NUM_NC
#define LCD_BACKLIGHT   GPIO_NUM_42
#define LCD_H_RES       320
#define LCD_V_RES       240
#define LCD_PIXEL_CLK   (80 * 1000 * 1000)

// ---- 背光 PWM ----
#define BL_LEDC_CH          LEDC_CHANNEL_0
#define BL_LEDC_TIMER       LEDC_TIMER_1

static esp_lcd_panel_handle_t panel = NULL;
static esp_lcd_panel_io_handle_t io = NULL;

// ---- 背光 ----
static void backlight_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = BL_LEDC_TIMER,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num = LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BL_LEDC_CH,
        .timer_sel = BL_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = true,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
}

void lcd_drv_set_backlight(int pct)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    uint32_t duty = (1023 * pct) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH);
}

// ---- ST7789 初始化 ----
static void st7789_init(void)
{
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = LCD_SCLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = GPIO_NUM_NC,     // CS 走 PCA9557，不走 SPI 硬件
        .pclk_hz = LCD_PIXEL_CLK,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 2,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,
                                               &io_cfg, &io));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_cfg, &panel));

    esp_lcd_panel_reset(panel);
    pca9557_drv_set_bit(PCA9557_IO_LCD_CS, 0);  // 拉低 CS
    esp_lcd_panel_init(panel);
    esp_lcd_panel_invert_color(panel, true);
    esp_lcd_panel_swap_xy(panel, true);
    esp_lcd_panel_mirror(panel, true, false);
}

esp_lcd_panel_handle_t lcd_drv_get_panel(void)
{
    return panel;
}

esp_lcd_panel_io_handle_t lcd_drv_get_io(void)
{
    return io;
}

void lcd_drv_fill(uint16_t color)
{
    if (!panel) return;
    uint16_t *buf = heap_caps_malloc(LCD_H_RES * 2, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!buf) return;
    for (int i = 0; i < LCD_H_RES; i++) buf[i] = color;
    for (int y = 0; y < LCD_V_RES; y++)
        esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y + 1, buf);
    heap_caps_free(buf);
}

void lcd_drv_draw_bitmap(int x, int y, int w, int h, const uint16_t *data)
{
    if (!panel) return;
    esp_lcd_panel_draw_bitmap(panel, x, y, x + w, y + h, data);
}

// ---- 对外接口 ----
void lcd_drv_init(void)
{
    i2c_bus_init();
    pca9557_drv_init();
    backlight_init();
    st7789_init();
    lcd_drv_fill(0x0000);                   // 清屏黑色
    esp_lcd_panel_disp_on_off(panel, true); // 开显示
    lcd_drv_set_backlight(100);             // 背光最亮
    ESP_LOGI(TAG, "LCD ready");
}
