#ifndef _IIC_DRV_H_
#define _IIC_DRV_H_

#include "platform.h"

typedef struct _i2c_obj_t {
	i2c_port_t port;
	gpio_num_t scl;
	gpio_num_t sda;
	esp_err_t init_flag;
} i2c_obj_t;




typedef struct _i2c_buf_t {
	size_t len;
	uint8_t* buf;
} i2c_buf_t;

extern i2c_obj_t iic_master[I2C_NUM_MAX];

/* 读写标志位 */
#define I2C_FLAG_READ                   (0x01)                                                          /* 读标志 */
#define I2C_FLAG_STOP                   (0x02)                                                          /* 停止标志 */
#define I2C_FLAG_WRITE                  (0x04)                                                          /* 写标志 */

/* 引脚与相关参数定义 */
#define IIC0_SDA_GPIO_PIN               GPIO_NUM_48                                                     /* IIC0_SDA引脚 */
#define IIC0_SCL_GPIO_PIN               GPIO_NUM_45                                                     /* IIC0_SCL引脚 */
#define IIC1_SDA_GPIO_PIN               GPIO_NUM_5                                                      /* IIC1_SDA引脚 */
#define IIC1_SCL_GPIO_PIN               GPIO_NUM_4                                                      /* IIC1_SCL引脚 */
#define IIC_FREQ                        400000                                                          /* IIC通信频率 */
#define I2C_MASTER_TX_BUF_DISABLE       0                                                               /* I2C主机不需要缓冲区 */
#define I2C_MASTER_RX_BUF_DISABLE       0                                                               /* I2C主机不需要缓冲区 */
#define ACK_CHECK_EN                    0x1                                                             /* I2C master将从slave检查ACK */

/* 函数声明 */
i2c_obj_t iic_init(uint8_t iic_port);                                                                   /* 初始化IIC */
esp_err_t i2c_transfer(i2c_obj_t* self, uint16_t addr, size_t n, i2c_buf_t* bufs, unsigned int flags);  /* IIC读写数据 */


#endif /* _IIC_DRV_H_ */