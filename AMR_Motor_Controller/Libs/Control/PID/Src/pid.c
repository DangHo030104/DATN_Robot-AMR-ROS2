#include "pid.h"

#include <math.h>

static float Clamp(float x, float min, float max)
{
    return x < min ? min : (x > max ? max : x);
}

void Pid_Init(Pid *pid, const PidCfg *cfg)
{
    pid->kp = cfg->kp;
    pid->kp_fine = cfg->kp_fine;
    pid->ki = cfg->ki;
    pid->kd = cfg->kd;
    pid->fine_band = cfg->fine_band;
    pid->min = cfg->min;
    pid->max = cfg->max;
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
    float fine = fminf(fabsf(error), pid->fine_band);       // error < fine_band thi dung kp_fine, nguoc lai dung kp
    float kp = fmaxf(pid->kp, 0.0f);                        // Kp khong am
    float kp_fine = Clamp(pid->kp_fine, 0.0f, kp);          // Kp_fine khong am va khong duoc > kp
    float p = kp_fine * fine + kp * (fabsf(error) - fine);  // Thanh phan P cua PID
    if (error < 0.0f) p = -p;                           

    float d = -pid->kd * (input - pid->prev_input) / dt;        // Thanh phan D cua PID
    pid->prev_input = input;

    if (pid->ki > 0.0f) {
        float change = pid->ki * error * dt;
        float next_i = Clamp(pid->integral + change, pid->min, pid->max);   // Thanh phan I cua PID
        float next_out = p + d + next_i;        // Tong cac thanh phan P + I + D
        if ((next_out >= pid->min && next_out <= pid->max) || (next_out > pid->max && change < 0.0f) ||
            (next_out < pid->min && change > 0.0f)) {
            pid->integral = next_i;
        }
    } else {
        pid->integral = 0.0f;
    }
    return Clamp(p + d + pid->integral, pid->min, pid->max);
}
