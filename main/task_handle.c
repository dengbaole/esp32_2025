#include "task_handle.h"


QueueHandle_t queue_handle = NULL;


typedef struct {
    uint16_t value;
}queue_data_t;


void task_handle(void* pvParameters) { 
	//从队列里面接收数据并打印
	queue_data_t queue_data;
	while(1) {
		if(pdTRUE == xQueueReceive(queue_handle, &queue_data, 100)) {
			ESP_LOGI("queuee", "task_handle: %d", queue_data.value);
		}
	}
}


void task_handle2(void* pvParameters) {
	//向队列里面发送数据并打印
	queue_data_t queue_data;
	memset(&queue_data, 0, sizeof(queue_data_t));
	while(1) {
		xQueueSend(queue_handle, &queue_data, 100);
		vTaskDelay(pdMS_TO_TICKS(1000));
		queue_data.value++;
	}
}




void task_init(void) {
	queue_handle = xQueueCreate(10, sizeof(queue_data_t));
	xTaskCreatePinnedToCore(task_handle, "task_handle", 2048, NULL, 10, NULL, 1);
	xTaskCreatePinnedToCore(task_handle2, "task_handle2", 2048, NULL, 10, NULL, 1);
}

