#ifndef ROBOT_CFG_H
#define ROBOT_CFG_H

/* Cau hinh robot */

#define WATCHDOG_TIMEOUT_MS  500U        // Dung motor neu mat lenh trong 500 ms
#define CTRL_PERIOD_MS       10U         // Chu ky dieu khien (ms)
#define WHEEL_RADIUS_M       0.0325f     // Ban kinh banh xe (m)
#define WHEEL_BASE_M   	     0.215f      // Khoang cach giua 2 banh xe (m)
#define WHEEL_CPR            1320.0f     // So xung encoder/1 vong quay
#define WHEEL_MAX_RPM        60.0f       // Toc do toi da cua dong co (RPM)

// Gioi han chuyen dong cua robot
#define DRIVE_MAX_V          0.10f
#define DRIVE_MAX_W          0.50f
#define DRIVE_ACC_V          0.15f
#define DRIVE_ACC_W          0.80f

// Thong so PID
#define PID_KP               0.60f       // Error lớn: Kp lớn để motor nhanh đạt setpoint
#define PID_KP_FINE          0.15f       // Error nhỏ: Kp nhỏ để giảm rung quanh setpoint
#define PID_KI               1.00f       
#define PID_KD               0.00f       
#define PID_FINE_BAND        2.00f       // Vùng error nhỏ (±2 RPM)

#endif
