#include "task_handle.h"

void task_handle(void* pvParameters) {
	while(1) {
		ESP_LOGI("main", "task_handle");
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


void task_handle2(void* pvParameters) {
	while(1) {
		ESP_LOGI("main", "task_handle2");
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}




void task_init(void) {
	xTaskCreatePinnedToCore(task_handle, "task_handle", 2048, NULL, 10, NULL, 1);
	xTaskCreatePinnedToCore(task_handle2, "task_handle2", 2048, NULL, 10, NULL, 1);
}

