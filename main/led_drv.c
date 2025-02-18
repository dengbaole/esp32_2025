#include "led_drv.h"


void led_init(void) {
	gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = (1ULL << LED_GPIO),
		.pull_down_en = 0,
		.pull_up_en = 0
	};
	gpio_config(&io_conf);
}


void led_on(void) {
	gpio_set_level(LED_GPIO, 1);
}

void led_off(void) {
	gpio_set_level(LED_GPIO, 0);
}