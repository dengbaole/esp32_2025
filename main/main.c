#include "platform.h"

static const char* TAG = "main";


void app_main(void) {
	esp_task_wdt_delete(NULL); // 删除当前任务的看门狗

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	sdcard_test();
	i2c0_master = iic_init(I2C_NUM_0);
	qmi8658_iic_init();
	xl9555_init(i2c0_master);
	lcd_cfg_t lcd_config_info = {0};
	lcd_config_info.notify_flush_ready = NULL;
	lcd_init(lcd_config_info);
	lcd_draw_circle(100, 100, 50, BLUE);
	ESP_ERROR_CHECK( ret );
	task_init();
	while(1) {
		esp_task_wdt_reset(); // 重置看门狗
		vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
	}
}
