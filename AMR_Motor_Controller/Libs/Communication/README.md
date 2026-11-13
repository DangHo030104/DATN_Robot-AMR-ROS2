# UART protocol

USART1 uses PA9 (TX), PA10 (RX), 115200 baud, 8 data bits, no parity, and one stop bit. Connect a 3.3 V UART peer and common ground. Frames are ASCII lines terminated by `\n`; `\r\n` is accepted on receive.

| Direction | Frame | Meaning |
| --- | --- | --- |
| RPi4 to STM32 | `CMD,<v>,<w>\n` | Linear velocity in m/s and angular velocity in rad/s |
| STM32 to RPi4 | `ENC,<left>,<right>\n` | Signed cumulative encoder counts every 100 ms |
| STM32 to RPi4 | `IMU,<time_ms>,<ax>,<ay>,<az>,<gx>,<gy>,<gz>\n` | MPU6050 raw accelerometer and gyro counts every 20 ms |

The STM32 accepts finite commands with `abs(v) <= 0.20` and `abs(w) <= 0.80`. It applies the newest valid command and refreshes a 500 ms watchdog. On timeout or drive fault, it clears the command and stops both motors. A new valid command can resume control when the drive has no fault.

The MPU6050 uses I2C2 on PB10/PB11 at address `0x68`. It is configured for
accelerometer range +/-2 g (`16384 LSB/g`) and gyro range +/-250 deg/s
(`131 LSB/(deg/s)`). Gyro bias is removed during firmware startup; the serial
bridge must convert the remaining raw counts to ROS SI units.

Receive overflow, UART errors, valid commands, and invalid commands remain available as debugger counters in `serial_comm.c`. No command acknowledgment or status frame is transmitted.
