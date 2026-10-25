# Các module điều khiển robot AMR

Thư mục `Libs` chứa toàn bộ mã nguồn do người dùng viết cho phần điều khiển
hai động cơ của robot AMR. Mã được chia theo chức năng để mỗi module chỉ đảm
nhận một nhiệm vụ, dễ đọc, dễ kiểm thử và thuận tiện khi bổ sung giao tiếp với
Raspberry Pi trong tương lai.

Firmware hiện tại thực hiện các chức năng chính:

- Nhận lệnh vận tốc tuyến tính `v` và vận tốc góc `w`.
- Giới hạn vận tốc và gia tốc của robot.
- Chuyển vận tốc robot thành RPM mục tiêu của từng bánh.
- Đọc encoder và tính RPM thực tế.
- Điều khiển tốc độ từng bánh bằng PID.
- Xuất PWM và chiều quay tới driver động cơ.
- Phát hiện lệnh không hợp lệ và vòng điều khiển chạy quá chậm.

Telemetry và MPU6050 chưa được sử dụng trong phiên bản này.

## 1. Cấu trúc thư mục

```text
Libs/
|-- App/
|   |-- Inc/drive_base.h
|   `-- Src/drive_base.c
|-- Control/
|   |-- MotionProfile/
|   |   |-- Inc/motion_profile.h
|   |   `-- Src/motion_profile.c
|   |-- PID/
|   |   |-- Inc/pid.h
|   |   `-- Src/pid.c
|   `-- WheelController/
|       |-- Inc/wheel_ctrl.h
|       `-- Src/wheel_ctrl.c
|-- Drivers/
|   |-- Encoder/
|   |   |-- Inc/encoder.h
|   |   `-- Src/encoder.c
|   `-- MotorDriver/
|       |-- Inc/motor_driver.h
|       `-- Src/motor_driver.c
|-- Robot/
|   |-- Inc/robot_cfg.h
|   `-- DifferentialDrive/
|       |-- Inc/diff_drive.h
|       `-- Src/diff_drive.c
|-- Safety/
|   |-- Inc/safety.h
|   `-- Src/safety.c
`-- README.md
```

## 2. Quan hệ giữa các module

```text
Core/main.c
    |
    `-- App/DriveBase
        |-- Safety
        |-- MotionProfile
        |-- DifferentialDrive
        |-- WheelController trái
        |   |-- Encoder
        |   |-- PID
        |   `-- MotorDriver
        `-- WheelController phải
            |-- Encoder
            |-- PID
            `-- MotorDriver
```

Các module tầng trên gọi module tầng dưới. `PID` và `DifferentialDrive` chỉ xử
lý toán học, không truy cập trực tiếp GPIO hoặc timer. `Encoder` và
`MotorDriver` là hai module làm việc trực tiếp với phần cứng STM32.

## 3. Luồng điều khiển

Vòng điều khiển được gọi từ `main.c` mỗi `CTRL_PERIOD_MS`, hiện là 10 ms:

```text
cmd_v, cmd_w
      |
      v
Safety_Check()
      |
      v
MotionProfile_Update()
      |
      v
DiffDrive_ToWheels()
      |
      +-----------------------+
      |                       |
      v                       v
Wheel_Update(trái)      Wheel_Update(phải)
      |                       |
      | Encoder -> PID        | Encoder -> PID
      |       -> PWM          |       -> PWM
      +-----------+-----------+
                  |
                  v
          DiffDrive_ToBody()
                  |
                  v
             drive.v, drive.w
```

Trình tự trong một chu kỳ:

1. Đọc `cmd_v` và `cmd_w`.
2. Kiểm tra lỗi an toàn.
3. Giới hạn lệnh vận tốc và tốc độ thay đổi của lệnh.
4. Tính RPM mục tiêu cho bánh trái và bánh phải.
5. Đọc số xung encoder phát sinh trong chu kỳ.
6. Tính và lọc RPM thực tế của từng bánh.
7. PID tính duty từ sai số RPM.
8. MotorDriver xuất chiều quay và PWM.
9. Từ RPM hai bánh, tính lại vận tốc thực tế `drive.v` và `drive.w`.

## 4. App/DriveBase

`DriveBase` là module điều phối cấp cao nhất. Module này kết nối tất cả thành
phần của hệ thống truyền động nhưng không trực tiếp điều khiển chân GPIO.

Structure chính:

```c
typedef struct
{
    WheelCtrl left;
    WheelCtrl right;
    DiffDrive model;
    MotionProfile motion;
    Safety safety;
    float v;
    float w;
} DriveBase;
```

Các hàm:

- `DriveBase_Init()`: khởi tạo phần cứng hai bánh, PID, động học, MotionProfile
  và Safety.
