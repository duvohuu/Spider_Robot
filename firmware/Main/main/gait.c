#include "gait.h"
#include "ik.h"
#include "pca9685.h"
#include "leg_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

// ── Tham số gait ────────────────────────────────────────────
#define STEP_X    30.0f
#define LIFT_Z    25.0f

#define FOOT_FWD_X  (STAND_X + STEP_X)
#define FOOT_BWD_X  (STAND_X - STEP_X)
#define FOOT_Y      0.0f
#define FOOT_GND_Z  STAND_Z
#define FOOT_AIR_Z  (STAND_Z - LIFT_Z)

#define T_LIFT_MS   120
#define T_SWING_MS  220
#define T_DOWN_MS   120

#define LF_BX  ( STAND_X * 0.7071f)
#define LF_BY  ( STAND_X * 0.7071f)
#define LM_BX  ( 0.0f)
#define LM_BY  ( STAND_X)
#define LB_BX  (-STAND_X * 0.7071f)
#define LB_BY  ( STAND_X * 0.7071f)
#define RF_BX  ( STAND_X * 0.7071f)
#define RF_BY  (-STAND_X * 0.7071f)
#define RM_BX  ( 0.0f)
#define RM_BY  (-STAND_X)
#define RB_BX  (-STAND_X * 0.7071f)
#define RB_BY  (-STAND_X * 0.7071f)

// ── set_leg ─────────────────────────────────────────────────
static void set_leg(uint8_t pca, uint8_t cx_ch, uint8_t fm_ch, uint8_t tb_ch,
                    float mount_deg, int cx_dir, int fm_dir, int tb_dir,
                    float cx_off,   float fm_off, float tb_off,
                    float x, float y, float z)
{
    float rad = DEG2RAD(mount_deg);
    float lx  =  x * cosf(rad) + y * sinf(rad);
    float ly  = -x * sinf(rad) + y * cosf(rad);

    IKResult ik = inverse_kinematics(lx, ly, z);
    if (!ik.valid) return;

    servo_angle(pca, cx_ch, 90.0f + cx_dir * ik.coxa_deg  + cx_off);
    servo_angle(pca, fm_ch, 90.0f + fm_dir * ik.femur_deg + fm_off);
    servo_angle(pca, tb_ch, 90.0f + tb_dir * (ik.tibia_deg - 90.0f) + tb_off);
}

// ── LEG macros ───────────────────────────────────────────────
#define LEG_LF(x,y,z) set_leg(PCA_L,LF_COXA_CH,LF_FEMUR_CH,LF_TIBIA_CH, \
    LF_MOUNT_DEG,-1,-1,-1, LF_CX_OFFSET,LF_FM_OFFSET,LF_TB_OFFSET,x,y,z)
#define LEG_LM(x,y,z) set_leg(PCA_L,LM_COXA_CH,LM_FEMUR_CH,LM_TIBIA_CH, \
    LM_MOUNT_DEG,-1,-1,-1, LM_CX_OFFSET,LM_FM_OFFSET,LM_TB_OFFSET,x,y,z)
#define LEG_LB(x,y,z) set_leg(PCA_L,LB_COXA_CH,LB_FEMUR_CH,LB_TIBIA_CH, \
    LB_MOUNT_DEG,-1,-1,-1, LB_CX_OFFSET,LB_FM_OFFSET,LB_TB_OFFSET,x,y,z)
#define LEG_RF(x,y,z) set_leg(PCA_R,RF_COXA_CH,RF_FEMUR_CH,RF_TIBIA_CH, \
    RF_MOUNT_DEG,+1,+1,+1, RF_CX_OFFSET,RF_FM_OFFSET,RF_TB_OFFSET,x,y,z)
#define LEG_RM(x,y,z) set_leg(PCA_R,RM_COXA_CH,RM_FEMUR_CH,RM_TIBIA_CH, \
    RM_MOUNT_DEG,+1,+1,+1, RM_CX_OFFSET,RM_FM_OFFSET,RM_TB_OFFSET,x,y,z)
