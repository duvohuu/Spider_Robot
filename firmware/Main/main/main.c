// main.c
#include <stdio.h>
#include <math.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "leg_config.h"

// ============================================================
//  I2C & PCA9685
// ============================================================
#define I2C_MASTER_NUM  I2C_NUM_0
#define I2C_MASTER_SDA  21
#define I2C_MASTER_SCL  22
#define I2C_FREQ        100000

#define PCA_L  0x41
#define PCA_R  0x40

#define MODE1       0x00
#define PRESCALE    0xFE
#define LED0_ON_L   0x06

// ============================================================
//  I2C INIT
// ============================================================
void i2c_master_init()
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_MASTER_SDA,
        .scl_io_num       = I2C_MASTER_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// ============================================================
//  PCA9685 WRITE / INIT
// ============================================================
void pca_write(uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    i2c_master_write_to_device(I2C_MASTER_NUM, addr, buf, 2, pdMS_TO_TICKS(100));
}

void pca_init(uint8_t addr)
{
    pca_write(addr, MODE1, 0x10);
    pca_write(addr, PRESCALE, 121);
    pca_write(addr, MODE1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));
    pca_write(addr, MODE1, 0xA1);
}

// ============================================================
//  SET PWM
// ============================================================
void pca_set_pwm(uint8_t addr, uint8_t ch, uint16_t on, uint16_t off)
{
    uint8_t reg = LED0_ON_L + 4 * ch;
    uint8_t data[5] = {
        reg,
        on  & 0xFF, on  >> 8,
        off & 0xFF, off >> 8
    };
    i2c_master_write_to_device(I2C_MASTER_NUM, addr, data, 5, pdMS_TO_TICKS(100));
}

// ============================================================
//  SERVO ANGLE
// ============================================================
void servo_angle(uint8_t addr, uint8_t ch, float angle)
{
    uint16_t pulse = (uint16_t)(205 + (angle / 180.0f) * (410 - 205));
    pca_set_pwm(addr, ch, 0, pulse);
}

// ============================================================
//  ĐẶT GÓC TỪNG CHÂN
// ============================================================
void lf_set(float coxa, float femur, float tibia) {
    servo_angle(PCA_L, LF_COXA_CH,  coxa);
    servo_angle(PCA_L, LF_FEMUR_CH, femur);
    servo_angle(PCA_L, LF_TIBIA_CH, tibia);
}
void lm_set(float coxa, float femur, float tibia) {
    servo_angle(PCA_L, LM_COXA_CH,  coxa);
    servo_angle(PCA_L, LM_FEMUR_CH, femur);
    servo_angle(PCA_L, LM_TIBIA_CH, tibia);
}
void lb_set(float coxa, float femur, float tibia) {
    servo_angle(PCA_L, LB_COXA_CH,  coxa);
    servo_angle(PCA_L, LB_FEMUR_CH, femur);
    servo_angle(PCA_L, LB_TIBIA_CH, tibia);
}
void rf_set(float coxa, float femur, float tibia) {
    servo_angle(PCA_R, RF_COXA_CH,  coxa);
    servo_angle(PCA_R, RF_FEMUR_CH, femur);
    servo_angle(PCA_R, RF_TIBIA_CH, tibia);
}
void rm_set(float coxa, float femur, float tibia) {
    servo_angle(PCA_R, RM_COXA_CH,  coxa);
    servo_angle(PCA_R, RM_FEMUR_CH, femur);
    servo_angle(PCA_R, RM_TIBIA_CH, tibia);
}
void rb_set(float coxa, float femur, float tibia) {
    servo_angle(PCA_R, RB_COXA_CH,  coxa);
    servo_angle(PCA_R, RB_FEMUR_CH, femur);
    servo_angle(PCA_R, RB_TIBIA_CH, tibia);
}

// ============================================================
//  THAM SỐ TRIPOD GAIT
// ============================================================

// --- Tư thế đứng ---
// Coxa  90° = chân vuông góc thân
// Femur/Tibia: chân TRÁI lắp servo đối xứng gương với PHẢI → góc DOWN ngược nhau
#define COXA_N       90.0f

#define FEMUR_DOWN_R 50.0f    // chân PHẢI: đùi chúi xuống (quan sát đúng)
#define TIBIA_DOWN_R 150.0f   // chân PHẢI: bàn chân chạm đất (quan sát đúng)
#define FEMUR_DOWN_L 130.0f   // chân TRÁI: đảo chiều 180 - 60
#define TIBIA_DOWN_L 30.0f    // chân TRÁI: đảo chiều 180 - 120

// --- Tư thế nhấc chân (90° là điểm đối xứng → giống nhau cả 2 bên) ---
#define FEMUR_UP     90.0f
#define TIBIA_UP     90.0f

// --- Biên độ bước ---
// Chân TRÁI : coxa GIẢM → tiến (quan sát thực tế)
// Chân PHẢI : coxa TĂNG → tiến (quan sát thực tế)
#define STRIDE      20.0f

