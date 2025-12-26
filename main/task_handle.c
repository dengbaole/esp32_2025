#include "task_handle.h"

void task_function(void* pvParameters) {
	// 从参数中获取任务ID和延迟时间
	int task_id = ((int*)pvParameters)[0];
	int delay_ms = ((int*)pvParameters)[1];
	if(task_id == 1) {
		
	}
	while(1) {
		ESP_LOGI("main", "Task %d running", task_id);
		vTaskDelay(pdMS_TO_TICKS(delay_ms));
	}
}

void task_init(void) {
	static int task1_params[2] = {1, 500};
	static int task2_params[2] = {2, 1000};

	xTaskCreatePinnedToCore(task_function, "task_1", 2048, (void*)task1_params, 10, NULL, 1);
	xTaskCreatePinnedToCore(task_function, "task_2", 2048, (void*)task2_params, 10, NULL, 1);
}
