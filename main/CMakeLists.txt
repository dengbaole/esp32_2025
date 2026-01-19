# 获取当前目录下的所有 .c 文件
file(GLOB SRCS "wifi_manager.c" "button_drv.c" "xl9555.c" "led_drv.c" "main.c" "task_handle.c" "*.c")

# 注册组件
idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ".")
