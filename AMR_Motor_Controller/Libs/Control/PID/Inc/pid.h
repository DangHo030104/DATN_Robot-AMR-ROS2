#ifndef PID_H
#define PID_H

/* Thuat toan dieu khien PID */

typedef struct
{
    float kp;
    float kp_fine;
    float ki;
    float kd;
    float fine_band;
    float integral;     // Tong tich phan hien tai
    float prev_input;   // Gia tri dau vao truoc do
    float min;
    float max;
} Pid;

typedef struct
{
    float kp;
    float kp_fine;
    float ki;
    float kd;
    float fine_band;
    float min;
    float max;
} PidCfg;

void Pid_Init(Pid *pid, const PidCfg *cfg);
void Pid_Reset(Pid *pid);
float Pid_Update(Pid *pid, float target, float input, float dt);

#endif
