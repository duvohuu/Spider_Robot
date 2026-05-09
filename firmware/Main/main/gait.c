#include "gait.h"
#include "ik.h"
#include "pca9685.h"
#include "leg_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

// ── Trạng thái vị trí chân hiện tại (body frame) ────────────
// Thứ tự: 0=LF, 1=LM, 2=LB, 3=RF, 4=RM, 5=RB
static FootPos g_foot[6];

// Chiều cao thân robot (thay đổi được qua lệnh set_height)
static volatile float g_body_height = STAND_Z;

// Trạng thái gait: wifi_server đặt cờ, gait.c đọc để lần sau
static volatile bool g_gait_running = false;

void gait_set_running(bool running) { g_gait_running = running; }
bool gait_is_running(void)          { return g_gait_running; }

void gait_get_foot_positions(FootPos out[6]) {
    memcpy(out, g_foot, sizeof(g_foot));
}
// ── Dynamic servo offsets (default từ leg_config.h) ────────────────────
// Thứ tự: 0=LF, 1=LM, 2=LB, 3=RF, 4=RM, 5=RB
static float g_cx_off[6] = {LF_CX_OFFSET, LM_CX_OFFSET, LB_CX_OFFSET,
                             RF_CX_OFFSET, RM_CX_OFFSET, RB_CX_OFFSET};
static float g_fm_off[6] = {LF_FM_OFFSET, LM_FM_OFFSET, LB_FM_OFFSET,
                             RF_FM_OFFSET, RM_FM_OFFSET, RB_FM_OFFSET};
static float g_tb_off[6] = {LF_TB_OFFSET, LM_TB_OFFSET, LB_TB_OFFSET,
                             RF_TB_OFFSET, RM_TB_OFFSET, RB_TB_OFFSET};

// ── Tham số gait (có thể thay đổi qua lệnh set_gait) ────────
static float g_step_x   = 15.0f;
static float g_lift_z   = 30.0f;
static float g_trim_y   = 0.0f;   // dương → bẻ phải (sửa lệch trái)
static int   g_t_lift   = 120;
static int   g_t_swing  = 220;
static int   g_t_down   = 120;
// Vị trí Y khi chân ở FWD / BWD (có trim heading)
#define YPOS_FWD(by)  ((by) - g_trim_y)
#define YPOS_BWD(by)  ((by) + g_trim_y)

void gait_set_offset(int leg_idx, float cx, float fm, float tb)
{
    if (leg_idx < 0 || leg_idx >= 6) return;
    g_cx_off[leg_idx] = cx;
    g_fm_off[leg_idx] = fm;
    g_tb_off[leg_idx] = tb;
}

void gait_set_params(float step_x, float lift_z, int t_lift, int t_swing, int t_down, float trim_y)
{
    if (step_x  > 0) g_step_x  = step_x;
    if (lift_z  > 0) g_lift_z  = lift_z;
    if (t_lift  > 0) g_t_lift  = t_lift;
    if (t_swing > 0) g_t_swing = t_swing;
    if (t_down  > 0) g_t_down  = t_down;
    g_trim_y = trim_y;
}

#define FOOT_FWD_X  (STAND_X + g_step_x)
#define FOOT_BWD_X  (STAND_X - g_step_x)
#define FOOT_Y      0.0f
#define FOOT_GND_Z  g_body_height
#define FOOT_AIR_Z  (g_body_height - g_lift_z)

#define T_LIFT_MS   g_t_lift
#define T_SWING_MS  g_t_swing
#define T_DOWN_MS   g_t_down

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
static void set_leg(int leg_idx,
                    uint8_t pca, uint8_t cx_ch, uint8_t fm_ch, uint8_t tb_ch,
                    float mount_deg, int cx_dir, int fm_dir, int tb_dir,
                    float cx_off,   float fm_off, float tb_off,
                    float x, float y, float z)
{
    g_foot[leg_idx] = (FootPos){x, y, z};

    float rad = DEG2RAD(mount_deg);
    float lx  =  x * cosf(rad) + y * sinf(rad);
    float ly  = -x * sinf(rad) + y * cosf(rad);

    IKResult ik = inverse_kinematics(lx, ly, z);
    if (!ik.valid) return;

    servo_angle(pca, cx_ch, 90.0f + cx_dir * ik.coxa_deg  + cx_off);
    servo_angle(pca, fm_ch, 90.0f + fm_dir * ik.femur_deg + fm_off);
    servo_angle(pca, tb_ch, 90.0f - tb_dir * (ik.tibia_deg - 90.0f) + tb_off);
}
// ── LEG macros ───────────────────────────────────────────────
#define LEG_LF(x,y,z) set_leg(0,PCA_L,LF_COXA_CH,LF_FEMUR_CH,LF_TIBIA_CH, \
    LF_MOUNT_DEG,+1,+1,+1, g_cx_off[0],g_fm_off[0],g_tb_off[0],x,y,z)
