#ifndef KEY_DRV_H
#define KEY_DRV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 按键事件类型
typedef enum {
    KEY_PRESS = 0,   // 按下
    KEY_RELEASE = 1, // 释放
} key_event_t;

// 按键 ID
typedef enum {
    KEY_ID_BOOT = 0, // GPIO 0 BOOT 按键
    KEY_ID_MAX,
} key_id_t;

// 按键回调函数类型
typedef void (*key_cb_t)(key_id_t id, key_event_t event);

// 初始化按键驱动
void key_drv_init(void);

// 注册按键回调
void key_drv_register_cb(key_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif // KEY_DRV_H
