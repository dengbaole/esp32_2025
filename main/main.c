#include "platform.h"

static const char *TAG = "main";

void xl9555_callback(uint16_t pin,int level) {
	switch (pin) {
	case IO0_1:
		/* code */
		ESP_LOGI(TAG,"BUTTON 1 CHEKK LEVEL:%d",level);
		break;
	case IO0_2:
		ESP_LOGI(TAG,"BUTTON 2 CHEKK LEVEL:%d",level);
		/* code */
		break;
	case IO0_3:
		ESP_LOGI(TAG,"BUTTON 3 CHEKK LEVEL:%d",level);
		/* code */
		break;
	case IO0_4:
		ESP_LOGI(TAG,"BUTTON 4 CHEKK LEVEL:%d",level);
		/* code */
		break;
	
	default:
		break;
	}
}

void app_main(void) {
	// esp_task_wdt_delete(NULL); // 删除当前任务的看门狗
	// task_init();
	// led_init();
	// led_breath_init();

	xl9555_init(GPIO_NUM_10,GPIO_NUM_11,GPIO_NUM_17,xl9555_callback);
	xl9555_ioconfig(0xffff);
	while (1) {
		// set_effect_1();	
		// ESP_LOGI();
		// esp_task_wdt_reset();			 // 重置看门狗
		// gpio_set_level(GPIO_NUM_15,1);
		// vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
		// gpio_set_level(GPIO_NUM_15,0);
		// vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
		esp_task_wdt_reset();  // Reset watchdog timer
    	vTaskDelay(pdMS_TO_TICKS(1000));  //	
	}
}