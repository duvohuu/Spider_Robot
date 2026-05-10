#pragma once
#include "utils.h"

// ============================================================
//  PCA9685 Channel Map
// ============================================================
#define LF_COXA_CH   13
#define LF_FEMUR_CH  15
#define LF_TIBIA_CH  14

#define LM_COXA_CH   12
#define LM_FEMUR_CH  11
#define LM_TIBIA_CH  10

#define LB_COXA_CH    7
#define LB_FEMUR_CH   9
#define LB_TIBIA_CH   8

#define RF_COXA_CH   10
#define RF_FEMUR_CH   7
#define RF_TIBIA_CH   8

#define RM_COXA_CH   11
#define RM_FEMUR_CH  12
#define RM_TIBIA_CH   9

#define RB_COXA_CH   15
#define RB_FEMUR_CH  13
#define RB_TIBIA_CH  14

// ============================================================
//  Góc gắn chân trên thân (mount angle, độ)
//  Đo từ trục X thân robot (phía trước), ngược chiều kim đồng hồ = dương
//  Điều chỉnh theo thực tế robot
// ============================================================
#define LF_MOUNT_DEG   45.0f
#define LM_MOUNT_DEG   90.0f
#define LB_MOUNT_DEG  135.0f
#define RF_MOUNT_DEG  -45.0f
#define RM_MOUNT_DEG  -90.0f
#define RB_MOUNT_DEG -135.0f

// ============================================================
//  Offset hiệu chỉnh servo (độ) — chỉnh sau khi calibrate thực tế
// ============================================================
#define LF_CX_OFFSET  0.0f
#define LF_FM_OFFSET  0.0f
#define LF_TB_OFFSET  0.0f

#define LM_CX_OFFSET  0.0f
#define LM_FM_OFFSET  0.0f
#define LM_TB_OFFSET  0.0f

#define LB_CX_OFFSET  0.0f
#define LB_FM_OFFSET  0.0f
#define LB_TB_OFFSET  0.0f

#define RF_CX_OFFSET  0.0f
#define RF_FM_OFFSET  0.0f
#define RF_TB_OFFSET  0.0f

#define RM_CX_OFFSET  0.0f
#define RM_FM_OFFSET  0.0f
#define RM_TB_OFFSET  0.0f

#define RB_CX_OFFSET  0.0f
#define RB_FM_OFFSET  0.0f
#define RB_TB_OFFSET  0.0f

// ============================================================
//  Kích thước chân (mm)
// ============================================================
#define L1  60.5f
#define L2  120.0f
#define L3  167.8f

// ============================================================
//  Tư thế đứng khởi tạo
// ============================================================
#define STAND_Z  120.0f
#define STAND_X  170.0f

