# ESP32-S3 立创·实战派 开发工程

芯片：ESP32-S3 (QFN56)，Flash 16MB，PSRAM 8MB（Octal），ESP-IDF v5.4.2

## 构建与烧录

```bash
make build        # 仅编译
make flash_only   # 仅烧录（USB-JTAG /dev/ttyACM0）
make monitor      # 串口监视（UART /dev/ttyUSB0，Ctrl+] 退出）
make flash        # 编译 + 烧录 + 监视
make clean        # 清理（会删 build/）
```

- ESP-IDF 路径：`~/esp/esp-idf`，用 makefile 里 source export.sh
- 烧录用 `/dev/ttyACM0`（USB-JTAG），监视用 `/dev/ttyUSB0`（CH342 UART）
- 串口被占用时先 `fuser -k /dev/ttyUSB0 /dev/ttyACM0`

## 模块架构

所有驱动在 `main/` 下，用 `main/config.h` 的宏开关控制（注释掉即禁用）：

```c
#define ENABLE_KEY          // BOOT 按键驱动 (key_drv)
#define ENABLE_IMU          // QMI8658 姿态传感器 (imu_drv)
#define ENABLE_SD           // Micro SD 卡 (sd_drv)
#define ENABLE_LCD          // ST7789 显示屏 (lcd_drv)
#define ENABLE_CAMERA       // GC0308 摄像头 (camera_drv, 依赖本地 managed_components)
#define ENABLE_AUDIO        // ES7210 录音 (audio_drv)
#define ENABLE_SPEAKER      // ES8311 播放 (speaker_drv)
#define ENABLE_TASK_HANDLE  // FreeRTOS 演示 (task_handle)
```

## 引脚规划（全部无冲突）

| GPIO | 功能 | 模块 |
|------|------|------|
| 0 | BOOT 按键（下降沿中断） | key_drv |
| 1 | I2C SDA（共用总线） | imu/audio/speaker/pca9557/camera |
| 2 | I2C SCL | 同上 |
| 5/7/3/46 | 摄像头 XCLK/PCLK/VSYNC/HREF | camera_drv |
| 12 | I2S DIN（录音） | audio_drv |
| 13/14/38 | I2S WS/BCK/MCK | audio/speaker |
| 16,18,8,17,15,6,4,9 | 摄像头 D0-D7 | camera_drv |
| 21/47/48 | SDMMC D0/CLK/CMD | sd_drv |
| 39/40/41/42 | LCD DC/MOSI/SCLK/背光 | lcd_drv |
| 45 | I2S DOUT（播放） | speaker_drv |

## I2C 总线（新 API 共享总线）

- `i2c_bus.h/c` 是共享 I2C 主总线（GPIO1/2，100kHz），所有驱动通过 `i2c_bus_add_device(addr)` 注册设备
- 设备地址：QMI8658=0x6A, ES7210=0x41, ES8311=0x18, PCA9557=0x19
- **不要**用旧 API `driver/i2c.h`（会跟摄像头新 API 冲突）
- PCA9557 IO 扩展器控制 LCD_CS(IO0)/PA_EN(IO1)/DVP_PWDN(IO2)，用 `pca9557_drv_set_bit`

## 音频注意

- ES7210（录音）和 ES8311（播放）共用 I2S 时钟线，不能同时用，代码里顺序执行
- ES8311 需要先开功放（pca9557 PA_EN）

## 摄像头

- GC0308，DVP 8位并口，320x240 RGB565
- 依赖 `managed_components/` 里的 esp32-camera（本地拷贝，联网装不了）
- 摄像头用 I2C 新 API，是所有 I2C 迁移到新总线的原因

## 屏幕方向

lcd_drv.c 里 `esp_lcd_panel_swap_xy / mirror` 控制横竖屏，改这三个参数即可
