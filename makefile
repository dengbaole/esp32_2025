SHELL := /bin/bash
IDF_PATH := $(HOME)/esp/esp-idf

# 烧录端口：USB-JTAG（能自动进下载模式）
FLASH_PORT ?= $(shell test -e /dev/ttyACM0 && echo /dev/ttyACM0 || echo /dev/ttyUSB0)
# 监视端口：UART（printf 从这出，更稳定）
MON_PORT ?= $(shell test -e /dev/ttyUSB0 && echo /dev/ttyUSB0 || echo /dev/ttyACM0)
BAUD ?= 115200

.PHONY: flash flash_only monitor build clean menuconfig erase

.ONESHELL:

# 一键编译 + 烧录(USB-JTAG) + 串口监视(UART)
flash:
	source $(IDF_PATH)/export.sh
	idf.py -p $(FLASH_PORT) build flash
	idf.py -p $(MON_PORT) -b $(BAUD) monitor

# 仅编译
build:
	source $(IDF_PATH)/export.sh
	idf.py build

# 仅烧录（不重新编译）
flash_only:
	source $(IDF_PATH)/export.sh
	idf.py -p $(FLASH_PORT) flash

# 仅串口监视
monitor:
	source $(IDF_PATH)/export.sh
	idf.py -p $(MON_PORT) -b $(BAUD) monitor

# 清理
clean:
	source $(IDF_PATH)/export.sh
	idf.py fullclean

# 配置界面
menuconfig:
	source $(IDF_PATH)/export.sh
	idf.py menuconfig

# 擦除整个 Flash
erase:
	source $(IDF_PATH)/export.sh
	idf.py -p $(FLASH_PORT) erase_flash
