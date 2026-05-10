#include "ik.h"
#include "leg_config.h"
#include <math.h>

IKResult inverse_kinematics(float x, float y, float z)
{
    IKResult r = {0};

    r.coxa_deg = RAD2DEG(atan2f(y, x));

    float horiz = sqrtf(x*x + y*y) - L1;
    float D     = sqrtf(horiz*horiz + z*z);

    if (D > (L2 + L3) || D < fabsf(L2 - L3) || D < 1e-3f) {
        r.valid = false;
        return r;
    }

    float cos_t3 = (D*D - L2*L2 - L3*L3) / (2.0f * L2 * L3);
    cos_t3 = CLAMP(cos_t3, -1.0f, 1.0f);
    float t3 = acosf(cos_t3);

    float alpha = atan2f(z, horiz);
    float beta  = atan2f(L3 * sinf(t3), L2 + L3 * cosf(t3));
    float t2    = alpha - beta;

    r.femur_deg = RAD2DEG(t2);
    r.tibia_deg = RAD2DEG(t3);
    r.valid     = true;
    return r;
}