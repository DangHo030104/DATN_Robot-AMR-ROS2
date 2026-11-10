#include "mpu6050.h"

/* Địa chỉ thanh ghi MPU6050 */
#define MPU6050_REG_SMPLRT_DIV     0x19U    // Chia tần số lấy mẫu
#define MPU6050_REG_CONFIG         0x1AU    // Cấu hình bô lọc DLPF
#define MPU6050_REG_GYRO_CONFIG    0x1BU    // Cấu hình gyro
#define MPU6050_REG_ACCEL_CONFIG   0x1CU    // Cấu hình accel
#define MPU6050_REG_ACCEL_XOUT_H   0x3BU    // Đọc burst 14 byte từ 0x3B
#define MPU6050_REG_PWR_MGMT_1     0x6BU    // Quản lý nguồn: bật/tắt và chọn clock cảm biến.
#define MPU6050_REG_WHO_AM_I       0x75U    // Kiểm tra đúng Slave MPU6050

#define MPU6050_WHO_AM_I_VALUE     0x68U
#define MPU6050_READ_TIMEOUT_MS    5U
#define MPU6050_WRITE_TIMEOUT_MS   5U
#define MPU6050_CALIBRATION_MS     10U


// Ghi 1 byte vào thanh ghi của MPU6050
static HAL_StatusTypeDef Mpu6050_WriteRegister(Mpu6050 *imu, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(imu->i2c, (uint16_t)(imu->address << 1), reg, I2C_MEMADD_SIZE_8BIT, &value, 1U, MPU6050_WRITE_TIMEOUT_MS);
}


// Đọc nhiều byte từ thanh ghi MPU6050
static HAL_StatusTypeDef Mpu6050_ReadRegisters(Mpu6050 *imu, uint8_t reg, uint8_t *data, uint16_t length)
{
    return HAL_I2C_Mem_Read(imu->i2c, (uint16_t)(imu->address << 1), reg, I2C_MEMADD_SIZE_8BIT, data, length, MPU6050_READ_TIMEOUT_MS);
}


/* Ghép 2 byte liên tiếp từ MPU6050 thành int16_t
 * MPU6050 truyền byte cao trước: ACCEL_XOUT_H, ACCEL_XOUT_L */
static int16_t Mpu6050_ParseInt16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}


// Giới hạn giá trị int32_t về phạm vi int16_t
static int16_t Mpu6050_ClampInt16(int32_t value)
{
    if (value > INT16_MAX)
        return INT16_MAX;
    if (value < INT16_MIN)
        return INT16_MIN;
    return (int16_t)value;
}


/* Đọc 1 mẫu 14 byte:
 * 	data[0..1]   accel X
	data[2..3]   accel Y
	data[4..5]   accel Z
	data[6..7]   temperature
	data[8..9]   gyro X
	data[10..11] gyro Y
	data[12..13] gyro Z, chưa trừ bias gyro */
static HAL_StatusTypeDef Mpu6050_ReadRaw(Mpu6050 *imu, Mpu6050Sample *sample)
{
    uint8_t data[14];
    uint32_t sample_ms = HAL_GetTick();

    HAL_StatusTypeDef status = Mpu6050_ReadRegisters(imu, MPU6050_REG_ACCEL_XOUT_H, data, sizeof(data));

    if (status != HAL_OK)
    {
        imu->read_error_count++;
        sample->valid = 0;
        return status;
    }

    sample->accel_x = Mpu6050_ParseInt16(&data[0]);
    sample->accel_y = Mpu6050_ParseInt16(&data[2]);
    sample->accel_z = Mpu6050_ParseInt16(&data[4]);
    sample->temperature = Mpu6050_ParseInt16(&data[6]);
    sample->gyro_x = Mpu6050_ParseInt16(&data[8]);
    sample->gyro_y = Mpu6050_ParseInt16(&data[10]);
    sample->gyro_z = Mpu6050_ParseInt16(&data[12]);
    sample->sample_ms = sample_ms;
    sample->valid = 1;

    return HAL_OK;
}


