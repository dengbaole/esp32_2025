#include "task_handle.h"

#define NUM0_BIT BIT0
#define NUM1_BIT BIT1

static EventGroupHandle_t test_event;

void task_handle(void* pvParameters) { 
	//定时设置不同的事件位
	while(1) {
		xEventGroupSetBits(test_event, NUM0_BIT);
		vTaskDelay(pdMS_TO_TICKS(1000));
		xEventGroupSetBits(test_event, NUM1_BIT);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


void task_handle2(void* pvParameters) {
	EventBits_t ev;
	//打印事件位
	while(1) {
		ev = xEventGroupWaitBits(test_event, NUM0_BIT | NUM1_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		if(ev & NUM0_BIT) {
			printf("NUM0_BIT\n");
		}
		if(ev & NUM1_BIT) {
			printf("NUM1_BIT\n");
		}
	}
}




void task_init(void) {
	test_event = xEventGroupCreate();
	xTaskCreatePinnedToCore(task_handle, "task_handle", 2048, NULL, 10, NULL, 1);
	xTaskCreatePinnedToCore(task_handle2, "task_handle2", 2048, NULL, 10, NULL, 1);
}

