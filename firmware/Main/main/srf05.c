#include "srf05.h"
#include "board_config.h"

#ifdef SRF05_ENABLE

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

// Thời gian pulse TRIG (µs) — datasheet yêu cầu ≥10µs
#define TRIG_PULSE_US    10

// Timeout chờ ECHO (µs) — 30000µs ≈ 515cm, đủ cho mọi trường hợp
#define ECHO_TIMEOUT_US  30000

void srf05_init(void)
{
    // Cấu hình TRIG là output
    gpio_config_t trig_cfg = {
        .pin_bit_mask = (1ULL << SRF05_TRIG_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig_cfg);

    // Cấu hình ECHO là input (pull-down để tránh nhiễu khi chưa đo)
    gpio_config_t echo_cfg = {
        .pin_bit_mask = (1ULL << SRF05_ECHO_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo_cfg);

    gpio_set_level(SRF05_TRIG_GPIO, 0);
}

float srf05_measure_cm(void)
{
    // Gửi trigger pulse: LOW 2µs → HIGH 10µs → LOW
    gpio_set_level(SRF05_TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(SRF05_TRIG_GPIO, 1);
    esp_rom_delay_us(TRIG_PULSE_US);
    gpio_set_level(SRF05_TRIG_GPIO, 0);

    // Chờ ECHO lên HIGH (cảm biến bắt đầu gửi sóng)
    int64_t t_start = esp_timer_get_time();
    while (gpio_get_level(SRF05_ECHO_GPIO) == 0) {
        if ((esp_timer_get_time() - t_start) > ECHO_TIMEOUT_US) {
            return -1.0f;  // timeout: không nhận được echo
        }
    }
    int64_t t_echo_rise = esp_timer_get_time();

    // Chờ ECHO xuống LOW (sóng phản hồi về)
    while (gpio_get_level(SRF05_ECHO_GPIO) == 1) {
        if ((esp_timer_get_time() - t_echo_rise) > ECHO_TIMEOUT_US) {
            return -1.0f;  // timeout: vật quá xa hoặc không có vật
        }
    }
    int64_t t_echo_fall = esp_timer_get_time();

    // Khoảng cách (cm) = thời gian echo (µs) / 58
    // (tốc độ âm ~340m/s, khứ hồi nên chia đôi → 1cm ≈ 58µs)
    float duration_us = (float)(t_echo_fall - t_echo_rise);
    return duration_us / 58.0f;
}

#endif // SRF05_ENABLE
