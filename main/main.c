#include "platform.h"

static const char* TAG = "main";
t_sQMI8658 QMI8658; // 定义QMI8658结构体变量

void app_main(void) {
	esp_task_wdt_delete(NULL); // 删除当前任务的看门狗
	ESP_ERROR_CHECK(iic_init());
	ESP_LOGI(TAG, "IIC init success");
	qmi8658_iic_init();
	task_init();
	sdcard_test();
	while(1) {
		//ESP_LOGI(TAG, "Running task...");
		esp_task_wdt_reset(); // 重置看门狗
		vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
        qmi8658_fetch_angleFromAcc(&QMI8658);   // 获取XYZ轴的倾角
        // 输出XYZ轴的倾角
        ESP_LOGI(TAG, "angle_x = %.1f  angle_y = %.1f angle_z = %.1f",QMI8658.AngleX, QMI8658.AngleY, QMI8658.AngleZ);
	}
}
