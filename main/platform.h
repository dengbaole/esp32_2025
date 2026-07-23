#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "i2c_bus.h"

// 模块配置
#include "config.h"

#ifdef ENABLE_TASK_HANDLE
#include "task_handle.h"
#endif
#ifdef ENABLE_KEY
#include "key_drv.h"
#endif
#ifdef ENABLE_IMU
#include "imu_drv.h"
#endif
#ifdef ENABLE_SD
#include "sd_drv.h"
#endif
#ifdef ENABLE_AUDIO
#include "audio_drv.h"
#endif
#ifdef ENABLE_SPEAKER
#include "speaker_drv.h"
#endif
#ifdef ENABLE_LCD
#include "lcd_drv.h"
#endif
#if defined(ENABLE_LCD) || defined(ENABLE_SPEAKER) || defined(ENABLE_CAMERA)
#include "pca9557_drv.h"
#endif
#ifdef ENABLE_CAMERA
#include "camera_drv.h"
#endif

#endif // !_TASK_HANDLE_H