#define LEG_RB(x,y,z) set_leg(PCA_R,RB_COXA_CH,RB_FEMUR_CH,RB_TIBIA_CH, \
    RB_MOUNT_DEG,+1,+1,+1, RB_CX_OFFSET,RB_FM_OFFSET,RB_TB_OFFSET,x,y,z)

// ── Gait functions ───────────────────────────────────────────
void calibration(void)
{
    LEG_LF(LF_BX, LF_BY, FOOT_GND_Z);
    LEG_LM(LM_BX, LM_BY, FOOT_GND_Z);
    LEG_LB(LB_BX, LB_BY, FOOT_GND_Z);
    LEG_RF(RF_BX, RF_BY, FOOT_GND_Z);
    LEG_RM(RM_BX, RM_BY, FOOT_GND_Z);
    LEG_RB(RB_BX, RB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

///A bắt đầu ở BWD, B ở FWD — khớp đúng với đầu vào của tripod_step
void tripod_init(void)
{
    // Nhóm A (LF, RM, LB) → BWD
    LEG_LF(LF_BX - STEP_X, LF_BY, FOOT_GND_Z);
    LEG_RM(RM_BX - STEP_X, RM_BY, FOOT_GND_Z);
    LEG_LB(LB_BX - STEP_X, LB_BY, FOOT_GND_Z);
    // Nhóm B (RF, LM, RB) → FWD
    LEG_RF(RF_BX + STEP_X, RF_BY, FOOT_GND_Z);
    LEG_LM(LM_BX + STEP_X, LM_BY, FOOT_GND_Z);
    LEG_RB(RB_BX + STEP_X, RB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(500));
}

void tripod_step(void)
{
    // Nửa bước 1: A swing FWD, B stance FWD→BWD
    LEG_LF(LF_BX - STEP_X, LF_BY, FOOT_AIR_Z);
    LEG_RM(RM_BX - STEP_X, RM_BY, FOOT_AIR_Z);
    LEG_LB(LB_BX - STEP_X, LB_BY, FOOT_AIR_Z);
    vTaskDelay(pdMS_TO_TICKS(T_LIFT_MS));

    LEG_LF(LF_BX + STEP_X, LF_BY, FOOT_AIR_Z);
    LEG_RM(RM_BX + STEP_X, RM_BY, FOOT_AIR_Z);
    LEG_LB(LB_BX + STEP_X, LB_BY, FOOT_AIR_Z);
    LEG_RF(RF_BX - STEP_X, RF_BY, FOOT_GND_Z);
    LEG_LM(LM_BX - STEP_X, LM_BY, FOOT_GND_Z);
    LEG_RB(RB_BX - STEP_X, RB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(T_SWING_MS));

    LEG_LF(LF_BX + STEP_X, LF_BY, FOOT_GND_Z);
    LEG_RM(RM_BX + STEP_X, RM_BY, FOOT_GND_Z);
    LEG_LB(LB_BX + STEP_X, LB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(T_DOWN_MS));

    // Nửa bước 2: B swing FWD, A stance FWD→BWD
    LEG_RF(RF_BX - STEP_X, RF_BY, FOOT_AIR_Z);
    LEG_LM(LM_BX - STEP_X, LM_BY, FOOT_AIR_Z);
    LEG_RB(RB_BX - STEP_X, RB_BY, FOOT_AIR_Z);
    vTaskDelay(pdMS_TO_TICKS(T_LIFT_MS));

    LEG_RF(RF_BX + STEP_X, RF_BY, FOOT_AIR_Z);
    LEG_LM(LM_BX + STEP_X, LM_BY, FOOT_AIR_Z);
    LEG_RB(RB_BX + STEP_X, RB_BY, FOOT_AIR_Z);
    LEG_LF(LF_BX - STEP_X, LF_BY, FOOT_GND_Z);
    LEG_RM(RM_BX - STEP_X, RM_BY, FOOT_GND_Z);
    LEG_LB(LB_BX - STEP_X, LB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(T_SWING_MS));

    LEG_RF(RF_BX + STEP_X, RF_BY, FOOT_GND_Z);
    LEG_LM(LM_BX + STEP_X, LM_BY, FOOT_GND_Z);
    LEG_RB(RB_BX + STEP_X, RB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(T_DOWN_MS));
}