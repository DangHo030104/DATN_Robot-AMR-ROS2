#include "command_watchdog.h"

/* Khởi tạo Watchdog, mặc định không kích hoạt */
void CommandWatchdog_Init(CommandWatchdog *wd, uint32_t timeout_ms)
{
    if (wd == 0)
        return;

    wd->last_command_ms = 0;
    wd->timeout_ms = timeout_ms;
    wd->armed = 0;
    wd->timed_out = 0;
}

/* Kích hoạt giám sát sau khi hệ thống đủ điều kiện an toàn */
void CommandWatchdog_Arm(CommandWatchdog *wd, uint32_t now_ms)
{
    if (wd == 0 || wd->timeout_ms == 0)
        return;

    wd->last_command_ms = now_ms;
    wd->timed_out = 0;
    wd->armed = 1;
}

/* Chuyển Watchdog về trạng thái không giám sát */
void CommandWatchdog_Disarm(CommandWatchdog *wd)
{
    if (wd == 0)
        return;

    wd->armed = 0;
    wd->timed_out = 0;
}

/* Làm mới thời gian khi có lệnh điều khiển hợp lệ */
void CommandWatchdog_Refresh(CommandWatchdog *wd, uint32_t now_ms)
{
    if (wd == 0)
        return;

    if (wd->armed && !wd->timed_out)
    {
        wd->last_command_ms = now_ms;
    }
}

/* Kiểm tra xem thời gian mất lệnh đã vượt giới hạn chưa */
uint8_t CommandWatchdog_Update(CommandWatchdog *wd, uint32_t now_ms)
{
    if (wd == 0 || !wd->armed)
        return 0;

    if (wd->timed_out)
        return 1;

    uint32_t elapsed = (uint32_t)(now_ms - wd->last_command_ms);

    if (elapsed >= wd->timeout_ms)
    {
        wd->timed_out = 1;
    }

    return wd->timed_out;
}
