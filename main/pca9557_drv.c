#include "pca9557_drv.h"
#include "i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "pca9557";

#define PCA9557_ADDR            0x19
#define PCA9557_REG_INPUT       0x00
#define PCA9557_REG_OUTPUT      0x01
#define PCA9557_REG_POLARITY    0x02
#define PCA9557_REG_CONFIG      0x03

static i2c_master_dev_handle_t dev = NULL;

static void write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    i2c_master_transmit(dev, buf, 2, 100);
}

static esp_err_t read_reg(uint8_t reg, uint8_t *val)
{
    return i2c_master_transmit_receive(dev, &reg, 1, val, 1, 100);
}

void pca9557_drv_init(void)
{
    dev = i2c_bus_add_device(PCA9557_ADDR);
    write_reg(PCA9557_REG_CONFIG, 0xF8);
    write_reg(PCA9557_REG_OUTPUT, 0x05);
    ESP_LOGI(TAG, "initialized");
}

esp_err_t pca9557_drv_set_bit(uint8_t bit, uint8_t level)
{
    uint8_t data;
    esp_err_t ret = read_reg(PCA9557_REG_OUTPUT, &data);
    if (ret != ESP_OK) return ret;
    if (level) data |= bit; else data &= ~bit;
    write_reg(PCA9557_REG_OUTPUT, data);
    return ESP_OK;
}
