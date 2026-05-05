#pragma once
#include <stdbool.h>

typedef struct { float x, y, z; } Vec3;

typedef struct {
    float coxa_deg;
    float femur_deg;
    float tibia_deg;
    bool  valid;
} IKResult;

Vec3     forward_kinematics(float coxa_deg, float femur_deg, float tibia_deg);
IKResult inverse_kinematics(float x, float y, float z);