#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

/* Điều khiển động cơ */

#include "main.h"
#include <stdint.h>

typedef struct
{
    TIM_HandleTypeDef *tim;
    uint32_t channel;
    GPIO_TypeDef *port;
    uint16_t in1;
    uint16_t in2;
    int8_t sign;
    int8_t direction;
    float duty;
} MotorDriver;

void MotorDriver_Init(MotorDriver *m, TIM_HandleTypeDef *tim, uint32_t channel,
                      GPIO_TypeDef *port, uint16_t in1, uint16_t in2, int8_t sign);
                      
HAL_StatusTypeDef MotorDriver_Start(MotorDriver *m);
void MotorDriver_Set(MotorDriver *m, float duty);
void MotorDriver_Stop(MotorDriver *m);

#endif
