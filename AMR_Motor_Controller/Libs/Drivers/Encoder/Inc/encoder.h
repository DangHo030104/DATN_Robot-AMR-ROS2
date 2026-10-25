#ifndef ENCODER_H
#define ENCODER_H

/* Doc encoder, xu ly tran bo dem */

#include "main.h"
#include <stdint.h>

typedef struct
{
    TIM_HandleTypeDef *tim;
    uint16_t prev;              // Lưu gia tri CNT truoc do    
    int8_t sign;                // Chuẩn hóa chiều quay (1: thuan, -1: nghich)
    int64_t total;              // tông số xung đã đếm được kể từ khi khởi tạo
} Encoder;

void Encoder_Init(Encoder *enc, TIM_HandleTypeDef *tim, int8_t sign);
HAL_StatusTypeDef Encoder_Start(Encoder *enc);
int32_t Encoder_Read(Encoder *enc);

#endif
