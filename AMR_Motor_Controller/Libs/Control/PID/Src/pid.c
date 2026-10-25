#include "pid.h"

#include <math.h>

/* Giới hạn x trong khoảng [min, max] */
static float Clamp(float x, float min, float max)
{
    return x < min ? min : (x > max ? max : x);    
}

void Pid_Init(Pid *pid, const Pid *cfg)
{
    *pid = *cfg;
    Pid_Reset(pid);
}

void Pid_Reset(Pid *pid)
{
    pid->integral = 0.0f;
    pid->prev_input = 0.0f;
}

float Pid_Update(Pid *pid, float target, float input, float dt)
{
    float error = target - input;     
    
    /* error < fine_band -> dùng kp_fine, ngc lại dùng kp */ 
    float fine = fminf(fabsf(error), pid->fine_band);       
    
    float p = pid->kp_fine * fine + pid->kp * (fabsf(error) - fine);        // P 
    if (error < 0.0f) p = -p;                               // Đổi dấu khi error âm để P có cùng dấu với error                    

    float d = -pid->kd * (input - pid->prev_input) / dt;                    // D (không dùng nên D = 0)
    pid->prev_input = input;

    /* ===== Anti - Integral Windup ===== 
     * Có 2 lớp bảo vệ:
     * - Giới hạn riêng tích phân [min, max] 
     * - Ngừng integral khi đầu ra bão hòa */
    if (pid->ki > 0.0f) {
        float change = pid->ki * error * dt;                // I Thay đổi trong 1 chu kỳ 

        float next_i = Clamp(pid->integral + change, pid->min, pid->max);   // I 

        float next_out = p + d + next_i;                                    // P + I + D

        // Cập nhật integral nếu chưa bão hòa hoặc đang bão hòa nhưng error có dấu ngược với bão hòa
        if ((next_out >= pid->min && next_out <= pid->max) || 
            (next_out > pid->max && change < 0.0f) ||
            (next_out < pid->min && change > 0.0f)) 
        {
            pid->integral = next_i;    
        }
    } else {
        pid->integral = 0.0f;
    }

    // Trả về PWM, giới hạn duty cycle trong khoảng [-100, 100]%
    return Clamp(p + d + pid->integral, pid->min, pid->max);
}
