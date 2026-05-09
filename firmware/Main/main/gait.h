#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct { float x, y, z; } FootPos;

// Thứ tự: 0=LF, 1=LM, 2=LB, 3=RF, 4=RM, 5=RB
void gait_get_foot_positions(FootPos out[6]);

void gait_set_running(bool running);  // gọi từ wifi_server khi nhận start/stop
bool gait_is_running(void);

void calibration(void);
void tripod_init(void);
void tripod_step(void);
void gait_set_height(float z);  // cập nhật chiều cao, di chân nếu đang đứng

// Pose mode: điều khiển trực tiếp 1 chân (body frame), gait phải đang dừng
// leg_idx: 0=LF, 1=LM, 2=LB, 3=RF, 4=RM, 5=RB
void gait_set_foot_direct(int leg_idx, float x, float y, float z);

// Cập nhật offset servo từ web app (leg_idx: 0=LF..5=RB)
void gait_set_offset(int leg_idx, float cx, float fm, float tb);

// Cập nhật tham số gait từ web app (pass 0 để giữ nguyên giá trị)
// trimY > 0 → bẻ phải (sửa lệch trái); trimY < 0 → bẻ trái
void gait_set_params(float step_x, float lift_z, int t_lift, int t_swing, int t_down, float trim_y);

// Điều chỉnh bước đi riêng từng chân (scale: 0.1 – 3.0, mặc định 1.0)


#ifdef CONFIG_DEBUG
void test_tibia_mechanical(void);
void test_femur(void);
#endif