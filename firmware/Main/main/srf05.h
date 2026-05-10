#pragma once
#include "board_config.h"

#ifdef SRF05_ENABLE

// ============================================================
//  HY-SRF05 Ultrasonic Distance Sensor Driver
//
//  Kết nối:
//    VCC  → 5V
//    GND  → GND
//    TRIG → SRF05_TRIG_GPIO  (3.3V output OK)
//    ECHO → SRF05_ECHO_GPIO  qua voltage divider (5V→3.3V)
//           Ví dụ: 1kΩ nối tiếp + 2kΩ xuống GND
//
//  Để xóa tính năng này: comment out #define SRF05_ENABLE
//  trong board_config.h — không cần sửa file khác.
// ============================================================

// Khởi tạo GPIO TRIG và ECHO
void srf05_init(void);

// Đo khoảng cách (cm).
// Trả về giá trị > 0 nếu thành công.
// Trả về -1.0f nếu timeout (không có vật, >450cm) hoặc lỗi.
float srf05_measure_cm(void);

#endif // SRF05_ENABLE
