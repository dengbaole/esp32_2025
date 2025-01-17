#include "task_handle.h"


SemaphoreHandle_t bin_sem;

void task_handle(void* pvParameters) { 
	//释放信号量
	while(1) {
		xSemaphoreGive(bin_sem);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


void task_handle2(void* pvParameters) {
	//等待信号量
	while(1) {
		if(pdTRUE == xSemaphoreTake(bin_sem, portMAX_DELAY)) {
			ESP_LOGI("bin", "Semaphore taken");
		}
	}
}




void task_init(void) {
	bin_sem = xSemaphoreCreateBinary();
	xTaskCreatePinnedToCore(task_handle, "task_handle", 2048, NULL, 10, NULL, 1);
	xTaskCreatePinnedToCore(task_handle2, "task_handle2", 2048, NULL, 10, NULL, 1);
}

