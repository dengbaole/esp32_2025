# ESP32-S3 硬件驱动项目

ESP32-S3 嵌入式开发项目，集成多种外设驱动和系统功能。

## 功能模块

| 模块 | 文件 | 说明 |
|------|------|------|
| WiFi 管理 | `wifi_manager.c/h` | STA 模式连接，自动重连，状态回调 |
| 按键驱动 | `button_drv.c/h` | 链表管理，短按/长按检测，软件消抖 |
| GPIO 扩展 | `xl9555.c/h` | XL9555 I2C GPIO 扩展芯片驱动，中断回调 |
| LED 驱动 | `led_drv.c/h` | LED 初始化、呼吸灯效果 |
| 六轴传感器 | `esp32_s3_qmi8658.c/h` | QMI8658 加速度计+陀螺仪，倾角解算 |
| 任务管理 | `task_handle.c/h` | FreeRTOS 任务/队列/信号量/事件管理 |
| 主程序 | `main.c` | 系统入口，硬件初始化调度 |

## 硬件

- **主控**：ESP32-S3
- **IMU**：QMI8658（I2C 地址 0x6A，SDA=GPIO1，SCL=GPIO2）
- **GPIO 扩展**：XL9555（I2C 地址 0x20，INT=GPIO17，SDA=GPIO10，SCL=GPIO11）

## 快速开始

```bash
# 激活 ESP-IDF
source ~/esp/esp-idf/export.sh

# 编译
idf.py build

# 烧录（替换 /dev/ttyACM0 为实际串口）
idf.py -p /dev/ttyACM0 flash

# 查看串口输出
idf.py -p /dev/ttyACM0 monitor
```

### 快捷别名（需先 source ~/.bashrc）

```bash
build    # 编译
flash    # 烧录
mon      # 串口输出
bfm      # 编译 + 烧录 + 串口
```

## 项目结构

```
├── main/
│   ├── CMakeLists.txt        # 构建配置
│   ├── main.c                # 主程序入口
│   ├── wifi_manager.c/h      # WiFi 管理
│   ├── button_drv.c/h        # 按键驱动
│   ├── xl9555.c/h            # GPIO 扩展芯片
│   ├── led_drv.c/h           # LED 驱动
│   ├── esp32_s3_qmi8658.c/h  # IMU 六轴传感器
│   ├── task_handle.c/h       # FreeRTOS 任务管理
│   └── platform.h            # 公共头文件
├── CMakeLists.txt
├── sdkconfig
└── README.md
```
