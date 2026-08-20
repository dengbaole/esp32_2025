# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# LVGL 相关组件为本地拷贝，不通过组件仓库联网拉取
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_lvgl_port
    ${CMAKE_SOURCE_DIR}/managed_components/lvgl__lvgl
    ${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_lcd_touch
    ${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_lcd_touch_ft5x06
)
project(main)
