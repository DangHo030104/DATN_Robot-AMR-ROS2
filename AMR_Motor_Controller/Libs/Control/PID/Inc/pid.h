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

    float integral;     // Tích phân tích lũy từ các chu kỳ trước
    float prev_input;   // RPM chu kỳ trước dùng để tính tốc độ thay đổi RPM cho thành phần D 
    
    float min;
    float max;
} Pid;

void Pid_Init(Pid *pid, const Pid *cfg);
void Pid_Reset(Pid *pid);
float Pid_Update(Pid *pid, float target, float input, float dt);

#endif
