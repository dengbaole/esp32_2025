#include "iic_drv.h"


esp_err_t iic_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = IIC_SDA_PIN,
        .scl_io_num = IIC_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = IIC_CLK_SPEED,
    };
    i2c_param_config(IIC_PORT, &conf);
    return i2c_driver_install(IIC_PORT, conf.mode, 0, 0, 0);
}