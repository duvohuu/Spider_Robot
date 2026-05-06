#pragma once

void calibration(void);
void tripod_init(void);
void tripod_step(void);

#ifdef CONFIG_DEBUG
void test_tibia_mechanical(void);
void test_femur(void);
#endif