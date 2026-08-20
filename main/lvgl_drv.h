#ifndef LVGL_DRV_H
#define LVGL_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

// 启动 LVGL：复用 lcd_drv 已初始化的 ST7789 面板，并添加触摸屏输入
// 依赖：ENABLE_LCD 已开启且 lcd_drv_init() 已完成
void lvgl_drv_start(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_DRV_H
