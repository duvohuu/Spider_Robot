#pragma once
#include <stdbool.h>

typedef struct {
    float coxa_deg;
    float femur_deg;
    float tibia_deg;
    bool  valid;
} IKResult;

IKResult inverse_kinematics(float x, float y, float z);