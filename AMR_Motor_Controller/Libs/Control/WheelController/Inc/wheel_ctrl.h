#ifndef WHEEL_CTRL_H
#define WHEEL_CTRL_H

/* Điều khiển tốc độ bánh xe */

#include "encoder.h"
#include "motor_driver.h"
#include "pid.h"

#define WHEEL_FILTER_LEN 10U    

typedef struct
{
    TIM_HandleTypeDef *enc_tim;
    int8_t enc_sign;
    TIM_HandleTypeDef *pwm_tim;
    uint32_t pwm_channel;
    GPIO_TypeDef *dir_port;
    uint16_t in1;
    uint16_t in2;
    int8_t drive_sign;
} WheelHw;  

// Encoder + bộ lọc RPM + PID + MotorDriver
typedef struct
{
    Encoder enc;
    MotorDriver motor;
    Pid pid;
    float cpr;
    float target_rpm;
    float raw_rpm;      // RPM chua loc
    float rpm;          // RPM đa loc
    float avg_rpm;
    float error;
    float filter[WHEEL_FILTER_LEN];
    float filter_sum;
    uint16_t filter_pos;    // Vi tri hien tai trong bo loc
    uint16_t filter_count;  // So luong gia tri da loc 
    int32_t report_ticks;   // So xung da dem duoc trong 1 chu ky
    uint32_t report_ms;     // Thoi gian da dem duoc trong 1 chu ky (ms)
    int8_t target_sign;     // Dấu của target_rpm (1: dương, -1: âm, 0: dừng)
} WheelCtrl;

void Wheel_Init(WheelCtrl *w, const WheelHw *hw, float cpr, const Pid *pid_cfg);
HAL_StatusTypeDef Wheel_Start(WheelCtrl *w);
void Wheel_Update(WheelCtrl *w, float target_rpm, uint32_t elapsed_ms);
void Wheel_Stop(WheelCtrl *w);

#endif
