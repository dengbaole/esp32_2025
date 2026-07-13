#ifndef LCD_DRV_H
#define LCD_DRV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 LCD（SPI + ST7789 + 背光）
void lcd_drv_init(void);

// 填充整屏颜色（RGB565）
void lcd_drv_fill(uint16_t color);

// 画位图
void lcd_drv_draw_bitmap(int x, int y, int w, int h, const uint16_t *data);

// 背光亮度 0-100
void lcd_drv_set_backlight(int pct);

#ifdef __cplusplus
}
#endif

#endif
