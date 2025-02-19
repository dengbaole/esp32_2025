#include "platform.h"

#define NUM0_BIT BIT0
#define NUM1_BIT BIT1

static EventGroupHandle_t test_event;
t_sQMI8658 QMI8658; // 定义QMI8658结构体变量
static const char* TAG = "task_handle";


void task_handle(void* pvParameters) {
	//定时设置不同的事件位
	while(1) {
		//   esp_task_wdt_reset(); // 重置看门狗
		//   vTaskDelay(pdMS_TO_TICKS(1000));
		xEventGroupSetBits(test_event, NUM0_BIT);
		vTaskDelay(pdMS_TO_TICKS(1000));
		xEventGroupSetBits(test_event, NUM1_BIT);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


void task_handle_qmi8658(void* pvParameters) {
	EventBits_t ev;
	//打印事件位
	led_init();
	while(1) {
		ev = xEventGroupWaitBits(test_event, NUM0_BIT | NUM1_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		if(ev & NUM0_BIT) {
			printf("NUM0_BIT\n");
			led_on();
		}
		if(ev & NUM1_BIT) {
			printf("NUM1_BIT\n");
			led_off();
		}

		vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
		qmi8658_fetch_angleFromAcc(&QMI8658);   // 获取XYZ轴的倾角
		// 输出XYZ轴的倾角
		ESP_LOGI(TAG, "angle_x = %.1f  angle_y = %.1f angle_z = %.1f", QMI8658.AngleX, QMI8658.AngleY, QMI8658.AngleZ);
	}
}



void wifi_handle(void* pvParameters) {
	//打印事件位

	wifi_scan_m();
	// wifi_connect(2403,18649682167);
	while(1) {
		//ESP_LOGI(TAG, "Running task...");

	}
}

void task_init(void) {
	test_event = xEventGroupCreate();
	if (xTaskCreatePinnedToCore(task_handle, "task_handle", 2048, NULL, 10, NULL, 1) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create task_handle");
	}
	if (xTaskCreatePinnedToCore(task_handle_qmi8658, "task_handle_qmi8658", 4096, NULL, 10, NULL, 1) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create task_handle_qmi8658");
	}
	
	// xTaskCreatePinnedToCore(wifi_handle, "wifi_handle", 4096, NULL, 10, NULL, 0);
}