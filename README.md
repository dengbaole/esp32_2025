# ESP32-S3 Wi-Fi 配网示例

这是一个基于 ESP-IDF 的 ESP32-S3 工程，目前包含 Wi-Fi 网页配网、NVS 保存 Wi-Fi 信息、XL9555 IO 扩展按键扫描，以及预留的 QMI8658 姿态传感器代码。

## 当前功能

- 上电初始化 NVS 和 Wi-Fi。
- 如果 NVS 中已有 Wi-Fi 配置，直接连接保存的路由器。
- 如果没有保存配置，开启 `ESP32_Setup` 热点，密码为 `12345678`。
- 手机连接热点后访问 `http://192.168.4.1`，提交 SSID 和密码。
- 配网信息保存到 NVS，设备切换到 STA 连接目标 Wi-Fi。
- Wi-Fi 连接成功后关闭 AP 热点和网页服务器。
- 通过 XL9555 扩展 IO 读取按键状态，支持短按和长按回调。

## 主要文件

```text
main/
├── main.c                 # 应用入口，负责初始化和主流程编排
├── ap_config.c/.h         # AP 热点配网和网页服务器
├── wifi_manager.c/.h      # Wi-Fi 初始化、连接、重连和状态回调
├── xl9555.c/.h            # XL9555 IO 扩展芯片驱动
├── button_drv.c/.h        # 按键扫描、消抖、短按和长按
├── esp32_s3_qmi8658.c/.h  # QMI8658 姿态传感器驱动，当前未在主流程启用
└── task_handle.c/.h       # 任务示例代码，当前默认未启用
```

## 硬件连接

当前主流程中使用的引脚如下：

| 模块 | 功能 | GPIO |
| --- | --- | --- |
| XL9555 | SDA | GPIO10 |
| XL9555 | SCL | GPIO11 |
| XL9555 | INT | GPIO17 |
| QMI8658 | SDA | GPIO1 |
| QMI8658 | SCL | GPIO2 |

XL9555 默认把 `IO0_1` 到 `IO0_4` 注册为按键输入，低电平为按下。

## 构建和烧录

先确保已经进入 ESP-IDF 环境，并且终端中能正常运行 `python` 和 `idf.py`。

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

把 `COMx` 换成实际串口号。

## 配网流程

1. 首次启动时，设备会开启热点 `ESP32_Setup`。
2. 使用手机或电脑连接该热点，密码 `12345678`。
3. 浏览器访问 `http://192.168.4.1`。
4. 输入目标 Wi-Fi 的 SSID 和密码。
5. 提交后设备保存配置并尝试连接目标 Wi-Fi。
6. 连接成功后，热点会自动关闭。

## 注意事项

- 配网页提交内容使用 `application/x-www-form-urlencoded` 解析，支持空格、中文和常见特殊字符。
- 密码可以留空，用于连接开放 Wi-Fi。
- 如果保存了错误的 Wi-Fi 信息，可以擦除 NVS 或在代码中增加清除配置入口。
- QMI8658 和 `task_handle.c` 中的任务示例目前没有在 `app_main()` 中启用，需要时可调用 `task_init()` 或按实际业务接入。
