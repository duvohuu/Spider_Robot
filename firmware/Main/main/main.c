#include "pca9685.h"
#include "gait.h"
#include "wifi_server.h"
#include "srf05.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>
#include <stdio.h>

static const char *TAG = "main";

// ── Obstacle avoidance state ──────────────────────────────────
// Toàn bộ block này biến mất khi comment out SRF05_ENABLE
#ifdef SRF05_ENABLE
typedef enum { OBS_NORMAL, OBS_TURNING } ObsState;
static ObsState s_obs_state = OBS_NORMAL;
static int      s_obs_turns = 0;

// Ngưỡng khoảng cách — chỉnh tại đây nếu cần
#define OBS_STOP_CM     12.0f   // dừng tiến, bắt đầu quay
#define OBS_WARN_CM     25.0f   // bắt đầu lệch nhẹ
#define OBS_WARN_TRIM    4.0f   // độ lệch nhẹ (mm): bẻ phải nhẹ
#define OBS_TURN_TRIM   18.0f   // độ lệch mạnh khi quay tại chỗ
#define OBS_TURN_STEPS   6      // số tripod_step để quay ~90°
#endif

void app_main(void)
{
    // ── Hardware init ────────────────────────────────────────
    i2c_master_init();
    pca_init(PCA_L);
    pca_init(PCA_R);
#ifdef SRF05_ENABLE
    srf05_init();
    ESP_LOGI(TAG, "SRF05 obstacle avoidance enabled (stop<%.0fcm, warn<%.0fcm)",
             (double)OBS_STOP_CM, (double)OBS_WARN_CM);
#endif

    // ── WiFi SoftAP + WebSocket server ───────────────────────
    wifi_server_init();

    // ── Tư thế đứng ban đầu ──────────────────────────────────
    calibration();

    ESP_LOGI(TAG, "Ready — connect WiFi [Spider-Robot / spider123]");
    ESP_LOGI(TAG, "Open: ws://192.168.4.1/ws");
    ESP_LOGI(TAG, "Send: {\"cmd\":\"set_height\",\"z\":<mm>}");

    // ── Main loop: chỉnh Z + chạy gait ──────────────────────
    float last_z = ws_get_height();
    bool  was_running = false;

    while (1) {
        bool running = ws_gait_running();

        // ── Chuyển từ idle → running ─────────────────────────
        if (running && !was_running) {
            tripod_init();
            was_running = true;
        }

        // ── Chuyển từ running → idle ─────────────────────────
        if (!running && was_running) {
            gait_set_height(ws_get_height());   // về tư thế đứng
            was_running = false;
#ifdef SRF05_ENABLE
            // Reset trạng thái obstacle avoidance khi dừng gait
            s_obs_state = OBS_NORMAL;
            s_obs_turns = 0;
            gait_set_params(0, 0, 0, 0, 0, 0.0f);  // xóa trim
#endif
        }

        // ── Đang chạy: thực hiện 1 bước tripod ──────────────
        if (running) {
#ifdef SRF05_ENABLE
            // ── Đo khoảng cách & phát lên web ────────────────
            float dist = srf05_measure_cm();
            if (dist > 0.0f) {
                char dbuf[48];
                snprintf(dbuf, sizeof(dbuf), "{\"dist_cm\":%.1f}", dist);
                ws_broadcast(dbuf);
            }

            // ── State machine tránh vật cản ──────────────────
            if (s_obs_state == OBS_NORMAL) {
                if (dist < 0.0f) {
                    // Sensor lỗi/timeout → reset trim, tiếp tục đi thẳng
                    gait_set_params(0, 0, 0, 0, 0, 0.0f);
                } else if (dist < OBS_STOP_CM) {
                    // Sát vật cản → dừng tiến, chuyển sang quay
                    s_obs_state = OBS_TURNING;
                    s_obs_turns = OBS_TURN_STEPS;
                    gait_set_params(0, 0, 0, 0, 0, OBS_TURN_TRIM);
                    ESP_LOGI(TAG, "OBS: %.1f cm — quay tránh", dist);
                } else if (dist < OBS_WARN_CM) {
                    // Gần vật cản → lệch nhẹ sang phải
                    gait_set_params(0, 0, 0, 0, 0, OBS_WARN_TRIM);
                } else {
                    // Đường thông → đi thẳng
                    gait_set_params(0, 0, 0, 0, 0, 0.0f);
                }
            }

            if (s_obs_state == OBS_TURNING) {
                tripod_step();
                if (--s_obs_turns <= 0) {
                    s_obs_state = OBS_NORMAL;
                    gait_set_params(0, 0, 0, 0, 0, 0.0f);
                    ESP_LOGI(TAG, "OBS: xong quay, tiếp tục");
                }
                continue;
            }
#endif
            tripod_step();
            continue;   // không delay thêm — tripod_step tự delay
        }

        // ── Đang đứng: theo dõi Z từ web ────────────────────
        float z = ws_get_height();
        if (fabsf(z - last_z) > 0.5f) {
            gait_set_height(z);
            ESP_LOGI(TAG, "height → %.1f mm", z);
            last_z = z;
        }

        // ── Pose mode: điều khiển từng chân trực tiếp ─────────────
        int fidx; float fx, fy, fz;
        if (ws_take_foot_cmd(&fidx, &fx, &fy, &fz)) {
            gait_set_foot_direct(fidx, fx, fy, fz);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}