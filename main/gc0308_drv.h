#ifndef GC0308_DRV_H
#define GC0308_DRV_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// GC0308 7 位 I2C 地址（写 = 0x42，读 = 0x43）
#define GC0308_I2C_ADDR    0x21
// GC0308 芯片 ID
#define GC0308_PID         0x9B

// 初始化 GC0308：软复位 -> 默认寄存器表 -> RGB565 -> QVGA(320x240) 1/2 子采样 -> 水平镜像
// 调用前需要共享 I2C 总线已初始化（函数内部也会确保 i2c_bus_init）
esp_err_t gc0308_drv_init(void);

#ifdef __cplusplus
}
#endif

#endif // GC0308_DRV_H
