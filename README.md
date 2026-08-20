# ESP32-S3 立创·实战派 开发工程

芯片：ESP32-S3 (QFN56)，Flash 16MB，PSRAM 8MB，ESP-IDF v5.4

## 快速开始

```bash
make flash          # 一键编译 + 烧录 + 串口监视
make build          # 仅编译
make flash_only     # 仅烧录
make monitor        # 仅串口（Ctrl+] 退出）
make menuconfig     # 配置菜单
make clean          # 清理
```

串口自动检测：烧录用 USB-JTAG (`/dev/ttyACM0`)，监视用 UART (`/dev/ttyUSB0`)。

## 模块开关

编辑 [config.h](main/config.h)，注释掉对应的 `#define` 即可禁用模块：

```c
#define ENABLE_KEY          // BOOT 按键驱动
#define ENABLE_IMU          // QMI8658 姿态传感器
#define ENABLE_SD           // Micro SD 卡
#define ENABLE_LCD          // ST7789 LCD 显示屏
#define ENABLE_CAMERA       // GC0308 摄像头（裸写，不依赖组件）
#define ENABLE_LVGL         // LVGL 图形库（复用 lcd_drv，启动 widgets 示例）
#define ENABLE_AUDIO        // ES7210 音频录音
#define ENABLE_SPEAKER      // ES8311 音频播放
#define ENABLE_TASK_HANDLE  // FreeRTOS 事件系统演示
```

> ENABLE_AUDIO 和 ENABLE_SPEAKER 共用 I2S 时钟线，同时开启时顺序执行：先录音再播放。

## 引脚规划

| GPIO | 功能 | 模块 |
|------|------|------|
| 0 | BOOT 按键（下降沿中断） | key_drv |
| 1 | I2C SDA（共用） | imu / audio / speaker |
| 2 | I2C SCL（共用） | imu / audio / speaker |
| 12 | I2S DIN（录音输入） | audio_drv |
| 13 | I2S WS（字选择） | audio / speaker |
| 14 | I2S BCK（位时钟） | audio / speaker |
| 21 | SDMMC D0 | sd_drv |
| 38 | I2S MCLK（主时钟） | audio / speaker |
| 45 | I2S DOUT（播放输出） | speaker_drv |
| 47 | SDMMC CLK | sd_drv |
| 48 | SDMMC CMD | sd_drv |

### I2C 总线设备

| 地址 | 芯片 | 用途 |
|------|------|------|
| 0x18 | ES8311 | 音频 DAC（播放） |
| 0x19 | PCA9557 | IO 扩展（功放使能） |
| 0x41 | ES7210 | 音频 ADC（录音） |
| 0x6A | QMI8658 | 6 轴 IMU 姿态 |

## 项目结构

```
├── CMakeLists.txt
├── makefile
├── sdkconfig
├── main
│   ├── CMakeLists.txt
│   ├── config.h             # 模块开关
│   ├── platform.h           # 公共头文件
│   ├── main.c               # 应用入口
│   ├── i2c_bus.c / .h       # 共享 I2C 总线（新 API）
│   ├── pca9557_drv.c / .h   # PCA9557 IO 扩展器
│   ├── key_drv.c / .h       # BOOT 按键驱动
│   ├── imu_drv.c / .h       # QMI8658 姿态传感器驱动
│   ├── sd_drv.c / .h        # Micro SD 卡驱动
│   ├── audio_drv.c / .h     # ES7210 音频录音驱动
│   ├── speaker_drv.c / .h   # ES8311 音频播放驱动
│   ├── lcd_drv.c / .h       # ST7789 LCD 驱动
│   ├── gc0308_drv.c / .h    # GC0308 传感器驱动（裸 I2C 寄存器）
│   ├── camera_drv.c / .h    # 摄像头采集驱动（LCD_CAM + GDMA）
│   ├── format_wav.h         # WAV 文件头
│   └── task_handle.c / .h   # FreeRTOS 事件系统演示
└── README.md
```

## 模块说明

### BOOT 按键 (key_drv)

- GPIO0，下降沿中断 + 20ms 软件消抖
- ISR 通过队列通知任务，回调通知应用层
- 支持按下/释放双事件

### QMI8658 姿态传感器 (imu_drv)

- I2C 通信（地址 0x6A），100kHz
- 6 轴 IMU：加速度计 ±4g + 陀螺仪 ±512dps
- 纯加速度计计算 X/Y/Z 倾角

### Micro SD 卡 (sd_drv)

- SDMMC 1-bit 模式，FAT 文件系统，挂载路径 `/sdcard`
- 支持中文长文件名（GBK 编码）
- 挂载失败自动格式化

### ES7210 音频录音 (audio_drv)

- I2S TDM 模式，48kHz / 16bit / 立体声
- 裸 I2C 寄存器驱动，零外部依赖
- 录制 PCM WAV 到 SD 卡

### ES8311 音频播放 (speaker_drv)

- I2S Standard 模式，48kHz / 16bit / 立体声
- 裸 I2C 寄存器驱动，零外部依赖
- 从 SD 卡读 WAV/PCM 文件播放
- PCA9557 IO 扩展器控制功放使能
- 音量范围 0-100（默认 70%）

### GC0308 摄像头 (gc0308_drv + camera_drv)

- 已去组件化：不再依赖 esp32-camera，`managed_components/` 只留作学习对照
- `gc0308_drv.c`：传感器层，I2C(0x21) 软复位 + 239 条默认寄存器 + RGB565 + QVGA 1/2 子采样
- `camera_drv.c`：控制器层，LEDC 产生 24MHz XCLK，LCD_CAM 接收 DVP，GDMA 环形描述符直接写 PSRAM 双缓冲
- VSYNC 中断状态机：第一个 VSYNC 武装 DMA，之后每个 VSYNC 表示一帧完成并回调
- API 与原来完全一致：`camera_drv_init / start(cb) / stop`

### LVGL 图形库 (lvgl_drv)

- 复用 `lcd_drv` 已初始化好的 ST7789 panel/io，不重复初始化 LCD/SPI
- 触摸 FT5x06(0x38) 走共享新 I2C 总线，避免与摄像头/IMU 冲突
- 默认运行 `lv_demo_widgets()` 示例；在摄像头预览结束后启动
- 依赖本地组件：`lvgl__lvgl`、`espressif__esp_lvgl_port`、`espressif__esp_lcd_touch(_ft5x06)`
- LVGL 固件较大，改用 `partitions.csv` 给 app 分 4MB
