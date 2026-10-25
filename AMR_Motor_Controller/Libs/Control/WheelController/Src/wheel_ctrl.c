#include "wheel_ctrl.h"

#include <math.h>

#define STOP_EPSILON_RPM 0.01f      // Nguong dung (RPM)
#define REPORT_PERIOD_MS 1000U      // Chu ky bao cao (ms)

static float Filter_Update(WheelCtrl *w, float value)
{
    w->filter_sum -= w->filter[w->filter_pos];
    w->filter[w->filter_pos] = value;
    w->filter_sum += value;
    if (++w->filter_pos >= WHEEL_FILTER_LEN) w->filter_pos = 0;
    if (w->filter_count < WHEEL_FILTER_LEN) ++w->filter_count;
    return w->filter_sum / w->filter_count;
}

void Wheel_Init(WheelCtrl *w, const WheelHw *hw, float cpr, const Pid *pid_cfg)
{
    Encoder_Init(&w->enc, hw->enc_tim, hw->enc_sign);
    MotorDriver_Init(&w->motor, hw->pwm_tim, hw->pwm_channel,
                     hw->dir_port, hw->in1, hw->in2, hw->drive_sign);
    Pid_Init(&w->pid, pid_cfg);

    w->cpr = cpr;
    w->target_rpm = 0.0f;
    w->raw_rpm = 0.0f;
    w->rpm = 0.0f;
    w->avg_rpm = 0.0f;
    w->error = 0.0f;
    w->filter_sum = 0.0f;
    w->filter_pos = 0;
    w->filter_count = 0;
    w->report_ticks = 0;
    w->report_ms = 0;
    w->target_sign = 0;
    for (uint16_t i = 0; i < WHEEL_FILTER_LEN; ++i) w->filter[i] = 0.0f;
}

HAL_StatusTypeDef Wheel_Start(WheelCtrl *w)
{
    if (Encoder_Start(&w->enc) != HAL_OK) return HAL_ERROR;
    return MotorDriver_Start(&w->motor);
}

/* Cap nhat trang thai bánh xe */
void Wheel_Update(WheelCtrl *w, float target_rpm, uint32_t elapsed_ms)
{
    float dt = elapsed_ms * 0.001f;         // Chuyen ms sang s
    int32_t ticks = Encoder_Read(&w->enc);  

    /* Tính raw_rpm sau đó lọc thành rpm */ 
    w->raw_rpm = ticks * 60.0f / (w->cpr * dt);
    w->rpm = Filter_Update(w, w->raw_rpm);

    // Cap nhat so xung da dem duoc trong chu ky 1s
    w->report_ticks += ticks;
    w->report_ms += elapsed_ms;

    if (w->report_ms >= REPORT_PERIOD_MS) {
        w->avg_rpm = w->report_ticks * 60000.0f / (w->cpr * w->report_ms);
        w->report_ticks = 0;
        w->report_ms = 0;
    }

    /* Dieu khien PID + MotorDriver */ 
    w->target_rpm = target_rpm;
    if (!isfinite(target_rpm) || fabsf(target_rpm) <= STOP_EPSILON_RPM) {
        w->error = 0.0f;
        w->target_sign = 0;
        Pid_Reset(&w->pid);
        w->pid.prev_input = w->rpm;
        MotorDriver_Stop(&w->motor);
        return;
    }

    // Cap nhat dau dao chieu
    int8_t sign = target_rpm > 0.0f ? 1 : -1;
    if (w->target_sign && sign != w->target_sign) {
        Pid_Reset(&w->pid);
        w->pid.prev_input = w->rpm;
    }
    w->target_sign = sign;              
    w->error = target_rpm - w->rpm;     // Sai so giua RPM muc tieu va RPM da loc

    // Cap nhat PID va dieu khien dong co
    MotorDriver_Set(&w->motor, Pid_Update(&w->pid, target_rpm, w->rpm, dt));
}

void Wheel_Stop(WheelCtrl *w)
{
    w->target_rpm = 0.0f;
    w->error = 0.0f;
    w->target_sign = 0;
    Pid_Reset(&w->pid);
    MotorDriver_Stop(&w->motor);
}
