#ifndef PCA9557_DRV_H
#define PCA9557_DRV_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// PCA9557 IO 引脚定义
#define PCA9557_IO_LCD_CS    BIT(0)   // IO0 — LCD 片选
#define PCA9557_IO_PA_EN     BIT(1)   // IO1 — 功放使能
#define PCA9557_IO_DVP_PWDN  BIT(2)   // IO2 — 摄像头断电

// 初始化 PCA9557（IO0/1/2 设为输出，默认 CS=1 PA=0 DVP=1）
void pca9557_drv_init(void);

// 设置某个 IO 引脚的高低电平（read-modify-write，不影响其他引脚）
esp_err_t pca9557_drv_set_bit(uint8_t bit, uint8_t level);

#ifdef __cplusplus
}
#endif

#endif
