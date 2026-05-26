/**
 * @file ap_config.c
 * @brief AP 配网模块 — 让ESP32开热点，用户手机输WiFi密码
 *
 * 关键流程：
 *   ap_config_start()  → 开热点 + 启动网页服务器
 *                       手机连上热点 → 浏览器打开 192.168.4.1
 *                       → 用户填SSID/密码提交
 *   handle_post_config() → 保存到NVS → 回调通知main.c去连接
 *   ap_config_stop()    → 连上WiFi后关掉热点
 */

#include "ap_config.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "cJSON.h"

#define TAG "ap_config"

#define NVS_NAMESPACE   "ap_cfg"
#define NVS_KEY_SSID    "ssid"
#define NVS_KEY_PASS    "password"

static httpd_handle_t       s_server     = NULL;
static p_ap_config_callback s_cb         = NULL;
static char                 s_received_ssid[32] = {0};
static char                 s_received_pass[64] = {0};
static bool                 s_config_done       = false;

/* 配网页面的HTML，存在flash里省内存 */
static const char *s_index_html = ""
"<!DOCTYPE html>"
"<html><head>"
"<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>ESP32 Config</title>"
"<style>"
"body{font-family:-apple-system,sans-serif;background:#0f172a;color:#e2e8f0;"
"display:flex;min-height:100vh;align-items:center;justify-content:center;margin:0;padding:16px}"
".card{background:#1e293b;width:100%;max-width:360px;padding:28px;border-radius:16px}"
"h1{font-size:20px;margin:0 0 4px;color:#f1f5f9}"
"p{font-size:13px;color:#94a3b8;margin:0 0 20px}"
"label{display:block;font-size:12px;font-weight:600;color:#94a3b8;margin:12px 0 4px}"
"input{width:100%;padding:10px 12px;border:1px solid #334155;background:#0f172a;"
"color:#e2e8f0;border-radius:8px;font-size:15px;box-sizing:border-box}"
"button{width:100%;padding:12px;background:#3b82f6;color:#fff;border:none;"
"border-radius:8px;font-size:15px;font-weight:600;cursor:pointer;margin-top:16px}"
"</style>"
"</head><body>"
"<div class='card'>"
"<h1>Wi-Fi Setup</h1>"
"<p>Enter your Wi-Fi credentials</p>"
"<form action='/api/config' method='POST'>"
"<label>SSID</label>"
"<input type='text' name='ssid' placeholder='Wi-Fi name' required>"
"<label>Password</label>"
"<input type='password' name='password' placeholder='Password (leave blank if open)'>"
"<button type='submit'>Save & Connect</button>"
"</form>"
"</div>"
"</body></html>";


/* ================================================================
 * HTTP 请求处理
 * ================================================================ */

/* GET /  → 返回配网页 */
static esp_err_t handle_get_root(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_sendstr(req, s_index_html);
    return ESP_OK;
}

/* POST /api/config  → 接收用户提交的WiFi信息 */
static esp_err_t handle_post_config(httpd_req_t *req)
{
    /* 接收浏览器发来的数据，格式：ssid=xxx&password=*** */
    char buf[256] = {0};
    int ret, remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Too large");
        return ESP_FAIL;
    }
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Read failed");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    /* 解析出ssid和password */
    char ssid_val[64] = {0};
    char pass_val[128] = {0};
    char *p = strstr(buf, "ssid=");
    if (!p) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid");
        return ESP_FAIL;
    }
    p += 5;  // 跳过 "ssid="
    char *amp = strchr(p, '&');
    if (amp) strncpy(ssid_val, p, amp - p);
    else     strncpy(ssid_val, p, sizeof(ssid_val) - 1);

    p = strstr(buf, "password=");
    if (p) {
        p += 9;  // 跳过 "password="
        strncpy(pass_val, p, sizeof(pass_val) - 1);
    }

    // URL解码：浏览器把空格变成+号，要还原回来
    for (char *q = ssid_val; *q; q++) if (*q == '+') *q = ' ';
    for (char *q = pass_val; *q; q++) if (*q == '+') *q = ' ';

    strncpy(s_received_ssid, ssid_val, 31);
    s_received_ssid[31] = '\0';
    strncpy(s_received_pass, pass_val, 63);
    s_received_pass[63] = '\0';

    /* 保存到NVS（断电不丢） */
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, NVS_KEY_SSID, s_received_ssid);
        nvs_set_str(nvs, NVS_KEY_PASS, s_received_pass);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "Saved! SSID=%s", s_received_ssid);
    }

    /* 通知main.c：收到WiFi信息了，去连接吧 */
    s_config_done = true;
    if (s_cb) s_cb(AP_CONFIG_STATE_GET_SSID_PWD);

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}


