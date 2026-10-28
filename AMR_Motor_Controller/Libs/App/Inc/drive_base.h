#ifndef DRIVE_BASE_H
#define DRIVE_BASE_H

/* Module điều phối toàn bộ hệ thống */

#include "diff_drive.h"
#include "motion_profile.h"
#include "safety.h"
#include "wheel_ctrl.h"

typedef struct
{
    WheelCtrl left;
    WheelCtrl right;
    DiffDrive model;
    MotionProfile motion;
    Safety safety;
    float v;
    float w;
} DriveBase;

extern volatile float cmd_v;       
extern volatile float cmd_w;          
extern volatile uint8_t drive_enable;       
extern volatile uint8_t drive_fault;      

void DriveBase_Init(DriveBase *base, TIM_HandleTypeDef *left_enc, TIM_HandleTypeDef *right_enc, TIM_HandleTypeDef *pwm);

HAL_StatusTypeDef DriveBase_Start(DriveBase *base);

void DriveBase_Update(DriveBase *base, uint32_t elapsed_ms);

void DriveBase_Stop(DriveBase *base);

void DriveBase_SafeStop(DriveBase *base);

#endif
