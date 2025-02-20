# 获取当前目录下的所有 .c 文件
file(GLOB SRCS "xl9555_iic_drv.c" "iic_drv.c" "main.c" "sd_drv.c" "task_handle.c" "*.c")

# 注册组件
idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ".")
