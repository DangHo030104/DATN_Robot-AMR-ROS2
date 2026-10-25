#ifndef SAFETY_H
#define SAFETY_H

/* Kiểm tra lỗi hệ thống */

#include <stdint.h>

typedef enum
{
    DRIVE_OK = 0,
    DRIVE_LOOP_OVERRUN = 1,
    DRIVE_BAD_COMMAND = 2
} DriveFault;

typedef struct
{
    DriveFault fault;
} Safety;

void Safety_Init(Safety *s);
uint8_t Safety_Check(Safety *s, uint32_t elapsed_ms, float cmd_v, float cmd_w);

#endif
