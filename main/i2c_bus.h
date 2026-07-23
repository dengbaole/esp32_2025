#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// 初始化共享 I2C 总线（GPIO1=SDA, GPIO2=SCL, 100kHz），只调一次
void i2c_bus_init(void);

// 获取总线句柄
i2c_master_bus_handle_t i2c_bus_get(void);

// 在共享总线上注册一个 I2C 设备，返回设备句柄
i2c_master_dev_handle_t i2c_bus_add_device(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif
