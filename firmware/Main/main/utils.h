#pragma once
#include <math.h>

#define DEG2RAD(d)     ((d) * (float)M_PI / 180.0f)
#define RAD2DEG(r)     ((r) * 180.0f / (float)M_PI)
#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):((v)>(hi)?(hi):(v)))
