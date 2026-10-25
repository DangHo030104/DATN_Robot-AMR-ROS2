#ifndef MOTION_PROFILE_H
#define MOTION_PROFILE_H

/* Giới hạn tốc độ và gia tốc */

typedef struct
{
    float v;
    float w;
    float max_v;
    float max_w;
    float acc_v;
    float acc_w;
} MotionProfile;

void MotionProfile_Init(MotionProfile *p, float max_v, float max_w, float acc_v, float acc_w);
void MotionProfile_Reset(MotionProfile *p);
void MotionProfile_Update(MotionProfile *p, float cmd_v, float cmd_w, float dt);

#endif
