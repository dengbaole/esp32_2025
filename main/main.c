#include "platform.h"

static const char *TAG = "main";

static volatile uint16_t xl9555_button_level = 0xFFFF;

int get_button_level(int gpio) {
    return (xl9555_button_level&gpio)?1:0;
}

void xl9555_input_callback(uint16_t io_num,int level) {
    if(level) {
        xl9555_button_level |= io_num;
    } else {
        xl9555_button_level &= ~io_num;
    }
}

void short_press(int gpio) {
    ESP_LOGI(TAG,"Button %d short press",gpio);
}

void long_press(int gpio) {
    ESP_LOGI(TAG,"Button %d long press",gpio);
}

void button_init(void) {
    button_config_t button_cfg = {
        .active_level = 0,
        .getlevel_cb = get_button_level,
        .gpio_num = IO0_1,
        .long_cb = long_press,
        .long_press_time = 3000,
        .short_cb = short_press,
    };
    button_event_set(&button_cfg);
    button_cfg.gpio_num = IO0_2;
    button_event_set(&button_cfg);
    button_cfg.gpio_num = IO0_3;
    button_event_set(&button_cfg);
    button_cfg.gpio_num = IO0_4;
    button_event_set(&button_cfg);
}


void app_main(void) {
	// esp_task_wdt_delete(NULL); // 删除当前任务的看门狗
	// task_init();
	// led_init();
	// led_breath_init();

	xl9555_init(GPIO_NUM_10,GPIO_NUM_11,GPIO_NUM_17,xl9555_input_callback);
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