- `DriveBase_Start()`: bắt đầu timer encoder và PWM của hai bánh.
- `DriveBase_Update()`: thực hiện một chu kỳ điều khiển hoàn chỉnh.
- `DriveBase_Stop()`: khóa điều khiển, reset PID và đưa PWM về 0.

Các biến lệnh và trạng thái toàn cục:

```c
cmd_v         // Vận tốc tuyến tính yêu cầu, đơn vị m/s
cmd_w         // Vận tốc góc yêu cầu, đơn vị rad/s
drive_enable  // 0: dừng, 1: cho phép chạy
drive_fault   // Mã lỗi truyền động
```

`drive_enable` mặc định bằng 0, vì vậy robot không tự chạy sau khi cấp nguồn.
Cho đến khi có module giao tiếp, có thể thay đổi các biến này bằng Live
Expressions trong STM32CubeIDE.

## 5. Robot/DifferentialDrive

Module này cài đặt động học cho robot hai bánh vi sai.

### Động học nghịch

Chuyển vận tốc thân robot thành RPM hai bánh:

```text
v_left  = v - track * w / 2
v_right = v + track * w / 2

left_rpm  = v_left  * 60 / (2*pi*radius)
right_rpm = v_right * 60 / (2*pi*radius)
```

Nếu một bánh vượt `WHEEL_MAX_RPM`, RPM của cả hai bánh được giảm theo cùng một
tỉ lệ. Cách này giữ nguyên quan hệ giữa hai bánh và do đó không làm thay đổi
hướng chuyển động mong muốn.

### Động học thuận

Chuyển RPM thực tế của hai bánh thành vận tốc thân robot:

```text
v = (v_left + v_right) / 2
w = (v_right - v_left) / track
```

Kết quả được lưu trong `drive.v` và `drive.w`. Đây là vận tốc tức thời, chưa
phải tọa độ `x`, `y`, `theta`. Việc tích phân vị trí nên được bổ sung trong một
module Odometry riêng.

## 6. Control/MotionProfile

`MotionProfile` giới hạn vận tốc và gia tốc để lệnh không thay đổi quá nhanh.

Structure chứa:

- `v`, `w`: lệnh đã được giới hạn và làm mượt.
- `max_v`, `max_w`: vận tốc tuyến tính và vận tốc góc tối đa.
- `acc_v`, `acc_w`: gia tốc tuyến tính và gia tốc góc tối đa.

Mỗi chu kỳ, giá trị chỉ được thay đổi tối đa:

```text
step_v = acc_v * dt
step_w = acc_w * dt
```

Với `DRIVE_ACC_V = 0.15 m/s²` và `dt = 0.01 s`, vận tốc tuyến tính chỉ tăng
tối đa `0.0015 m/s` trong một chu kỳ.

Hiện tại khi lệnh bằng 0 hoặc `drive_enable = 0`, `DriveBase` reset trực tiếp
MotionProfile. Vì vậy lệnh dừng được áp dụng ngay, không giảm tốc từ từ.

## 7. Control/WheelController

Mỗi bánh xe có một `WheelCtrl` độc lập. Module này kết hợp:

```text
Encoder + bộ lọc RPM + PID + MotorDriver
```

Các giá trị quan trọng:

- `target_rpm`: RPM mục tiêu.
- `raw_rpm`: RPM tính trực tiếp từ số xung của một chu kỳ.
- `rpm`: RPM sau bộ lọc trung bình trượt.
- `avg_rpm`: RPM trung bình trong khoảng một giây.
- `error`: sai số giữa RPM mục tiêu và RPM đã lọc.
- `motor.duty`: duty thực tế gửi tới MotorDriver.

RPM thô được tính theo công thức:

```text
raw_rpm = ticks * 60 / (CPR * dt)
```

Do số xung trong 10 ms thường nhỏ, `raw_rpm` thay đổi theo từng nấc. Module sử
dụng bộ lọc trung bình trượt 10 mẫu, tương đương khoảng 100 ms, trước khi đưa
RPM vào PID.

Khi RPM mục tiêu gần bằng 0, WheelController reset PID và dừng motor. Khi dấu
của RPM mục tiêu thay đổi, PID cũng được reset để tích phân của chiều cũ không
ảnh hưởng tới chiều mới.

## 8. Control/PID

PID nhận RPM mục tiêu và RPM thực tế, sau đó trả về duty trong khoảng
`-100%` đến `+100%`:

```text
error  = target - input
output = P + I + D
```

Structure `Pid` duy nhất chứa cả thông số cấu hình và trạng thái chạy:

```c
typedef struct
{
    float kp;
    float kp_fine;
    float ki;
    float kd;
    float fine_band;
    float integral;
    float prev_input;
    float min;
    float max;
} Pid;
```

Đặc điểm của PID hiện tại:

