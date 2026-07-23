#ifndef CAMERA_DRV_H
#define CAMERA_DRV_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 初始化摄像头（GC0308，DVP 并口）
void camera_drv_init(void);

// 开始摄像头预览（帧数据通过回调输出）
// on_frame: 每收到一帧回调，参数是 RGB565 数据指针 + 宽高
typedef void (*camera_frame_cb_t)(const uint16_t *buf, int w, int h);
void camera_drv_start(camera_frame_cb_t on_frame);

// 停止预览
void camera_drv_stop(void);

#ifdef __cplusplus
}
#endif

#endif
