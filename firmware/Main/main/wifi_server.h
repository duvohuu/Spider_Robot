#pragma once
#include <stdbool.h>

// ============================================================
//  WiFi SoftAP + WebSocket server
//
//  ESP32 tạo WiFi hotspot riêng:
//    SSID    : Spider-Robot
//    Password: spider123
//    IP      : 192.168.4.1
//
//  Web App kết nối: ws://192.168.4.1/ws
//  Lệnh nhận:  { "cmd": "set_height", "z": <mm> }
//              { "cmd": "set_foot", "leg": "LF", "x": <mm>, "y": <mm>, "z": <mm> }
//              { "cmd": "start" }
//              { "cmd": "stop" }
//              { "cmd": "ping" }
// ============================================================

// Khởi tạo NVS, WiFi SoftAP, HTTP+WS server
void wifi_server_init(void);

// Gửi JSON string tới tất cả WS client đang kết nối
void ws_broadcast(const char *json_str);

// Trả về số client WebSocket đang kết nối
int ws_client_count(void);

// Trả về chiều cao Z hiện tại (mm), được set từ web app
float ws_get_height(void);

// Pose mode: lấy lệnh set_foot nếu có (xóa cờ sau khi lấy)
// Trả về true nếu có lệnh mới; out_idx: 0=LF..5=RB
bool ws_take_foot_cmd(int *out_idx, float *out_x, float *out_y, float *out_z);

// Trả về trạng thái gait (start/stop từ web app)
bool  ws_gait_running(void);