/* ================================================================
 * HTTP 服务器
 * ================================================================ */

static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 4;
    config.stack_size = 4096;

    if (httpd_start(&s_server, &config) == ESP_OK) {
        httpd_register_uri_handler(s_server, &(httpd_uri_t){
            .uri = "/", .method = HTTP_GET, .handler = handle_get_root
        });
        httpd_register_uri_handler(s_server, &(httpd_uri_t){
            .uri = "/api/config", .method = HTTP_POST, .handler = handle_post_config
        });
        ESP_LOGI(TAG, "HTTP Server ready");
    }
}

static void stop_webserver(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
}


/* ================================================================
 * 对外接口
 * ================================================================ */

/* 启动配网：开热点 + 启网页服务器 */
void ap_config_start(const char *ap_ssid, const char *ap_pass, p_ap_config_callback cb)
{
    if (s_server) {
        ESP_LOGW(TAG, "Already running");
        return;
    }

    s_cb = cb;
    s_config_done = false;
    memset(s_received_ssid, 0, sizeof(s_received_ssid));
    memset(s_received_pass, 0, sizeof(s_received_pass));

    /* 创建AP网卡（必须有，否则手机连上后拿不到IP，卡在"获取IP地址"） */
    esp_netif_create_default_wifi_ap();

    /* 配置热点 */
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 0,
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 4,
            .beacon_interval = 100,
        },
    };
    if (ap_ssid) {
        strncpy((char *)wifi_config.ap.ssid, ap_ssid, 31);
    } else {
        strncpy((char *)wifi_config.ap.ssid, "ESP32_Config", 31);
    }
    if (ap_pass && strlen(ap_pass) >= 8) {
        strncpy((char *)wifi_config.ap.password, ap_pass, 63);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    /* 切到AP+STA模式（既开热点又保留了连WiFi的能力），配置AP */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    start_webserver();

    ESP_LOGI(TAG, "============================");
    ESP_LOGI(TAG, " AP Config: SSID=%s", wifi_config.ap.ssid);
    ESP_LOGI(TAG, " Visit http://192.168.4.1");
    ESP_LOGI(TAG, "============================");

    if (s_cb) s_cb(AP_CONFIG_STATE_WAITING);
}

/* 停止配网：关网页、关热点 */
void ap_config_stop(void)
{
    stop_webserver();
    esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_LOGI(TAG, "AP stopped");
}

bool ap_config_is_done(void)
{
    return s_config_done;
}

const char *ap_config_get_ssid(void)
{
    return s_received_ssid;
}

const char *ap_config_get_password(void)
{
    return s_received_pass;
}

/* 从NVS读取之前保存的WiFi配置 */
bool ap_config_load_saved(void)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK)
        return false;

    size_t len = sizeof(s_received_ssid);
    esp_err_t ret = nvs_get_str(nvs, NVS_KEY_SSID, s_received_ssid, &len);
    if (ret != ESP_OK) {
        nvs_close(nvs);
        return false;
    }

    len = sizeof(s_received_pass);
    ret = nvs_get_str(nvs, NVS_KEY_PASS, s_received_pass, &len);
    nvs_close(nvs);

    if (ret == ESP_OK && strlen(s_received_ssid) > 0) {
        s_config_done = true;
        ESP_LOGI(TAG, "Loaded: SSID=%s", s_received_ssid);
        return true;
    }
    return false;
}
