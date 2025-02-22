
#include "xl9555_iic_drv.h"


i2c_obj_t xl9555_i2c_master;
const char* xl9555_name = "xl9555.c";


esp_err_t xl9555_read_byte(uint8_t reg_addr, uint8_t* data, size_t len) {
	return i2c_master_write_read_device(IIC_PORT, XL9555_ADDR, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
}


esp_err_t xl9555_write_byte(uint8_t reg, uint8_t* data, size_t len) {
	uint8_t write_buf[len + 1];
	write_buf[0] = reg;
	memcpy(write_buf + 1, data, len);
	return i2c_master_write_to_device(IIC_PORT, XL9555_ADDR, write_buf, len + 1, 1000 / portTICK_PERIOD_MS);
}


uint16_t xl9555_pin_write(uint16_t pin, int val) {
	uint8_t w_data[2];
	uint16_t temp = 0x0000;

	xl9555_read_byte(XL9555_INPUT_PORT0_REG, w_data, 2);

	if (pin <= 0x0080) {
		if (val) {
			w_data[0] |= (uint8_t)(0xFF & pin);
		} else {
			w_data[0] &= ~(uint8_t)(0xFF & pin);
		}
	} else {
		if (val) {
			w_data[1] |= (uint8_t)(0xFF & (pin >> 8));
		} else {
			w_data[1] &= ~(uint8_t)(0xFF & (pin >> 8));
		}
	}

	temp = ((uint16_t)w_data[1] << 8) | w_data[0];

	xl9555_write_byte(XL9555_OUTPUT_PORT0_REG, w_data, 2);

	return temp;
}


int xl9555_pin_read(uint16_t pin) {
	uint16_t ret;
	uint8_t r_data[2];

	xl9555_read_byte(XL9555_INPUT_PORT0_REG, r_data, 2);

	ret = r_data[1] << 8 | r_data[0];

	return (ret & pin) ? 1 : 0;
}


void xl9555_ioconfig(uint16_t config_value) {
	/* 从机地址 + CMD + data1(P0) + data2(P1) */
	/* P00、P01、P04、P06为输入，其他引脚为输出 -->0000 0000 0101 0011 注意：0为输出，1为输入*/
	uint8_t data[2];
	esp_err_t err;

	data[0] = (uint8_t)(0xFF & config_value);
	data[1] = (uint8_t)(0xFF & (config_value >> 8));

	do {
		err = xl9555_write_byte(XL9555_CONFIG_PORT0_REG, data, 2);

		if (err != ESP_OK) {
			ESP_LOGE(xl9555_name, "%s configure %X failed, ret: %d", __func__, config_value, err);
		}

		vTaskDelay(pdMS_TO_TICKS(100));

	} while (err != ESP_OK);
}


static void IRAM_ATTR xl9555_exit_gpio_isr_handler(void* arg) {
	uint32_t gpio_num = (uint32_t) arg;

	if (gpio_num == XL9555_INT_IO) {
		esp_rom_delay_us(20000);

		if ( gpio_get_level(XL9555_INT_IO) == 0) {
			/* 中断处理 */
		}
	}
}


void xl9555_int_init(void) {
	gpio_config_t gpio_init_struct;

	/* 配置BOOT引脚和外部中断 */
	gpio_init_struct.mode = GPIO_MODE_INPUT;                    /* 选择为输入模式 */
	gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;           /* 上拉使能 */
	gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;      /* 下拉失能 */
	gpio_init_struct.intr_type = GPIO_INTR_NEGEDGE;             /* 下降沿触发 */
	gpio_init_struct.pin_bit_mask = 1ull << XL9555_INT_IO;      /* 设置的引脚的位掩码 */
	gpio_config(&gpio_init_struct);                             /* 配置使能 */

	/* 注册中断服务 */
	gpio_install_isr_service(0);

	/* 设置GPIO的中断回调函数 */
	gpio_isr_handler_add(XL9555_INT_IO, xl9555_exit_gpio_isr_handler, (void*) XL9555_INT_IO);
}


void xl9555_init(i2c_obj_t self) {
	uint8_t r_data[2];

	/* 前面未初始化IIC! */
	if (self.init_flag == ESP_FAIL) {
		xl9555_i2c_master = iic_init(I2C_NUM_0);  /* 初始化IIC */
	}

	xl9555_i2c_master = self;

	/* 输入模式下，中断才有效（读取IO电平） */
	// xl9555_int_init();

	/* 上电先读取一次清除中断标志 */
	xl9555_read_byte(XL9555_INPUT_PORT0_REG, r_data, 2);

	/* 配置那些扩展管脚为输入输出模式 */
	xl9555_ioconfig(0xFE1B);

	/* 芯片初始化过程中先关闭蜂鸣器 */
	xl9555_pin_write(BEEP_IO, 1);

	/* 关闭喇叭 */

	xl9555_pin_write(SPK_CTRL_IO, 0);

}


uint8_t xl9555_key_scan(uint8_t mode) {
	uint8_t keyval = 0;
	static uint8_t key_up = 1;                                          /* 按键按松开标志 */

	if (mode) {
		key_up = 1;                                                     /* 支持连按 */
	}

	if (key_up && (KEY0 == 0 || KEY1 == 0)) {                           /* 按键松开标志为1, 且有任意一个按键按下了 */
		vTaskDelay(10);                                                 /* 去抖动 */
		key_up = 0;

		if (KEY0 == 0) {
			keyval = KEY0_PRES;
		}

		if (KEY1 == 0) {
			keyval = KEY1_PRES;
		}
	} else if (KEY0 == 1 && KEY1 == 1) {                                /* 没有任何按键按下, 标记按键松开 */
		key_up = 1;
	}

	return keyval;                                                      /* 返回键值 */
}
