#include "platform.h"

static const char* TAG = "main";



void xl9555_input_callback(uint16_t io_num, int level) {
	if(level) {
		xl9555_button_level |= io_num;
	} else {
		xl9555_button_level &= ~io_num;
	}
}



#define DEFAULT_WIFI_SSID           "802"
#define DEFAULT_WIFI_PASSWORD       "aa1550555930"

void wifi_state_handler(WIFI_STATE state) {
	if(state == WIFI_STATE_CONNECTED) {
		ESP_LOGI(TAG, "Wifi connect success!");
	} else {
		ESP_LOGI(TAG, "Wifi disconnect! ");
	}
}


void app_main(void) {
	// esp_task_wdt_delete(NULL); // 删除当前任务的看门狗
	// task_init();
	// led_init();
	// led_breath_init();

	//wifi
	nvs_flash_init();
	wifi_manager_init(wifi_state_handler);
	wifi_manager_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);



	xl9555_init(GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_17, xl9555_input_callback);
	xl9555_ioconfig(0xffff);
	button_init();

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