#include "platform.h"
#include "ap_config.h"

static const char* TAG = "main";


void xl9555_input_callback(uint16_t io_num, int level) {
	if(level) {
		xl9555_button_level |= io_num;
	} else {
		xl9555_button_level &= ~io_num;
	}
}


/* ===== 配网回调 =====
 * 当配网模块状态变化时，会调用这个函数
 * AP_CONFIG_STATE_WAITING         → 热点已开，等用户连手机配网
 * AP_CONFIG_STATE_GET_SSID_PWD    → 用户已提交WiFi信息，去连接
 */
static void ap_config_callback(ap_config_state_t state) {
	switch (state) {
		case AP_CONFIG_STATE_WAITING:
			ESP_LOGI(TAG, "AP config started, connect to hotspot and visit http://192.168.4.1");
			break;
		case AP_CONFIG_STATE_GET_SSID_PWD:
			ESP_LOGI(TAG, "Received Wi-Fi: SSID=%s", ap_config_get_ssid());
			wifi_manager_connect(ap_config_get_ssid(), ap_config_get_password());
			break;
		default:
			break;
	}
}

void wifi_state_handler(WIFI_STATE state) {
	if(state == WIFI_STATE_CONNECTED) {
		ESP_LOGI(TAG, "Wifi connect success!");
		// 连上了，关掉热点节省资源
		ap_config_stop();
	} else {
		ESP_LOGI(TAG, "Wifi disconnect!");
	}
}


void app_main(void) {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// 注册看门狗，让 main 循环里的 esp_task_wdt_reset() 能正常喂狗
	esp_task_wdt_add(NULL);

	wifi_manager_init(wifi_state_handler);

	/* ===== 配网核心逻辑 =====
	 * 开机二选一：
	 *   - 有保存的WiFi配置 → 直接连接
	 *   - 没有              → 开热点等用户配网
	 */
	if (ap_config_load_saved()) {
		ESP_LOGI(TAG, "Found saved Wi-Fi config: %s", ap_config_get_ssid());
		wifi_manager_connect(ap_config_get_ssid(), ap_config_get_password());
	} else {
		ESP_LOGI(TAG, "No saved config, start AP config mode");
		// 热点名: ESP32_Setup  密码: 12345678
		ap_config_start("ESP32_Setup", "12345678", ap_config_callback);
	}

	xl9555_init(GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_17, xl9555_input_callback);
	xl9555_ioconfig(0xffff);
	button_init();

	while (1) {
		esp_task_wdt_reset();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
