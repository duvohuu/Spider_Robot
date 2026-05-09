#include "wifi_server.h"
#include "gait.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "cJSON.h"

#include <string.h>
#include <stdio.h>

// ── Config ───────────────────────────────────────────────────
#define AP_SSID      "Spider-Robot"
#define AP_PASS      "spider123"
#define AP_CHANNEL   1
#define AP_MAX_CONN  4

#define WS_MAX_CLIENTS 4

// Giá trị mặc định (mm) — chỉnh theo robot thực tế
#define DEFAULT_HEIGHT_MM  120.0f

static const char *TAG = "ws_server";

// ── WebSocket client tracking ────────────────────────────────
static httpd_handle_t s_server = NULL;
static int  s_ws_fds[WS_MAX_CLIENTS];
static int  s_ws_count = 0;
static SemaphoreHandle_t s_ws_mutex = NULL;

// ── Z height (set từ web app) ────────────────────────────────
static volatile float s_height = DEFAULT_HEIGHT_MM;
static volatile bool  s_gait_running = false;

// ── Pose mode: set_foot ────────────────────────────────────────────
static volatile bool  s_foot_dirty = false;
static volatile int   s_foot_idx   = -1;
static volatile float s_foot_x     = 0.0f;
static volatile float s_foot_y     = 0.0f;
static volatile float s_foot_z     = 0.0f;

float ws_get_height(void)    { return s_height; }
bool  ws_gait_running(void)  { return s_gait_running; }

bool ws_take_foot_cmd(int *out_idx, float *out_x, float *out_y, float *out_z)
{
    if (!s_foot_dirty) return false;
    *out_idx = s_foot_idx;
    *out_x   = s_foot_x;
    *out_y   = s_foot_y;
    *out_z   = s_foot_z;
    s_foot_dirty = false;
    return true;
}

// ── Helpers ──────────────────────────────────────────────────
static void fd_add(int fd)
{
    xSemaphoreTake(s_ws_mutex, portMAX_DELAY);
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (s_ws_fds[i] < 0) { s_ws_fds[i] = fd; s_ws_count++; break; }
    }
    xSemaphoreGive(s_ws_mutex);
}

static void fd_remove(int fd)
{
    xSemaphoreTake(s_ws_mutex, portMAX_DELAY);
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (s_ws_fds[i] == fd) { s_ws_fds[i] = -1; s_ws_count--; break; }
    }
    xSemaphoreGive(s_ws_mutex);
}

int ws_client_count(void) { return s_ws_count; }

// ── Broadcast ────────────────────────────────────────────────
void ws_broadcast(const char *json_str)
{
    if (!s_server || !json_str) return;
    xSemaphoreTake(s_ws_mutex, portMAX_DELAY);
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (s_ws_fds[i] < 0) continue;
        httpd_ws_frame_t frame = {
            .type    = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t *)json_str,
            .len     = strlen(json_str),
        };
        esp_err_t err = httpd_ws_send_frame_async(s_server, s_ws_fds[i], &frame);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "broadcast fd=%d err=%s", s_ws_fds[i], esp_err_to_name(err));
            s_ws_fds[i] = -1;
            s_ws_count--;
        }
    }
    xSemaphoreGive(s_ws_mutex);
}

