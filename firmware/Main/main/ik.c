#include "ik.h"
#include "leg_config.h"
#include <math.h>

Vec3 forward_kinematics(float coxa_deg, float femur_deg, float tibia_deg)
{
    float t1 = DEG2RAD(coxa_deg);
    float t2 = DEG2RAD(femur_deg);
    float t3 = DEG2RAD(tibia_deg);
    float reach = L1 + L2 * cosf(t2) + L3 * cosf(t2 + t3);
    Vec3 p;
    p.x = reach * cosf(t1);
    p.y = reach * sinf(t1);
    p.z = L2 * sinf(t2) + L3 * sinf(t2 + t3);
    return p;
}

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