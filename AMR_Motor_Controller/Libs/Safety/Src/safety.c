#include "safety.h"

#include <math.h>

void Safety_Init(Safety *s)
{
    s->fault = DRIVE_OK;
}

uint8_t Safety_Check(Safety *s, uint32_t elapsed_ms, float cmd_v, float cmd_w)
{
    if (s->fault != DRIVE_OK) 
    {
        return 0;
    }

    if (elapsed_ms > 50U) 
    {
        s->fault = DRIVE_LOOP_OVERRUN;
    }
    else if (!isfinite(cmd_v) || !isfinite(cmd_w)) 
    {
        s->fault = DRIVE_BAD_COMMAND;
    }

    return s->fault == DRIVE_OK;
}
