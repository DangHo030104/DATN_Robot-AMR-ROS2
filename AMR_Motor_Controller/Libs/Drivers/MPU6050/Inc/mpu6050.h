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

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    uint32_t sample_ms;     // Thời điểm bắt đầu đọc

    uint8_t valid;          // 1: mẫu hợp lệ, 0: có lỗi đọc
} Mpu6050Sample;

typedef enum
{
    MPU6050_STATE_UNINITIALIZED = 0,
    MPU6050_STATE_INITIALIZED,
    MPU6050_STATE_READY,
    MPU6050_STATE_READING,
    MPU6050_STATE_DISABLED
} Mpu6050State;

// Trạng thái của MPU6050
typedef struct
{
    I2C_HandleTypeDef *i2c;
    GPIO_TypeDef *gpio_port;
    int32_t gyro_bias_x;    // Sai lệch gyro_xyz đo được khi robot đứng yên
    int32_t gyro_bias_y;
    int32_t gyro_bias_z;
    uint32_t async_started_ms;
    uint32_t read_error_count;
    uint32_t last_i2c_error;
    uint32_t recovery_count;
    uint16_t scl_pin;
    uint16_t sda_pin;
    uint8_t async_data[14];
    uint8_t address;
    uint8_t state;
} Mpu6050;

HAL_StatusTypeDef Mpu6050_Init(Mpu6050 *imu, I2C_HandleTypeDef *i2c,
                              GPIO_TypeDef *gpio_port, uint16_t scl_pin,
                              uint16_t sda_pin, uint8_t address);
HAL_StatusTypeDef Mpu6050_CalibrateGyro(Mpu6050 *imu, uint16_t sample_count);
void Mpu6050_Service(Mpu6050 *imu, Mpu6050Sample *sample, uint32_t now_ms);

#endif
