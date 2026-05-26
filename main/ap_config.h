#ifndef _AP_CONFIG_H_
#define _AP_CONFIG_H_

#include "esp_err.h"
#include <stdbool.h>

/* 配网状态 */
typedef enum {
    AP_CONFIG_STATE_WAITING,        // 等用户连手机配网
    AP_CONFIG_STATE_GET_SSID_PWD,   // 用户已提交WiFi信息
    AP_CONFIG_STATE_CONNECTING,
    AP_CONFIG_STATE_SUCCESS,
    AP_CONFIG_STATE_FAIL,
} ap_config_state_t;

/* 回调函数类型 — 配网状态变了就调用这个 */
typedef void(*p_ap_config_callback)(ap_config_state_t state);

/* 启动配网：开热点 + 启网页，等用户手机连接配置 */
void ap_config_start(const char *ap_ssid, const char *ap_pass, p_ap_config_callback cb);

/* 停止配网：关网页，切回STA模式（热点消失） */
void ap_config_stop(void);

/* 检查配网是否完成了 */
bool ap_config_is_done(void);

/* 获取配网得到的WiFi信息 */
const char *ap_config_get_ssid(void);
const char *ap_config_get_password(void);

/* 从NVS读取之前保存的WiFi配置 */
bool ap_config_load_saved(void);

#endif