- Sử dụng `kp_fine` khi sai số nằm gần mục tiêu để giảm rung.
- Sử dụng `kp` lớn hơn cho phần sai số nằm ngoài `fine_band`.
- Thành phần I bù ma sát, tải và sai số ổn định.
- Anti-windup ngừng tăng tích phân khi đầu ra đã bão hòa.
- Thành phần D tính theo biến thiên đầu vào để tránh derivative kick khi đổi
  mục tiêu.
- `PID_KD` hiện bằng 0 nên bộ điều khiển thực tế là PI hai vùng Kp.

PID được reset khi motor dừng hoặc đổi chiều. Mỗi bánh có một đối tượng PID
riêng nên trạng thái tích phân của hai bánh không ảnh hưởng lẫn nhau.

## 9. Drivers/Encoder

Module `Encoder` đọc counter của timer đang chạy ở Encoder Mode.

Structure chứa:

- `tim`: timer encoder.
- `prev`: counter ở lần đọc trước.
- `sign`: dấu quy ước của encoder.
- `total`: tổng số xung tích lũy.

`Encoder_Read()` trả về số xung mới kể từ lần gọi trước. Module xử lý tràn
counter 16 bit trong cả hai chiều:

```text
0 ... 65535 -> 0
0 -> 65535 ... 0
```

Điều kiện là số xung giữa hai lần đọc nhỏ hơn 32768. Với chu kỳ 10 ms và tốc
độ hiện tại, số xung thực tế nhỏ hơn giới hạn này rất nhiều.

`sign` chuẩn hóa hướng encoder. Khi robot đi tới, encoder của cả hai bánh đều
có kết quả dương dù hai motor được lắp đối xứng.

## 10. Drivers/MotorDriver

`MotorDriver` chuyển duty có dấu thành tín hiệu PWM và hai chân điều khiển
chiều của driver cầu H.

Quy ước đầu vào:

```text
duty > 0  : quay thuận theo quy ước robot
duty < 0  : quay nghịch theo quy ước robot
duty = 0  : dừng
```

Duty luôn được giới hạn trong `[-100, 100]`. Biến `sign` chuyển chiều logic
của robot thành chiều điện vật lý, cần thiết vì motor trái và phải được lắp đối
xứng.

Bảng điều khiển:

| Trạng thái | IN1 | IN2 | PWM |
|---|---:|---:|---:|
| Dừng | 0 | 0 | 0% |
| Chiều dương vật lý | 1 | 0 | Theo duty |
| Chiều âm vật lý | 0 | 1 | Theo duty |

Khi lệnh yêu cầu đảo chiều trong lúc motor đang chạy, module đưa PWM và hai
chân chiều về 0 trong một chu kỳ trước khi áp dụng chiều mới. Khoảng dừng này
giúp tránh đảo chiều tức thời ở duty lớn.

## 11. Safety

Module Safety quản lý lỗi truyền động:

| Mã | Tên | Ý nghĩa |
|---:|---|---|
| 0 | `DRIVE_OK` | Hệ thống bình thường |
| 1 | `DRIVE_LOOP_OVERRUN` | Chu kỳ điều khiển lớn hơn 50 ms |
| 2 | `DRIVE_BAD_COMMAND` | `cmd_v` hoặc `cmd_w` là NaN/Infinity |

Lỗi được giữ lại sau khi xuất hiện. Khi có lỗi, `DriveBase` đặt
`drive_enable = 0`, đưa RPM mục tiêu về 0 và dừng hai motor. Phiên bản hiện tại
chưa có hàm xóa lỗi trong lúc chạy; cần reset vi điều khiển hoặc khởi tạo lại
Safety.

Các chức năng nên bổ sung sau này:

- Watchdog cho lệnh vận tốc từ Raspberry Pi.
- Phát hiện encoder mất tín hiệu khi PWM đang lớn.
- Bảo vệ quá tốc độ.
- Giám sát điện áp pin và dòng động cơ.
- Nút dừng khẩn cấp.

## 12. Thông số cấu hình robot

Các thông số được tập trung trong `Robot/Inc/robot_cfg.h`:

| Thông số | Giá trị hiện tại | Ý nghĩa |
|---|---:|---|
| `CTRL_PERIOD_MS` | 10 ms | Chu kỳ điều khiển |
| `WHEEL_RADIUS_M` | 0.0325 m | Bán kính bánh xe |
| `WHEEL_TRACK_M` | 0.215 m | Khoảng cách giữa hai bánh |
| `WHEEL_CPR` | 1320 | Số xung trên một vòng bánh |
| `WHEEL_MAX_RPM` | 60 RPM | RPM tối đa của mỗi bánh |
| `DRIVE_MAX_V` | 0.10 m/s | Vận tốc tuyến tính tối đa |
| `DRIVE_MAX_W` | 0.50 rad/s | Vận tốc góc tối đa |
| `DRIVE_ACC_V` | 0.15 m/s² | Gia tốc tuyến tính tối đa |
| `DRIVE_ACC_W` | 0.80 rad/s² | Gia tốc góc tối đa |
| `PID_KP` | 0.60 | Hệ số P chính |
| `PID_KP_FINE` | 0.15 | Hệ số P trong vùng sai số nhỏ |
| `PID_KI` | 1.00 | Hệ số tích phân |
| `PID_KD` | 0.00 | Hệ số vi phân |
| `PID_FINE_BAND` | 2 RPM | Độ rộng vùng P tinh |