#define CX_FWD_L    (COXA_N - STRIDE)   // 70°  — trái, vị trí tiến
#define CX_BWD_L    (COXA_N + STRIDE)   // 110° — trái, vị trí lùi
#define CX_FWD_R    (COXA_N + STRIDE)   // 110° — phải, vị trí tiến
#define CX_BWD_R    (COXA_N - STRIDE)   // 70°  — phải, vị trí lùi
// RM cùng chiều RF/RB: tăng = tiến

// --- Thời gian mỗi pha ---
#define T_LIFT_MS   150    // ms — nâng chân
#define T_SWING_MS  250    // ms — đưa chân ra trước / đẩy thân
#define T_DOWN_MS   150    // ms — hạ chân xuống

// ============================================================
//  TRIPOD GAIT
//
//  Nhóm A: LF (L), RM (R), LB (L)
//  Nhóm B: RF (R), LM (L), RB (R)
//
//  Trạng thái bất biến đầu mỗi chu kỳ:
//    Nhóm A tại BWD (sau thân) — chuẩn bị swing
//    Nhóm B tại FWD (trước thân) — chuẩn bị stance
// ============================================================

void calibration()
{
    lf_set(90.0f, 90.0f, 90.0f);   // A tại BWD
    rm_set(90.0f, 90.0f, 90.0f);
    lb_set(90.0f, 90.0f, 90.0f);

    rf_set(90.0f, 90.0f, 90.0f);   // B tại FWD
    lm_set(90.0f, 90.0f, 90.0f);
    rb_set(90.0f, 90.0f, 90.0f);

    vTaskDelay(pdMS_TO_TICKS(500));
}

// Đặt tư thế ban đầu trước khi bắt đầu vòng lặp
void tripod_init()
{
    lf_set(CX_BWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);   // A tại BWD
    rm_set(CX_BWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);
    lb_set(CX_BWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);

    rf_set(CX_FWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);   // B tại FWD
    lm_set(CX_FWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);
    rb_set(CX_FWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);

    vTaskDelay(pdMS_TO_TICKS(500));
}

// Một chu kỳ đi (= 2 nửa bước = robot tiến ~2×STRIDE góc)
void tripod_step()
{
    // =============================================
    //  Nửa bước 1: Nhóm A swing, Nhóm B stance
    // =============================================

    // Bước 1: Nhấc nhóm A (giữ nguyên coxa BWD)
    lf_set(CX_BWD_L, FEMUR_UP, TIBIA_UP);
    rm_set(CX_BWD_R, FEMUR_UP, TIBIA_UP);
    lb_set(CX_BWD_L, FEMUR_UP, TIBIA_UP);
    vTaskDelay(pdMS_TO_TICKS(T_LIFT_MS));

    // Bước 2: A đưa ra trước (trên không), B đẩy thân tiến (FWD → BWD)
    lf_set(CX_FWD_L, FEMUR_UP,     TIBIA_UP);
    rm_set(CX_FWD_R, FEMUR_UP,     TIBIA_UP);
    lb_set(CX_FWD_L, FEMUR_UP,     TIBIA_UP);
    rf_set(CX_BWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);
    lm_set(CX_BWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);
    rb_set(CX_BWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);
    vTaskDelay(pdMS_TO_TICKS(T_SWING_MS));

    // Bước 3: Hạ A xuống đất tại FWD
    lf_set(CX_FWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);
    rm_set(CX_FWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);
    lb_set(CX_FWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);
    vTaskDelay(pdMS_TO_TICKS(T_DOWN_MS));

    // Trạng thái giữa: A tại FWD, B tại BWD

    // =============================================
    //  Nửa bước 2: Nhóm B swing, Nhóm A stance
    // =============================================

    // Bước 4: Nhấc nhóm B (giữ nguyên coxa BWD)
    rf_set(CX_BWD_R, FEMUR_UP, TIBIA_UP);
    lm_set(CX_BWD_L, FEMUR_UP, TIBIA_UP);
    rb_set(CX_BWD_R, FEMUR_UP, TIBIA_UP);
    vTaskDelay(pdMS_TO_TICKS(T_LIFT_MS));

    // Bước 5: B đưa ra trước (trên không), A đẩy thân tiến (FWD → BWD)
    rf_set(CX_FWD_R, FEMUR_UP,     TIBIA_UP);
    lm_set(CX_FWD_L, FEMUR_UP,     TIBIA_UP);
    rb_set(CX_FWD_R, FEMUR_UP,     TIBIA_UP);
    lf_set(CX_BWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);
    rm_set(CX_BWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);
    lb_set(CX_BWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);
    vTaskDelay(pdMS_TO_TICKS(T_SWING_MS));

    // Bước 6: Hạ B xuống đất tại FWD
    rf_set(CX_FWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);
    lm_set(CX_FWD_L, FEMUR_DOWN_L, TIBIA_DOWN_L);
    rb_set(CX_FWD_R, FEMUR_DOWN_R, TIBIA_DOWN_R);
    vTaskDelay(pdMS_TO_TICKS(T_DOWN_MS));

    // Trạng thái cuối: A tại BWD, B tại FWD → giống ban đầu → lặp được
}

// ============================================================
//  MAIN
// ============================================================
void app_main()
{
    i2c_master_init();
    pca_init(PCA_L);
    pca_init(PCA_R);

    tripod_init();

    while (1)
    {
        tripod_step();
    }
}