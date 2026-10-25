#include "encoder.h"

void Encoder_Init(Encoder *enc, TIM_HandleTypeDef *tim, int8_t sign)
{
    enc->tim = tim;
    enc->prev = 0;
    enc->sign = sign;
    enc->total = 0;
    __HAL_TIM_SET_COUNTER(tim, 0);
}

HAL_StatusTypeDef Encoder_Start(Encoder *enc)
{
    return HAL_TIM_Encoder_Start(enc->tim, TIM_CHANNEL_ALL);
}

int32_t Encoder_Read(Encoder *enc)
{
    uint16_t now = (uint16_t)__HAL_TIM_GET_COUNTER(enc->tim);
    int32_t delta = (int32_t)now - (int32_t)enc->prev;
    if (delta > 32767) delta -= 65536;
    if (delta < -32768) delta += 65536;
    enc->prev = now;
    delta *= enc->sign;
    enc->total += delta;
    return delta;
}
