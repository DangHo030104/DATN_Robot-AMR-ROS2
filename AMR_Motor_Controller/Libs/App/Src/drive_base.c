#include "drive_base.h"
#include "robot_cfg.h"

volatile float cmd_v;                // Lenh van toc dai (m/s)
volatile float cmd_w;                // Lenh van toc goc (rad/s)
volatile uint8_t drive_enable;       // Trang thai dieu khien (0: stop, 1: enable)
volatile uint8_t drive_fault;        // Trang thai loi (0: ok, 1: loop overrun, 2: bad command)

void DriveBase_Init(DriveBase *base, TIM_HandleTypeDef *left_enc, TIM_HandleTypeDef *right_enc, TIM_HandleTypeDef *pwm)
{
    /* Cau hinh phan cung cho 2 banh xe */ 
    const WheelHw left_hw = {
        left_enc,
        1,
        pwm,
        TIM_CHANNEL_1,
        MOTOR_LEFT_IN1_GPIO_Port,
        MOTOR_LEFT_IN1_Pin,
        MOTOR_LEFT_IN2_Pin,
        1
    };

    const WheelHw right_hw = {
        right_enc,
        -1,
        pwm,
        TIM_CHANNEL_2,
        MOTOR_RIGHT_IN1_GPIO_Port,
        MOTOR_RIGHT_IN1_Pin,
        MOTOR_RIGHT_IN2_Pin,
        -1
    };

    /* Cau hinh PID */
    const Pid pid_cfg = {
        .kp = PID_KP,
        .kp_fine = PID_KP_FINE,
        .ki = PID_KI,
        .kd = PID_KD,
        .fine_band = PID_FINE_BAND,
        .min = -100.0f,
        .max = 100.0f
    };

    /* Khoi tao cac thanh phan */
    Wheel_Init(&base->left, &left_hw, WHEEL_CPR, &pid_cfg);
    Wheel_Init(&base->right, &right_hw, WHEEL_CPR, &pid_cfg);

    DiffDrive_Init(&base->model, WHEEL_RADIUS_M, WHEEL_TRACK_M, WHEEL_MAX_RPM);
    
    MotionProfile_Init(&base->motion, DRIVE_MAX_V, DRIVE_MAX_W, DRIVE_ACC_V, DRIVE_ACC_W);
    
    Safety_Init(&base->safety);

    /* Khoi tao trang thai dieu khien */
    base->v = 0.0f;
    base->w = 0.0f;
    drive_enable = 0;
    drive_fault = DRIVE_OK;
}

HAL_StatusTypeDef DriveBase_Start(DriveBase *base)
{
    if (Wheel_Start(&base->left) != HAL_OK || Wheel_Start(&base->right) != HAL_OK) {
        DriveBase_Stop(base);
        return HAL_ERROR;
    }
    return HAL_OK;
}

/* Cap nhat trang thai dieu khien */
void DriveBase_Update(DriveBase *base, uint32_t elapsed_ms)
{
    float v = cmd_v;
    float w = cmd_w;
    uint8_t safe = Safety_Check(&base->safety, elapsed_ms, v, w);
    drive_fault = base->safety.fault;      

    float left_target = 0.0f;
    float right_target = 0.0f;

    if (safe && drive_enable && (v != 0.0f || w != 0.0f)) {
        MotionProfile_Update(&base->motion, v, w, elapsed_ms * 0.001f);
        DiffDrive_ToWheels(&base->model, base->motion.v, base->motion.w, &left_target, &right_target);
    } else {
        if (!safe) drive_enable = 0;
        MotionProfile_Reset(&base->motion);
    }
 
    /* Điều khiển bánh xe */
    Wheel_Update(&base->left, left_target, elapsed_ms);
    Wheel_Update(&base->right, right_target, elapsed_ms);
    DiffDrive_ToBody(&base->model, base->left.rpm, base->right.rpm, &base->v, &base->w);
}

void DriveBase_Stop(DriveBase *base)
{
    drive_enable = 0;
    MotionProfile_Reset(&base->motion);
    Wheel_Stop(&base->left);
    Wheel_Stop(&base->right);
}
