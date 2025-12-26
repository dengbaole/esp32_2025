#include "platform.h"

static const char* TAG = "main";

void app_main(void) {
	esp_task_wdt_delete(NULL); // 删除当前任务的看门狗

	task_init();
	bsp_i2c_init();
	pca9557_init();
	bsp_lcd_init();
	lcd_set_color(4444);
	while(1) {
		//ESP_LOGI(TAG, "Running task...");
		esp_task_wdt_reset(); // 重置看门狗
		vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
	}
}