#define LEG_LM(x,y,z) set_leg(1,PCA_L,LM_COXA_CH,LM_FEMUR_CH,LM_TIBIA_CH, \
    LM_MOUNT_DEG,+1,+1,+1, g_cx_off[1],g_fm_off[1],g_tb_off[1],x,y,z)
#define LEG_LB(x,y,z) set_leg(2,PCA_L,LB_COXA_CH,LB_FEMUR_CH,LB_TIBIA_CH, \
    LB_MOUNT_DEG,+1,+1,+1, g_cx_off[2],g_fm_off[2],g_tb_off[2],x,y,z)

#define LEG_RF(x,y,z) set_leg(3,PCA_R,RF_COXA_CH,RF_FEMUR_CH,RF_TIBIA_CH, \
    RF_MOUNT_DEG,+1,-1,-1, g_cx_off[3],g_fm_off[3],g_tb_off[3],x,y,z)
#define LEG_RM(x,y,z) set_leg(4,PCA_R,RM_COXA_CH,RM_FEMUR_CH,RM_TIBIA_CH, \
    RM_MOUNT_DEG,+1,-1,-1, g_cx_off[4],g_fm_off[4],g_tb_off[4],x,y,z)
#define LEG_RB(x,y,z) set_leg(5,PCA_R,RB_COXA_CH,RB_FEMUR_CH,RB_TIBIA_CH, \
    RB_MOUNT_DEG,+1,-1,-1, g_cx_off[5],g_fm_off[5],g_tb_off[5],x,y,z)
// ── Gait functions ───────────────────────────────────────────
// Di chuyen ca 6 chan ve vi tri stand voi chieu cao z tuy chinh
void gait_set_height(float z)
{
    g_body_height = z;   
    if (!g_gait_running) {
        LEG_LF(LF_BX, LF_BY, z);
        LEG_LM(LM_BX, LM_BY, z);
        LEG_LB(LB_BX, LB_BY, z);
        LEG_RF(RF_BX, RF_BY, z);
        LEG_RM(RM_BX, RM_BY, z);
        LEG_RB(RB_BX, RB_BY, z);
    }
}

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
    // Nhóm A (LF=0, RM=4, LB=2) → BWD
    LEG_LF(LF_BX - g_step_x, YPOS_BWD(LF_BY), FOOT_GND_Z);
    LEG_RM(RM_BX - g_step_x, YPOS_BWD(RM_BY), FOOT_GND_Z);
    LEG_LB(LB_BX - g_step_x, YPOS_BWD(LB_BY), FOOT_GND_Z);
    // Nhóm B (RF=3, LM=1, RB=5) → FWD
    LEG_RF(RF_BX + g_step_x, YPOS_FWD(RF_BY), FOOT_GND_Z);
    LEG_LM(LM_BX + g_step_x, YPOS_FWD(LM_BY), FOOT_GND_Z);
    LEG_RB(RB_BX + g_step_x, YPOS_FWD(RB_BY), FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(500));
}

