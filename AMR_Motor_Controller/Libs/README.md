# Firmware modules

Application code is split by responsibility. Dependencies point from the
application layer toward control, robot model and hardware drivers.

```text
DriveBase
  +-- Safety
  +-- MotionProfile
  +-- DifferentialDrive
  +-- WheelController (left/right)
        +-- Encoder
        +-- PID
        +-- MotorDriver
```

| Module | Responsibility |
|---|---|
| `App/DriveBase` | Coordinates commands, safety, motion profile and both wheels |
| `Safety` | Latches invalid-command and loop-overrun faults |
| `Control/MotionProfile` | Limits velocity and acceleration |
| `Robot/DifferentialDrive` | Converts body velocity to wheel RPM and back |
| `Control/WheelController` | Runs the speed loop for one wheel |
| `Control/PID` | Hardware-independent PID calculation and anti-windup |
| `Drivers/Encoder` | Reads signed wheel ticks and handles timer wraparound |
| `Drivers/MotorDriver` | Controls PWM, direction pins and safe reversal |

## Debugger control

Until a communication module is added, use these globals in Live Expressions:

```text
cmd_v         linear command in m/s
cmd_w         angular command in rad/s
drive_enable  0 stops, 1 runs
drive_fault   0 OK, 1 loop overrun, 2 invalid command
```

Runtime state is available through the global `drive`, for example:

```text
drive.left.target_rpm
drive.left.rpm
drive.left.motor.duty
drive.left.enc.total
drive.right.target_rpm
drive.right.rpm
drive.v
drive.w
```
