#ifndef ENCODER_H
#define ENCODER_H

/* Doc encoder, xu ly tran bo dem */

#include "main.h"
#include <stdint.h>

typedef struct
{
    TIM_HandleTypeDef *tim;
    uint16_t prev;              // Gia tri cua bo dem CNT truoc do    
    int8_t sign;                // Dau hieu dao chieu (1: bth, -1: dao chieu)
    int64_t total;              // Tong so xung da dem duoc
} Encoder;

void Encoder_Init(Encoder *enc, TIM_HandleTypeDef *tim, int8_t sign);
HAL_StatusTypeDef Encoder_Start(Encoder *enc);
int32_t Encoder_Read(Encoder *enc);

#endif
