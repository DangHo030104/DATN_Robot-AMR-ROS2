#ifndef ROBOT_CFG_H
#define ROBOT_CFG_H

/* Cau hinh robot */

#define CTRL_PERIOD_MS  10U         // Chu ky dieu khien (ms)

#define WHEEL_RADIUS_M  0.0325f     // Ban kinh banh xe (m)
#define WHEEL_TRACK_M   0.215f      // Khoang cach giua 2 banh xe (m)
#define WHEEL_CPR       1320.0f     // So xung encoder tren 1 vong quay
#define WHEEL_MAX_RPM   60.0f       // Toc do toi da cua dong co (RPM)

// Gioi han chuyen dong cua robot
#define DRIVE_MAX_V     0.10f
#define DRIVE_MAX_W     0.50f
#define DRIVE_ACC_V     0.15f
#define DRIVE_ACC_W     0.80f

// Thong so PID
#define PID_KP          0.60f       // He so Kp chinh
#define PID_KP_FINE     0.15f       // He so Kp phu dung khi sai so gan setpoint
#define PID_KI          1.00f       // He so Ki
#define PID_KD          0.00f       // He so Kd
#define PID_FINE_BAND   2.00f       // Vung sai so nho (±2 RPM)

#endif
