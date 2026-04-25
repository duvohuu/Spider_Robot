// ============================================================
//  PCA9685 Channel Map — Spider Robot (6 legs × 3 joints)
// ============================================================
//
//  Naming: L = Left, R = Right
//          F = Front, M = Middle, B = Back
//          CX = Coxa (nối thân)
//          FM = Femur (đốt giữa)
//          TB = Tibia (đốt cuối - chân)
//
//  Chân nhìn từ trên xuống:
//
//       LF ──┐ ┌── RF
//       LM ──┤ ├── RM
//       LB ──┘ └── RB
//
// ============================================================

// ---------- CHÂN TRÁI TRƯỚC (Left Front) — PCA 0x41 ----------
#define LF_COXA_CH    13    // 0x41 CH13: Coxa  chân trái trước
#define LF_FEMUR_CH   15    // 0x41 CH15: Femur chân trái trước
#define LF_TIBIA_CH   14    // 0x41 CH14: Tibia chân trái trước

// ---------- CHÂN TRÁI GIỮA (Left Middle) — PCA 0x41 ----------
#define LM_COXA_CH    12    // 0x41 CH12: Coxa  chân trái giữa
#define LM_FEMUR_CH   11    // 0x41 CH11: Femur chân trái giữa
#define LM_TIBIA_CH   10    // 0x41 CH10: Tibia chân trái giữa

// ---------- CHÂN TRÁI SAU (Left Back) — PCA 0x41 ----------
#define LB_COXA_CH    7     // 0x41 CH7: Coxa  chân trái sau
#define LB_FEMUR_CH   9     // 0x41 CH9: Femur chân trái sau
#define LB_TIBIA_CH   8     // 0x41 CH8: Tibia chân trái sau

// ---------- CHÂN PHẢI TRƯỚC (Right Front) — PCA 0x40 ----------
#define RF_COXA_CH    10    // 0x40 CH10 : Coxa  chân phải trước
#define RF_FEMUR_CH   7     // 0x40 CH7 : Femur chân phải trước
#define RF_TIBIA_CH   8     // 0x40 CH8: Tibia chân phải trước

// ---------- CHÂN PHẢI GIỮA (Right Middle) — PCA 0x40 ----------
#define RM_COXA_CH    11    // 0x40 CH11: Coxa  chân phải giữa
#define RM_FEMUR_CH   12    // 0x40 CH12: Femur chân phải giữa
#define RM_TIBIA_CH   9     // 0x40 CH9: Tibia chân phải giữa

// ---------- CHÂN PHẢI SAU (Right Back) — PCA 0x40 ----------
#define RB_COXA_CH    15     // 0x40 CH15: Coxa  chân phải sau
#define RB_FEMUR_CH   13     // 0x40 CH13: Femur chân phải sau
#define RB_TIBIA_CH   14     // 0x40 CH14: Tibia chân phải sau