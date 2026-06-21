IDF_PATH ?= /home/dengbaole/esp/esp-idf
PORT     ?= /dev/ttyACM0

.PHONY: flash format build monitor

flash:
	cd $(IDF_PATH) && . ./export.sh > /dev/null && cd - > /dev/null && idf.py -p $(PORT) build flash monitor

format:
	clang-format -i main/*.c main/*.h

build:
	cd $(IDF_PATH) && . ./export.sh > /dev/null && cd - > /dev/null && idf.py build

monitor:
	cd $(IDF_PATH) && . ./export.sh > /dev/null && cd - > /dev/null && idf.py -p $(PORT) monitor
