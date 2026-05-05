#pragma once
#include <stdint.h>

#define PCA_L  0x41
#define PCA_R  0x40

void i2c_master_init(void);
void pca_init(uint8_t addr);
void servo_angle(uint8_t addr, uint8_t ch, float deg);