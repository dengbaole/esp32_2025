#include "i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "i2c_bus";
static i2c_master_bus_handle_t bus = NULL;
static bool inited = false;

void i2c_bus_init(void)
{
    if (inited) return;
    inited = true;

    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_1,
        .scl_io_num = GPIO_NUM_2,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus));
    ESP_LOGI(TAG, "I2C bus ready");
}

i2c_master_bus_handle_t i2c_bus_get(void)
{
    return bus;
}

i2c_master_dev_handle_t i2c_bus_add_device(uint8_t addr)
{
    i2c_master_dev_handle_t dev = NULL;
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &cfg, &dev));
    return dev;
}
