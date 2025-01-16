# 获取当前目录下的所有 .c 文件
file(GLOB SRCS "*.c")

# 注册组件
idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ".")
