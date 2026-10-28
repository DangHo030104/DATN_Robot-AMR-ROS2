# UART protocol

USART1 uses PA9 (TX), PA10 (RX), 115200 baud, 8 data bits, no parity, and one stop bit. Connect a 3.3 V UART peer and common ground. Frames are ASCII lines terminated by `\n`; `\r\n` is accepted on receive.

| Direction | Frame | Meaning |
| --- | --- | --- |
| RPi4 to STM32 | `CMD,<v>,<w>\n` | Linear velocity in m/s and angular velocity in rad/s |
| STM32 to RPi4 | `ENC,<left>,<right>\n` | Signed cumulative encoder counts every 100 ms |

The STM32 accepts finite commands with `abs(v) <= 0.10` and `abs(w) <= 0.50`. It applies the newest valid command and refreshes a 500 ms watchdog. On timeout or drive fault, it clears the command and stops both motors. A new valid command can resume control when the drive has no fault.

Receive overflow, UART errors, valid commands, and invalid commands have debugger counters exposed through `Serial_Comm_Get*Count()` functions. No command acknowledgment or status frame is transmitted.
