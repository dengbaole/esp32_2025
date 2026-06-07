#include "ap_config.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define TAG "ap_config"

#define NVS_NAMESPACE   "ap_cfg"
#define NVS_KEY_SSID    "ssid"
#define NVS_KEY_PASS    "password"

#define FORM_BODY_MAX_LEN   256
#define SSID_BUF_LEN        33
#define PASS_BUF_LEN        65

static httpd_handle_t       s_server = NULL;
static esp_netif_t         *s_ap_netif = NULL;
static p_ap_config_callback s_cb = NULL;
static char                 s_received_ssid[SSID_BUF_LEN] = {0};
static char                 s_received_pass[PASS_BUF_LEN] = {0};
static bool                 s_config_done = false;

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

static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void url_decode_component(char *dst, size_t dst_len, const char *src, size_t src_len)
{
    size_t di = 0;

    if (dst_len == 0) return;

    for (size_t si = 0; si < src_len && di < dst_len - 1; si++) {
        if (src[si] == '+') {
            dst[di++] = ' ';
        } else if (src[si] == '%' && si + 2 < src_len) {
            int hi = hex_to_int(src[si + 1]);
            int lo = hex_to_int(src[si + 2]);
            if (hi >= 0 && lo >= 0) {
                dst[di++] = (char)((hi << 4) | lo);
                si += 2;
            } else {
                dst[di++] = src[si];
            }
        } else {
            dst[di++] = src[si];
        }
    }

    dst[di] = '\0';
}

static bool get_form_value(const char *body, const char *key, char *out, size_t out_len)
{
    const size_t key_len = strlen(key);
    const char *field = body;

    while (field && *field) {
        const char *next = strchr(field, '&');
        const char *eq = strchr(field, '=');
        size_t field_len = next ? (size_t)(next - field) : strlen(field);

        if (eq && eq < field + field_len &&
            (size_t)(eq - field) == key_len &&
            strncmp(field, key, key_len) == 0) {
            const char *value = eq + 1;
            url_decode_component(out, out_len, value, field_len - (size_t)(value - field));
            return true;
        }

        field = next ? next + 1 : NULL;
    }

    if (out_len > 0) out[0] = '\0';
    return false;
}

static esp_err_t recv_request_body(httpd_req_t *req, char *buf, size_t buf_len)
{
    int received = 0;
    int remaining = req->content_len;

    if (remaining <= 0 || remaining >= (int)buf_len) {
        return ESP_ERR_INVALID_SIZE;
    }

    while (remaining > 0) {
        int ret = httpd_req_recv(req, buf + received, remaining);
        if (ret <= 0) {
            return ESP_FAIL;
        }
        received += ret;
        remaining -= ret;
    }

    buf[received] = '\0';
    return ESP_OK;
}

static esp_err_t handle_get_root(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_sendstr(req, s_index_html);
    return ESP_OK;
}

static esp_err_t handle_post_config(httpd_req_t *req)
{
    char buf[FORM_BODY_MAX_LEN] = {0};
    char ssid_val[SSID_BUF_LEN] = {0};
    char pass_val[PASS_BUF_LEN] = {0};

    if (recv_request_body(req, buf, sizeof(buf)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request body");
        return ESP_FAIL;
    }

    if (!get_form_value(buf, "ssid", ssid_val, sizeof(ssid_val)) || strlen(ssid_val) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid");
        return ESP_FAIL;
    }
    get_form_value(buf, "password", pass_val, sizeof(pass_val));

    strlcpy(s_received_ssid, ssid_val, sizeof(s_received_ssid));
    strlcpy(s_received_pass, pass_val, sizeof(s_received_pass));

    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, NVS_KEY_SSID, s_received_ssid);
        nvs_set_str(nvs, NVS_KEY_PASS, s_received_pass);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "Saved! SSID=%s", s_received_ssid);
    }

    s_config_done = true;
    if (s_cb) s_cb(AP_CONFIG_STATE_GET_SSID_PWD);

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}

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

    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 0,
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 4,
            .beacon_interval = 100,
        },
    };

    strlcpy((char *)wifi_config.ap.ssid, ap_ssid ? ap_ssid : "ESP32_Config", sizeof(wifi_config.ap.ssid));
    if (ap_pass && strlen(ap_pass) >= 8) {
        strlcpy((char *)wifi_config.ap.password, ap_pass, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    start_webserver();

    ESP_LOGI(TAG, "============================");
    ESP_LOGI(TAG, " AP Config: SSID=%s", wifi_config.ap.ssid);
    ESP_LOGI(TAG, " Visit http://192.168.4.1");
    ESP_LOGI(TAG, "============================");

    if (s_cb) s_cb(AP_CONFIG_STATE_WAITING);
}

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
