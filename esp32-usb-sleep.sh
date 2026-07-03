#!/bin/bash
# 睡前自动断开 ESP32 USB 设备，唤醒后恢复
# 安装: sudo cp esp32-usb-sleep.sh /usr/lib/systemd/system-sleep/esp32-usb.sh && sudo chmod +x /usr/lib/systemd/system-sleep/esp32-usb.sh

case "$1" in
    pre)
        for dev in 5-2.1.3 5-2.1.4; do
            [ -f "/sys/bus/usb/devices/$dev/authorized" ] && echo "0" > "/sys/bus/usb/devices/$dev/authorized"
        done
        ;;
    post)
        for dev in 5-2.1.3 5-2.1.4; do
            [ -f "/sys/bus/usb/devices/$dev/authorized" ] && echo "1" > "/sys/bus/usb/devices/$dev/authorized"
        done
        ;;
esac