HAL_StatusTypeDef Mpu6050_Init(Mpu6050 *imu, I2C_HandleTypeDef *i2c, uint8_t address)
{
    uint8_t who_am_i = 0U;

    if (imu == NULL || i2c == NULL || (address != 0x68U && address != 0x69U))		// 0x68: chân AD0 thấp ; 0x69: chân AD0 cao
        return HAL_ERROR;

    imu->i2c = i2c;
    imu->address = address;
    imu->gyro_bias_x = 0;
    imu->gyro_bias_y = 0;
    imu->gyro_bias_z = 0;
    imu->read_error_count = 0U;
    imu->initialized = 0;

    if (Mpu6050_ReadRegisters(imu, MPU6050_REG_WHO_AM_I, &who_am_i, 1) != HAL_OK || who_am_i != MPU6050_WHO_AM_I_VALUE)
        return HAL_ERROR;

    /* Wake the sensor and allow its clock to settle before configuration
     * 0x01: Xóa bit sleep và chọn PLL của gyro X làm clock */
    if (Mpu6050_WriteRegister(imu, MPU6050_REG_PWR_MGMT_1, 0x01U) != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(10);

    /* DLPF_CFG = 3
     * Sample rate = 1000 / (1 + 9) = 100 Hz
     * Gyro FS_SEL = 0  → ±250 °/s, 131 LSB/(°/s)
     * Accel AFS_SEL = 0 → ±2 g, 16384 LSB/g */
    if (Mpu6050_WriteRegister(imu, MPU6050_REG_CONFIG, 0x03U) != HAL_OK ||
        Mpu6050_WriteRegister(imu, MPU6050_REG_SMPLRT_DIV, 9U) != HAL_OK ||
        Mpu6050_WriteRegister(imu, MPU6050_REG_GYRO_CONFIG, 0x00U) != HAL_OK ||
        Mpu6050_WriteRegister(imu, MPU6050_REG_ACCEL_CONFIG, 0x00U) != HAL_OK)
        return HAL_ERROR;

    imu->initialized = 1;
    return HAL_OK;
}


// Đọc mẫu đã hiệu chỉnh gyro
HAL_StatusTypeDef Mpu6050_Read(Mpu6050 *imu, Mpu6050Sample *sample)
{
    if (imu == NULL || sample == NULL || !imu->initialized)
        return HAL_ERROR;

    if (Mpu6050_ReadRaw(imu, sample) != HAL_OK)
        return HAL_ERROR;

    // Trừ bias gyro đo được khi robot đứng yên
    sample->gyro_x = Mpu6050_ClampInt16((int32_t)sample->gyro_x - imu->gyro_bias_x);
    sample->gyro_y = Mpu6050_ClampInt16((int32_t)sample->gyro_y - imu->gyro_bias_y);
    sample->gyro_z = Mpu6050_ClampInt16((int32_t)sample->gyro_z - imu->gyro_bias_z);

    return HAL_OK;
}


// Hiệu chuẩn gyro khi khởi động
HAL_StatusTypeDef Mpu6050_CalibrateGyro(Mpu6050 *imu, uint16_t sample_count)
{
    Mpu6050Sample sample = {0};
    int64_t sum_x = 0;
    int64_t sum_y = 0;
    int64_t sum_z = 0;

    if (imu == NULL || !imu->initialized || sample_count == 0U)
        return HAL_ERROR;

    for (uint16_t i = 0U; i < sample_count; ++i)
    {
        if (Mpu6050_ReadRaw(imu, &sample) != HAL_OK)
            return HAL_ERROR;

        sum_x += sample.gyro_x;
        sum_y += sample.gyro_y;
        sum_z += sample.gyro_z;
        HAL_Delay(MPU6050_CALIBRATION_MS);
    }

    // Tính bias gyro bằng trung bình cộng của các mẫu
    imu->gyro_bias_x = (int32_t)(sum_x / sample_count);
    imu->gyro_bias_y = (int32_t)(sum_y / sample_count);
    imu->gyro_bias_z = (int32_t)(sum_z / sample_count);

    return HAL_OK;
}
