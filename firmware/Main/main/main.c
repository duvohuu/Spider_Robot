#include "pca9685.h"
#include "gait.h"
#include "wifi_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "main";

void app_main(void)
{
    // ── Hardware init ────────────────────────────────────────
    i2c_master_init();
    pca_init(PCA_L);
    pca_init(PCA_R);

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
        }

        // ── Đang chạy: thực hiện 1 bước tripod ──────────────
        if (running) {
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