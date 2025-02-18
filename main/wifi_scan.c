#include "wifi_scan.h"


#define DEFAULT_SCAN_LIST_SIZE 10

static const char* TAG = "scan";

static void print_auth_mode(int authmode) {
	switch (authmode) {
		case WIFI_AUTH_OPEN:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
			break;
		case WIFI_AUTH_OWE:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OWE");
			break;
		case WIFI_AUTH_WEP:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
			break;
		case WIFI_AUTH_WPA_PSK:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
			break;
		case WIFI_AUTH_WPA2_PSK:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
			break;
		case WIFI_AUTH_WPA_WPA2_PSK:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
			break;
		case WIFI_AUTH_ENTERPRISE:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_ENTERPRISE");
			break;
		case WIFI_AUTH_WPA3_PSK:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
			break;
		case WIFI_AUTH_WPA2_WPA3_PSK:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
			break;
		case WIFI_AUTH_WPA3_ENT_192:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENT_192");
			break;
		default:
			ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
			break;
	}
}

static void print_cipher_type(int pairwise_cipher, int group_cipher) {
	switch (pairwise_cipher) {
		case WIFI_CIPHER_TYPE_NONE:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
			break;
		case WIFI_CIPHER_TYPE_WEP40:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
			break;
		case WIFI_CIPHER_TYPE_WEP104:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
			break;
		case WIFI_CIPHER_TYPE_TKIP:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
			break;
		case WIFI_CIPHER_TYPE_CCMP:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
			break;
		case WIFI_CIPHER_TYPE_TKIP_CCMP:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
			break;
		case WIFI_CIPHER_TYPE_AES_CMAC128:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
			break;
		case WIFI_CIPHER_TYPE_SMS4:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
			break;
		case WIFI_CIPHER_TYPE_GCMP:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
			break;
		case WIFI_CIPHER_TYPE_GCMP256:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
			break;
		default:
			ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
			break;
	}

	switch (group_cipher) {
		case WIFI_CIPHER_TYPE_NONE:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
			break;
		case WIFI_CIPHER_TYPE_WEP40:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
			break;
		case WIFI_CIPHER_TYPE_WEP104:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
			break;
		case WIFI_CIPHER_TYPE_TKIP:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
			break;
		case WIFI_CIPHER_TYPE_CCMP:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
			break;
		case WIFI_CIPHER_TYPE_TKIP_CCMP:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
			break;
		case WIFI_CIPHER_TYPE_SMS4:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
			break;
		case WIFI_CIPHER_TYPE_GCMP:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
			break;
		case WIFI_CIPHER_TYPE_GCMP256:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
			break;
		default:
			ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
			break;
	}
}

/* Initialize Wi-Fi as sta and set scan method */
void wifi_scan_m(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	uint16_t number = DEFAULT_SCAN_LIST_SIZE;
	wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
	uint16_t ap_count = 0;
	memset(ap_info, 0, sizeof(ap_info));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	esp_wifi_scan_start(NULL, true);
	ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
	ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
	for (int i = 0; i < number; i++) {
		ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
		ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
		print_auth_mode(ap_info[i].authmode);
		if (ap_info[i].authmode != WIFI_AUTH_WEP) {
			print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
		}
		ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);
	}

}

// void wifi_init(void) {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     // Initialize the WiFi driver
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
// }

void wifi_connect(const char* ssid, const char* password) {
	wifi_config_t wifi_config = {};
	strncpy((char*)wifi_config.sta.ssid, ssid, WIFI_SSID_MAX_LEN);
	strncpy((char*)wifi_config.sta.password, password, WIFI_PASS_MAX_LEN);

	wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());
	ESP_LOGI(TAG, "Connecting to %s...", ssid);
}