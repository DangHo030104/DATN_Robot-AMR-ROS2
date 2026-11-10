#ifndef MPU6050_H
#define MPU6050_H

/* MPU6050 Driver */

#include "stm32f1xx_hal.h"
#include <stdint.h>

// Cấu trúc của 1 mẫu MPU6050
typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t temperature;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    uint32_t sample_ms;     // Thời điểm bắt đầu đọc

    uint8_t valid;          // 1: mẫu hợp lệ, 0: có lỗi đọc
} Mpu6050Sample;

// Trạng thái của MPU6050
typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint8_t address;
    int32_t gyro_bias_x;    // Sai lệch gyro_xyz đo được khi robot đứng yên
    int32_t gyro_bias_y;
    int32_t gyro_bias_z;
    uint32_t read_error_count;  
    uint8_t initialized;    // 1: đã khởi tạo, 0: chưa khởi tạo
} Mpu6050;

HAL_StatusTypeDef Mpu6050_Init(Mpu6050 *imu, I2C_HandleTypeDef *i2c, uint8_t address);
HAL_StatusTypeDef Mpu6050_Read(Mpu6050 *imu, Mpu6050Sample *sample);
HAL_StatusTypeDef Mpu6050_CalibrateGyro(Mpu6050 *imu, uint16_t sample_count);

#endif
