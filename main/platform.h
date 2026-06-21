#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <stdio.h>
#include <string.h>

#include "button_drv.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp32_s3_qmi8658.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "led_drv.h"
#include "math.h"
#include "nvs_flash.h"
#include "task_handle.h"
#include "wifi_manager.h"
#include "xl9555.h"

#endif  // !_TASK_HANDLE_H