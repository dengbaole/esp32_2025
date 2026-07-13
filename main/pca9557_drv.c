#include "pca9557_drv.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "pca9557";

#define PCA9557_ADDR            0x19
#define PCA9557_REG_INPUT       0x00
#define PCA9557_REG_OUTPUT      0x01
#define PCA9557_REG_POLARITY    0x02
#define PCA9557_REG_CONFIG      0x03

#define I2C_NUM    I2C_NUM_0

static esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(I2C_NUM, PCA9557_ADDR, buf, 2,
                                       pdMS_TO_TICKS(1000));
}

static esp_err_t read_reg(uint8_t reg, uint8_t *val)
{
    return i2c_master_write_read_device(I2C_NUM, PCA9557_ADDR,
                                         &reg, 1, val, 1, pdMS_TO_TICKS(1000));
}

void pca9557_drv_init(void)
{
    // IO0(CS)/IO1(PA_EN)/IO2(DVP_PWDN) 输出，其余输入
    write_reg(PCA9557_REG_CONFIG, 0xF8);
    // 默认：CS=1(失能), PA=0(功放关), DVP=1(摄像头断电)
    write_reg(PCA9557_REG_OUTPUT, 0x05);
    ESP_LOGI(TAG, "initialized");
}

esp_err_t pca9557_drv_set_bit(uint8_t bit, uint8_t level)
{
    uint8_t data;
    esp_err_t ret = read_reg(PCA9557_REG_OUTPUT, &data);
    if (ret != ESP_OK) return ret;
    if (level) data |= bit; else data &= ~bit;
    return write_reg(PCA9557_REG_OUTPUT, data);
}