`WHEEL_CPR` cần được xác nhận bằng cách quay bánh xe đúng một vòng và quan sát
độ thay đổi của `enc.total`. Giá trị phải tính cả tỉ số truyền và chế độ đếm
quadrature của timer.

## 13. Ánh xạ phần cứng

| Chức năng | Bánh trái | Bánh phải |
|---|---|---|
| Timer encoder | TIM3 | TIM2 |
| Encoder A | `ENCODER_LEFT_A` (PA6) | `ENCODER_RIGHT_A` (PA15) |
| Encoder B | `ENCODER_LEFT_B` (PA7) | `ENCODER_RIGHT_B` (PB3) |
| Dấu encoder | +1 | -1 |
| PWM | TIM4 Channel 1 | TIM4 Channel 2 |
| Chân PWM | `MOTOR_LEFT_PWM` (PB6) | `MOTOR_RIGHT_PWM` (PB7) |
| Chân IN1 | `MOTOR_LEFT_IN1` (PA1) | `MOTOR_RIGHT_IN1` (PB0) |
| Chân IN2 | `MOTOR_LEFT_IN2` (PA2) | `MOTOR_RIGHT_IN2` (PB1) |
| Dấu motor | +1 | -1 |

TIM4 hiện có `Prescaler = 71` và `Period = 7199`. Giá trị compare được tính từ
phần trăm duty:

```text
compare = |duty| * (ARR + 1) / 100
```

## 14. Các biến nên theo dõi khi debug

### Lệnh và trạng thái chung

```text
cmd_v
cmd_w
drive_enable
drive_fault
drive.v
drive.w
```

### Bánh trái

```text
drive.left.target_rpm
drive.left.raw_rpm
drive.left.rpm
drive.left.avg_rpm
drive.left.error
drive.left.pid.integral
drive.left.motor.duty
drive.left.motor.direction
drive.left.enc.total
```

### Bánh phải

```text
drive.right.target_rpm
drive.right.raw_rpm
drive.right.rpm
drive.right.avg_rpm
drive.right.error
drive.right.pid.integral
drive.right.motor.duty
drive.right.motor.direction
drive.right.enc.total
```

Khi robot chạy thẳng tới, `target_rpm`, `rpm` và `motor.duty` của cả hai bánh
nên cùng mang dấu dương theo quy ước phần mềm. `motor.direction` có thể trái
dấu giữa hai bánh vì nó biểu diễn chiều điện vật lý sau khi áp dụng `sign`.

## 15. Cách chạy thử bằng debugger

1. Nâng bánh xe khỏi mặt đất trong lần thử đầu tiên.
2. Cấp nguồn driver motor và nối chung GND với STM32.
3. Build, nạp firmware và mở Live Expressions.
4. Đặt lệnh thử ở mức thấp:

```text
cmd_v = 0.05
cmd_w = 0.0
drive_enable = 1
```

5. Kiểm tra hai giá trị `target_rpm` và `rpm`.
6. Đặt `drive_enable = 0` để dừng.

Nếu một bánh có RPM âm khi robot đang được lệnh đi tới, kiểm tra dây encoder A/B
và giá trị `enc_sign`. Nếu chiều motor sai nhưng dấu encoder đúng, kiểm tra
`drive_sign` hoặc hai chân IN1/IN2.

## 16. Hướng mở rộng

Khi bổ sung giao tiếp Raspberry Pi hoặc ESP32, module nhận dữ liệu chỉ cần cập
nhật `cmd_v`, `cmd_w` và `drive_enable`. Không nên đưa logic UART, ROS hoặc
packet protocol vào `PID`, `Encoder` hay `MotorDriver`.

Cấu trúc dự kiến có thể bổ sung:

```text
Libs/
|-- Communication/
|   `-- SerialProtocol
|-- Navigation/
|   `-- Odometry
|-- Power/
|   `-- BatteryMonitor
`-- Safety/
    `-- CommandWatchdog
```

Cách tách này giữ phần điều khiển motor độc lập với giao thức truyền thông và
giúp mỗi module có thể được thay đổi hoặc kiểm thử riêng.