void tripod_step(void)
{
    // ── Nửa bước 1: A swing FWD, B stance FWD→BWD ──

    // B stance: di chuyển nhỏ dần về BWD TRƯỚC khi nhấc A
    LEG_RF(RF_BX, RF_BY, FOOT_GND_Z);
    LEG_LM(LM_BX, LM_BY, FOOT_GND_Z);
    LEG_RB(RB_BX, RB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(80));

    // Nhấc A
    LEG_LF(LF_BX - g_step_x, YPOS_BWD(LF_BY), FOOT_AIR_Z);
    LEG_RM(RM_BX - g_step_x, YPOS_BWD(RM_BY), FOOT_AIR_Z);
    LEG_LB(LB_BX - g_step_x, YPOS_BWD(LB_BY), FOOT_AIR_Z);
    vTaskDelay(pdMS_TO_TICKS(T_LIFT_MS));

    // A swing FWD (trên không)
    LEG_LF(LF_BX + g_step_x, YPOS_FWD(LF_BY), FOOT_AIR_Z);
    LEG_RM(RM_BX + g_step_x, YPOS_FWD(RM_BY), FOOT_AIR_Z);
    LEG_LB(LB_BX + g_step_x, YPOS_FWD(LB_BY), FOOT_AIR_Z);
    vTaskDelay(pdMS_TO_TICKS(T_SWING_MS));

    // Hạ A
    LEG_LF(LF_BX + g_step_x, YPOS_FWD(LF_BY), FOOT_GND_Z);
    LEG_RM(RM_BX + g_step_x, YPOS_FWD(RM_BY), FOOT_GND_Z);
    LEG_LB(LB_BX + g_step_x, YPOS_FWD(LB_BY), FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(T_DOWN_MS));

    // B stance về BWD SAU khi A đã chạm đất
    LEG_RF(RF_BX - g_step_x, YPOS_BWD(RF_BY), FOOT_GND_Z);
    LEG_LM(LM_BX - g_step_x, YPOS_BWD(LM_BY), FOOT_GND_Z);
    LEG_RB(RB_BX - g_step_x, YPOS_BWD(RB_BY), FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(80));

    // ── Nửa bước 2: B swing FWD, A stance FWD→BWD ──

    // A stance: về neutral trước
    LEG_LF(LF_BX, LF_BY, FOOT_GND_Z);
    LEG_RM(RM_BX, RM_BY, FOOT_GND_Z);
    LEG_LB(LB_BX, LB_BY, FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(80));

    // Nhấc B
    LEG_RF(RF_BX - g_step_x, YPOS_BWD(RF_BY), FOOT_AIR_Z);
    LEG_LM(LM_BX - g_step_x, YPOS_BWD(LM_BY), FOOT_AIR_Z);
    LEG_RB(RB_BX - g_step_x, YPOS_BWD(RB_BY), FOOT_AIR_Z);
    vTaskDelay(pdMS_TO_TICKS(T_LIFT_MS));

    // B swing FWD (trên không)
    LEG_RF(RF_BX + g_step_x, YPOS_FWD(RF_BY), FOOT_AIR_Z);
    LEG_LM(LM_BX + g_step_x, YPOS_FWD(LM_BY), FOOT_AIR_Z);
    LEG_RB(RB_BX + g_step_x, YPOS_FWD(RB_BY), FOOT_AIR_Z);
    vTaskDelay(pdMS_TO_TICKS(T_SWING_MS));

    // Hạ B
    LEG_RF(RF_BX + g_step_x, YPOS_FWD(RF_BY), FOOT_GND_Z);
    LEG_LM(LM_BX + g_step_x, YPOS_FWD(LM_BY), FOOT_GND_Z);
    LEG_RB(RB_BX + g_step_x, YPOS_FWD(RB_BY), FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(T_DOWN_MS));

    // A stance về BWD
    LEG_LF(LF_BX - g_step_x, YPOS_BWD(LF_BY), FOOT_GND_Z);
    LEG_RM(RM_BX - g_step_x, YPOS_BWD(RM_BY), FOOT_GND_Z);
    LEG_LB(LB_BX - g_step_x, YPOS_BWD(LB_BY), FOOT_GND_Z);
    vTaskDelay(pdMS_TO_TICKS(80));
}

// ── Pose mode: điều khiển trực tiếp từng chân ───────────────────────────────────
void gait_set_foot_direct(int leg_idx, float x, float y, float z)
{
    if (g_gait_running) return;  // bảo vệ: không ghi đè khi đang gait
    switch (leg_idx) {
        case 0: LEG_LF(x, y, z); break;
        case 1: LEG_LM(x, y, z); break;
        case 2: LEG_LB(x, y, z); break;
        case 3: LEG_RF(x, y, z); break;
        case 4: LEG_RM(x, y, z); break;
        case 5: LEG_RB(x, y, z); break;
        default: break;
    }
}
