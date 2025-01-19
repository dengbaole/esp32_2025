#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_task_wdt.h"




#include "task_handle.h"
#include "iic_drv.h"


#define LED_GPIO GPIO_NUM_4
#define IIC_SDA_PIN GPIO_NUM_48
#define IIC_SCL_PIN GPIO_NUM_45
#define IIC_PORT 0
#define IIC_CLK_SPEED 400000



#endif // !_TASK_HANDLE_H