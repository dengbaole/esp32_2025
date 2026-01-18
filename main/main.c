#include "platform.h"

static const char *TAG = "main";



void app_main(void) {
	esp_task_wdt_delete(NULL); // 删除当前任务的看门狗
	task_init();
	led_init();
	led_breath_init();
	while (1) {
		set_effect_1();	
		// ESP_LOGI();
		// esp_task_wdt_reset();			 // 重置看门狗
		// gpio_set_level(GPIO_NUM_15,1);
		// vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
		// gpio_set_level(GPIO_NUM_15,0);
		// vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
		
	}
}