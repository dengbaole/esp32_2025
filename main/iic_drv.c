#include "iic_drv.h"







// esp_err_t iic_init(void) {
// 	i2c_config_t conf = {
// 		.mode = I2C_MODE_MASTER,
// 		.sda_io_num = IIC_SDA_PIN,
// 		.scl_io_num = IIC_SCL_PIN,
// 		.sda_pullup_en = GPIO_PULLUP_ENABLE,
// 		.scl_pullup_en = GPIO_PULLUP_ENABLE,
// 		.master.clk_speed = IIC_CLK_SPEED,
// 	};
// 	i2c_param_config(IIC_PORT, &conf);
// 	return i2c_driver_install(IIC_PORT, conf.mode, 0, 0, 0);
// }


i2c_obj_t iic_master[I2C_NUM_MAX];  /* 为IIC0和IIC1分别定义IIC控制块结构体 */
i2c_obj_t i2c0_master;
const char* iic_name = "iic.c";


i2c_obj_t iic_init(uint8_t iic_port) {
	uint8_t i;
	i2c_config_t iic_config_struct = {0};

	if (iic_port == I2C_NUM_0) {
		i = 0;
	} else {
		i = 1;
	}

	iic_master[i].port = iic_port;
	iic_master[i].init_flag = ESP_FAIL;

	if (iic_master[i].port == I2C_NUM_0) {
		iic_master[i].scl = IIC0_SCL_GPIO_PIN;
		iic_master[i].sda = IIC0_SDA_GPIO_PIN;
	} else {
		iic_master[i].scl = IIC1_SCL_GPIO_PIN;
		iic_master[i].sda = IIC1_SDA_GPIO_PIN;
	}

	iic_config_struct.mode = I2C_MODE_MASTER;                               /* 设置IIC模式-主机模式 */
	iic_config_struct.sda_io_num = iic_master[i].sda;                       /* 设置IIC_SDA引脚 */
	iic_config_struct.scl_io_num = iic_master[i].scl;                       /* 设置IIC_SCL引脚 */
	iic_config_struct.sda_pullup_en = GPIO_PULLUP_ENABLE;                   /* 配置IIC_SDA引脚上拉使能 */
	iic_config_struct.scl_pullup_en = GPIO_PULLUP_ENABLE;                   /* 配置IIC_SCL引脚上拉使能 */
	iic_config_struct.master.clk_speed = IIC_FREQ;                          /* 设置IIC通信速率 */
	i2c_param_config(iic_master[i].port, &iic_config_struct);               /* 设置IIC初始化参数 */

	/* 激活I2C控制器的驱动 */
	iic_master[i].init_flag = i2c_driver_install(iic_master[i].port,        /* 端口号 */
							  iic_config_struct.mode,    /* 主机模式 */
							  I2C_MASTER_RX_BUF_DISABLE, /* 从机模式下接收缓存大小(主机模式不使用) */
							  I2C_MASTER_TX_BUF_DISABLE, /* 从机模式下发送缓存大小(主机模式不使用) */
							  0);                        /* 用于分配中断的标志(通常从机模式使用) */

	if (iic_master[i].init_flag != ESP_OK) {
		while(1) {
			ESP_LOGI(iic_name, "%s , ret: %d", __func__, iic_master[i].init_flag);
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}

	return iic_master[i];
}


