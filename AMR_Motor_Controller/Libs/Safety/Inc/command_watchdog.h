#ifndef COMMAND_WATCHDOG_H
#define COMMAND_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t last_command_ms;
    uint32_t timeout_ms;

    uint8_t armed;
    uint8_t timed_out;
} CommandWatchdog;

void CommandWatchdog_Init(CommandWatchdog *wd, uint32_t timeout_ms);
void CommandWatchdog_Arm(CommandWatchdog *wd, uint32_t now_ms);
void CommandWatchdog_Disarm(CommandWatchdog *wd);
void CommandWatchdog_Refresh(CommandWatchdog *wd, uint32_t now_ms);
uint8_t CommandWatchdog_Update(CommandWatchdog *wd, uint32_t now_ms);

#endif
