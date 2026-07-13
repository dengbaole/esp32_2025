# 获取当前目录下的所有 .c 文件
file(GLOB SRCS "pca9557_drv.c" "lcd_drv.c" "speaker_drv.c" "audio_drv.c" "sd_drv.c" "imu_drv.c" "key_drv.c" "platform.c" "main.c" "task_handle.c" "*.c")

# 注册组件
idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ".")