// ── WS handler ───────────────────────────────────────────────
static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "WS client connected fd=%d", fd);
        fd_add(fd);
        // Gửi ngay state hiện tại để web app sync
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"type\":\"height\",\"z\":%.1f}", s_height);
        ws_broadcast(buf);
        return ESP_OK;
    }

    httpd_ws_frame_t frame = { .type = HTTPD_WS_TYPE_TEXT };
    uint8_t buf[256] = {0};
    frame.payload = buf;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1);
    if (ret != ESP_OK) {
        fd_remove(httpd_req_to_sockfd(req));
        return ret;
    }

    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        fd_remove(httpd_req_to_sockfd(req));
        return ESP_OK;
    }

    if (frame.type == HTTPD_WS_TYPE_TEXT && frame.len > 0) {
        buf[frame.len] = '\0';
        ESP_LOGI(TAG, "rx: %s", buf);

        cJSON *root = cJSON_Parse((char *)buf);
        if (root) {
            cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
            if (cJSON_IsString(cmd)) {
                if (strcmp(cmd->valuestring, "ping") == 0) {
                    ws_broadcast("{\"type\":\"pong\"}");

                } else if (strcmp(cmd->valuestring, "start") == 0) {
                    s_gait_running = true;
                    ESP_LOGI(TAG, "gait START");
                    ws_broadcast("{\"type\":\"state\",\"state\":\"running\"}");

                } else if (strcmp(cmd->valuestring, "stop") == 0) {
                    s_gait_running = false;
                    ESP_LOGI(TAG, "gait STOP");
                    ws_broadcast("{\"type\":\"state\",\"state\":\"idle\"}");

                } else if (strcmp(cmd->valuestring, "set_height") == 0) {
                    cJSON *jz = cJSON_GetObjectItem(root, "z");
                    if (cJSON_IsNumber(jz)) {
                        s_height = (float)jz->valuedouble;
                        ESP_LOGI(TAG, "set_height z=%.1f mm", s_height);
                        // Echo lại để web app xác nhận
                        char ack[64];
                        snprintf(ack, sizeof(ack), "{\"type\":\"height\",\"z\":%.1f}", s_height);
                        ws_broadcast(ack);
                    }

                } else if (strcmp(cmd->valuestring, "set_foot") == 0) {
                    cJSON *jleg = cJSON_GetObjectItem(root, "leg");
                    cJSON *jx   = cJSON_GetObjectItem(root, "x");
                    cJSON *jy   = cJSON_GetObjectItem(root, "y");
                    cJSON *jz   = cJSON_GetObjectItem(root, "z");
                    if (cJSON_IsString(jleg) && cJSON_IsNumber(jx) && cJSON_IsNumber(jy) && cJSON_IsNumber(jz)) {
                        const char *leg = jleg->valuestring;
                        int idx = -1;
                        if      (strcmp(leg, "LF") == 0) idx = 0;
                        else if (strcmp(leg, "LM") == 0) idx = 1;
                        else if (strcmp(leg, "LB") == 0) idx = 2;
                        else if (strcmp(leg, "RF") == 0) idx = 3;
                        else if (strcmp(leg, "RM") == 0) idx = 4;
                        else if (strcmp(leg, "RB") == 0) idx = 5;
                        if (idx >= 0) {
                            s_foot_x   = (float)jx->valuedouble;
                            s_foot_y   = (float)jy->valuedouble;
                            s_foot_z   = (float)jz->valuedouble;
                            s_foot_idx = idx;
                            s_foot_dirty = true;
                            ESP_LOGI(TAG, "set_foot %s idx=%d (%.1f, %.1f, %.1f)",
                                     leg, idx, s_foot_x, s_foot_y, s_foot_z);
                        }
                    }

                } else if (strcmp(cmd->valuestring, "set_offset") == 0) {
                    cJSON *jleg = cJSON_GetObjectItem(root, "leg");
                    cJSON *jcx  = cJSON_GetObjectItem(root, "cx");
                    cJSON *jfm  = cJSON_GetObjectItem(root, "fm");
                    cJSON *jtb  = cJSON_GetObjectItem(root, "tb");
                    if (cJSON_IsString(jleg) && cJSON_IsNumber(jcx) && cJSON_IsNumber(jfm) && cJSON_IsNumber(jtb)) {
                        const char *leg = jleg->valuestring;
                        int idx = -1;
                        if      (strcmp(leg, "LF") == 0) idx = 0;
                        else if (strcmp(leg, "LM") == 0) idx = 1;
                        else if (strcmp(leg, "LB") == 0) idx = 2;
                        else if (strcmp(leg, "RF") == 0) idx = 3;
                        else if (strcmp(leg, "RM") == 0) idx = 4;
                        else if (strcmp(leg, "RB") == 0) idx = 5;
                        if (idx >= 0) {
                            gait_set_offset(idx,
                                (float)jcx->valuedouble,
                                (float)jfm->valuedouble,
                                (float)jtb->valuedouble);
                            ESP_LOGI(TAG, "set_offset %s cx=%.1f fm=%.1f tb=%.1f",
                                     leg, (float)jcx->valuedouble,
                                     (float)jfm->valuedouble, (float)jtb->valuedouble);
                        }
                    }

                } else if (strcmp(cmd->valuestring, "set_gait") == 0) {
                    cJSON *jstep  = cJSON_GetObjectItem(root, "stepX");
                    cJSON *jlift  = cJSON_GetObjectItem(root, "liftZ");
                    cJSON *jtlift = cJSON_GetObjectItem(root, "tLiftMs");
                    cJSON *jtswing = cJSON_GetObjectItem(root, "tSwingMs");
                    cJSON *jtdown  = cJSON_GetObjectItem(root, "tDownMs");
                    cJSON *jtrimy  = cJSON_GetObjectItem(root, "trimY");
                    float step  = cJSON_IsNumber(jstep)   ? (float)jstep->valuedouble   : 0.0f;
                    float lift  = cJSON_IsNumber(jlift)   ? (float)jlift->valuedouble   : 0.0f;
                    int tlift   = cJSON_IsNumber(jtlift)  ? (int)jtlift->valuedouble    : 0;
                    int tswing  = cJSON_IsNumber(jtswing) ? (int)jtswing->valuedouble   : 0;
                    int tdown   = cJSON_IsNumber(jtdown)  ? (int)jtdown->valuedouble    : 0;
                    float trimy = cJSON_IsNumber(jtrimy)  ? (float)jtrimy->valuedouble  : 0.0f;
                    gait_set_params(step, lift, tlift, tswing, tdown, trimy);
                    ESP_LOGI(TAG, "set_gait stepX=%.1f liftZ=%.1f tLift=%d tSwing=%d tDown=%d trimY=%.1f",
                             step, lift, tlift, tswing, tdown, trimy);

                } // else if set_offset
            } // if cJSON_IsString(cmd)
            cJSON_Delete(root);
        } // if root
    } // if TEXT
    return ESP_OK;
}

static const httpd_uri_t ws_uri = {
    .uri          = "/ws",
    .method       = HTTP_GET,
    .handler      = ws_handler,
    .is_websocket = true,
    .handle_ws_control_frames = true,
};

// ── Start HTTP+WS server ─────────────────────────────────────
static void start_webserver(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port      = 80;
    cfg.max_open_sockets = WS_MAX_CLIENTS + 3;

    if (httpd_start(&s_server, &cfg) == ESP_OK) {
        httpd_register_uri_handler(s_server, &ws_uri);
        ESP_LOGI(TAG, "WS server started at ws://192.168.4.1/ws");
    }
}

// ── WiFi SoftAP ──────────────────────────────────────────────
static void wifi_init_softap(void)
{
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid            = AP_SSID,
            .ssid_len        = strlen(AP_SSID),
            .channel         = AP_CHANNEL,
            .password        = AP_PASS,
            .max_connection  = AP_MAX_CONN,
            .authmode        = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP started  SSID=%s  IP=192.168.4.1", AP_SSID);
}

// ── Public init ──────────────────────────────────────────────
void wifi_server_init(void)
{
    s_ws_mutex = xSemaphoreCreateMutex();
    for (int i = 0; i < WS_MAX_CLIENTS; i++) s_ws_fds[i] = -1;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_softap();
    start_webserver();
}
