#pragma once

// ============================================================
//  Board-level hardware configuration
// ============================================================
#define I2C_MASTER_SDA  21
#define I2C_MASTER_SCL  22
#define I2C_MASTER_FREQ 400000

// ============================================================
//  HY-SRF05 Ultrasonic Sensor — obstacle avoidance
//  Comment out SRF05_ENABLE để tắt toàn bộ tính năng này
// ============================================================
#define SRF05_ENABLE
#define SRF05_TRIG_GPIO  18   // GPIO kết nối chân TRIG
#define SRF05_ECHO_GPIO  19   // GPIO kết nối chân ECHO (qua voltage divider 5V→3.3V)
