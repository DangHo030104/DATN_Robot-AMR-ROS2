#include "motor_driver.h"

#include <math.h>

static float Clamp(float x, float min, float max)
{
    return x < min ? min : (x > max ? max : x);
}

void MotorDriver_Init(MotorDriver *m, TIM_HandleTypeDef *tim, uint32_t channel,
                      GPIO_TypeDef *port, uint16_t in1, uint16_t in2, int8_t sign)
{
    m->tim = tim;
    m->channel = channel;
    m->port = port;
    m->in1 = in1;
    m->in2 = in2;
    m->sign = sign;
    m->direction = 0;
    m->duty = 0.0f;

    __HAL_TIM_SET_COMPARE(tim, channel, 0);

    // Set both IN1 and IN2 pins to low to ensure the motor is stopped
    HAL_GPIO_WritePin(port, in1 | in2, GPIO_PIN_RESET);
}

HAL_StatusTypeDef MotorDriver_Start(MotorDriver *m)
{
    return HAL_TIM_PWM_Start(m->tim, m->channel);
}

void MotorDriver_Set(MotorDriver *m, float duty)
{
    duty = Clamp(duty, -100.0f, 100.0f);
    float physical = duty * m->sign;

    // Xác định hướng tiếp theo dựa trên duty
    int8_t next = physical > 0.0f ? 1 : (physical < 0.0f ? -1 : 0);

    // Nếu hướng tiếp theo khác với hướng hiện tại, dừng động cơ trước khi thay đổi hướng
    if (next && m->direction && next != m->direction) {
        __HAL_TIM_SET_COMPARE(m->tim, m->channel, 0);
        HAL_GPIO_WritePin(m->port, m->in1 | m->in2, GPIO_PIN_RESET);
        m->direction = 0;
        m->duty = 0.0f;
        return;
    }

    /* Chuyển duty cycle(%) sang giá trị compare(ARR) */
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(m->tim) + 1;                     // ARR=7200
    uint32_t compare = (uint32_t)(fabsf(physical) * period / 100.0f + 0.5f);

    /* Thiết lập chiều động cơ */ 
    if (!next) {                    // TH dừng
        HAL_GPIO_WritePin(m->port, m->in1 | m->in2, GPIO_PIN_RESET);
    } else if (next > 0) {          // TH tiến
        HAL_GPIO_WritePin(m->port, m->in1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(m->port, m->in2, GPIO_PIN_RESET);
    } else {                        // TH lùi
        HAL_GPIO_WritePin(m->port, m->in1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(m->port, m->in2, GPIO_PIN_SET);
    }

    /* Ghi giá trị compare (PWM) để điều khiển tốc độ động cơ */
    __HAL_TIM_SET_COMPARE(m->tim, m->channel, compare);

    // Lưu chiều vật lý hiện tại để lần gọi tiếp theo phát hiện đảo chiều
    m->direction = next;   

    // Chuyển compare sang duty cycle(%) 
    m->duty = next ? copysignf(100.0f * compare / period, duty) : 0.0f;
}

void MotorDriver_Stop(MotorDriver *m)
{
    MotorDriver_Set(m, 0.0f);
}
