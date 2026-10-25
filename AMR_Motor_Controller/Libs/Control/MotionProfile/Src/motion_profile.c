#include "motion_profile.h"

/* Gioi han van toc */
static float Clamp(float x, float min, float max)
{
    return x < min ? min : (x > max ? max : x);
}

/* Cap nhat van toc theo ti le */
static float Slew(float value, float target, float step)
{
    return value + Clamp(target - value, -step, step);
}

void MotionProfile_Init(MotionProfile *p, float max_v, float max_w, float acc_v, float acc_w)
{
    p->max_v = max_v;
    p->max_w = max_w;
    p->acc_v = acc_v;
    p->acc_w = acc_w;
    MotionProfile_Reset(p);
}

void MotionProfile_Reset(MotionProfile *p)
{
    p->v = 0.0f;
    p->w = 0.0f;
}

/* Gioi han van toc dai va van toc goc */
void MotionProfile_Update(MotionProfile *p, float cmd_v, float cmd_w, float dt)
{
    cmd_v = Clamp(cmd_v, -p->max_v, p->max_v);
    cmd_w = Clamp(cmd_w, -p->max_w, p->max_w);
    p->v = Slew(p->v, cmd_v, p->acc_v * dt);
    p->w = Slew(p->w, cmd_w, p->acc_w * dt);
}
