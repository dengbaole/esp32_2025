#ifndef CONFIG_H
#define CONFIG_H

// ===== 模块开关：注释掉对应的 #define 即禁用 =====

#define ENABLE_KEY          // BOOT 按键驱动
#define ENABLE_IMU          // QMI8658 姿态传感器
#define ENABLE_SD           // Micro SD 卡
#define ENABLE_LCD          // ST7789 LCD 显示屏
#define ENABLE_AUDIO        // ES7210 音频录音
#define ENABLE_SPEAKER      // ES8311 音频播放
#define ENABLE_TASK_HANDLE  // FreeRTOS 事件系统演示

#endif // CONFIG_H
