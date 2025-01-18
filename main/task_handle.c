#include "task_handle.h"


#define LED_GPIO GPIO_NUM_2


void led_handle(void* pvParameters) { 
	static int led_state = 0;
	while(1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		led_state = !led_state;
		gpio_set_level(LED_GPIO, led_state);
	}
}





void task_init(void) {
	gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = (1ULL << LED_GPIO),
		.pull_down_en = 0,
		.pull_up_en = 0
	};
	gpio_config(&io_conf);
	xTaskCreatePinnedToCore(led_handle, "led_handle", 2048, NULL, 10, NULL, 1);
}

