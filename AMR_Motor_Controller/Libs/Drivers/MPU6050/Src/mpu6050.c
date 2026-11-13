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
#define MPU6050_READ_PERIOD_MS     10U
#define MPU6050_READ_DEADLINE_MS   10U


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


// Clamp giá trị int32_t về int16_t
static int16_t Mpu6050_ClampInt16(int32_t value)
{
    if (value > INT16_MAX)
        return INT16_MAX;
    if (value < INT16_MIN)
        return INT16_MIN;
    return (int16_t)value;
}


// Parse mẫu runtime
static void Mpu6050_ParseSample(Mpu6050 *imu, Mpu6050Sample *sample, const uint8_t *data, uint32_t sample_ms)
{
    sample->accel_x = Mpu6050_ParseInt16(&data[0]);
    sample->accel_y = Mpu6050_ParseInt16(&data[2]);
    sample->accel_z = Mpu6050_ParseInt16(&data[4]);

    /* Hiệu chuẩn Gyro = gyro_raw − bias lúc đứng yên */
    sample->gyro_x = Mpu6050_ClampInt16((int32_t)Mpu6050_ParseInt16(&data[8]) - imu->gyro_bias_x);
    sample->gyro_y = Mpu6050_ClampInt16((int32_t)Mpu6050_ParseInt16(&data[10]) - imu->gyro_bias_y);
    sample->gyro_z = Mpu6050_ClampInt16((int32_t)Mpu6050_ParseInt16(&data[12]) - imu->gyro_bias_z);

    sample->sample_ms = sample_ms;
    sample->valid = 1;
}


/* Đọc 1 mẫu (raw) 14 byte:
 * 	data[0..1]   accel X
 *	data[2..3]   accel Y
 *	data[4..5]   accel Z
 *	data[6..7]   temperature
 *	data[8..9]   gyro X
 *	data[10..11] gyro Y
 *	data[12..13] gyro Z */
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
    sample->gyro_x = Mpu6050_ParseInt16(&data[8]);
    sample->gyro_y = Mpu6050_ParseInt16(&data[10]);
    sample->gyro_z = Mpu6050_ParseInt16(&data[12]);
    sample->sample_ms = sample_ms;
    sample->valid = 1;

    return HAL_OK;
}


HAL_StatusTypeDef Mpu6050_Init(Mpu6050 *imu, I2C_HandleTypeDef *i2c,
                              GPIO_TypeDef *gpio_port, uint16_t scl_pin,
                              uint16_t sda_pin, uint8_t address)
{
    uint8_t who_am_i = 0U;

    if (imu == NULL || i2c == NULL || i2c->Instance != I2C2 || gpio_port == NULL ||
        (address != 0x68U && address != 0x69U))		// 0x68: chân AD0 thấp ; 0x69: chân AD0 cao
        return HAL_ERROR;

    imu->i2c = i2c;
    imu->gpio_port = gpio_port;
    imu->scl_pin = scl_pin;
    imu->sda_pin = sda_pin;
    imu->address = address;
    imu->gyro_bias_x = 0;
    imu->gyro_bias_y = 0;
    imu->gyro_bias_z = 0;
    imu->async_started_ms = 0U;
    imu->read_error_count = 0U;
    imu->last_i2c_error = HAL_I2C_ERROR_NONE;
    imu->recovery_count = 0U;
    imu->state = MPU6050_STATE_UNINITIALIZED;

    if (Mpu6050_ReadRegisters(imu, MPU6050_REG_WHO_AM_I, &who_am_i, 1) != HAL_OK || who_am_i != MPU6050_WHO_AM_I_VALUE)
        return HAL_ERROR;

    /* 0x01: Xóa bit sleep và chọn PLL của gyro X làm clock */
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

    imu->state = MPU6050_STATE_INITIALIZED;
    return HAL_OK;
}


// Hiệu chuẩn gyro khi khởi động
HAL_StatusTypeDef Mpu6050_CalibrateGyro(Mpu6050 *imu, uint16_t sample_count)
{
    Mpu6050Sample sample = {0};
    int64_t sum_x = 0;
    int64_t sum_y = 0;
    int64_t sum_z = 0;

    if (imu == NULL || imu->state != MPU6050_STATE_INITIALIZED || sample_count == 0U)
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
    imu->async_started_ms = HAL_GetTick();
    imu->state = MPU6050_STATE_READY;

    return HAL_OK;
}


// Delay ngắn cho recovery -> Tạo k/c ngắn giữa các lần đổi trạng thái SCL/SDA
static void Mpu6050_BusDelay(void)
{
    for (volatile uint32_t count = 0U; count < 128U; ++count)
        __NOP();
}


static HAL_StatusTypeDef Mpu6050_RecoverBus(Mpu6050 *imu)
{
    GPIO_InitTypeDef gpio = {0};
    GPIO_PinState scl_state;
    GPIO_PinState sda_state;

    /* 1. Dừng peripheral và interrupt */
    HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C2_ER_IRQn);
    __HAL_I2C_DISABLE_IT(imu->i2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
    __HAL_I2C_DISABLE(imu->i2c);

    /* 2. I²C sử dụng open-drain:
		- Ghi RESET: chủ động kéo SCL/SDA xuống thấp.
		- Ghi SET: nhả SCL/SDA, điện trở pull-up kéo lên cao */
    gpio.Pin = imu->scl_pin | imu->sda_pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(imu->gpio_port, &gpio);

    /* 3. Nhả SCL/SDA */
    HAL_GPIO_WritePin(imu->gpio_port, imu->scl_pin | imu->sda_pin, GPIO_PIN_SET);
    Mpu6050_BusDelay();

    for (uint8_t pulse = 0U; pulse < 9U; ++pulse)
    {
        HAL_GPIO_WritePin(imu->gpio_port, imu->scl_pin, GPIO_PIN_RESET);
        Mpu6050_BusDelay();
        HAL_GPIO_WritePin(imu->gpio_port, imu->scl_pin, GPIO_PIN_SET);
        Mpu6050_BusDelay();
    }

    HAL_GPIO_WritePin(imu->gpio_port, imu->sda_pin, GPIO_PIN_RESET);
    Mpu6050_BusDelay();
    HAL_GPIO_WritePin(imu->gpio_port, imu->scl_pin, GPIO_PIN_RESET);
    Mpu6050_BusDelay();
    HAL_GPIO_WritePin(imu->gpio_port, imu->scl_pin, GPIO_PIN_SET);
    Mpu6050_BusDelay();
    HAL_GPIO_WritePin(imu->gpio_port, imu->sda_pin, GPIO_PIN_SET);
    Mpu6050_BusDelay();

    scl_state = HAL_GPIO_ReadPin(imu->gpio_port, imu->scl_pin);
    sda_state = HAL_GPIO_ReadPin(imu->gpio_port, imu->sda_pin);

    gpio.Mode = GPIO_MODE_AF_OD;
    HAL_GPIO_Init(imu->gpio_port, &gpio);

    SET_BIT(imu->i2c->Instance->CR1, I2C_CR1_SWRST);
    CLEAR_BIT(imu->i2c->Instance->CR1, I2C_CR1_SWRST);
    __HAL_RCC_I2C2_FORCE_RESET();
    __HAL_RCC_I2C2_RELEASE_RESET();

    HAL_NVIC_ClearPendingIRQ(I2C2_EV_IRQn);
    HAL_NVIC_ClearPendingIRQ(I2C2_ER_IRQn);
    imu->i2c->State = HAL_I2C_STATE_RESET;
    imu->i2c->Lock = HAL_UNLOCKED;

    if (HAL_I2C_Init(imu->i2c) != HAL_OK)
    {
        HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);
        HAL_NVIC_DisableIRQ(I2C2_ER_IRQn);
        return HAL_ERROR;
    }

    imu->recovery_count++;
    if (scl_state != GPIO_PIN_SET || sda_state != GPIO_PIN_SET)
        return HAL_ERROR;

    return HAL_OK;
}


static void Mpu6050_HandleReadFailure(Mpu6050 *imu, Mpu6050Sample *sample)
{
    sample->valid = 0;
    imu->last_i2c_error = HAL_I2C_GetError(imu->i2c);

    if (Mpu6050_RecoverBus(imu) != HAL_OK)
        imu->state = MPU6050_STATE_DISABLED;
    else
        imu->state = MPU6050_STATE_READY;
}


// Bắt đầu đọc interrupt
static HAL_StatusTypeDef Mpu6050_StartReadAsync(Mpu6050 *imu)
{
    HAL_StatusTypeDef status;

    if (imu == NULL) return HAL_ERROR;
    if (imu->state != MPU6050_STATE_READY) return HAL_BUSY;

    imu->async_started_ms = HAL_GetTick();
    status = HAL_I2C_Mem_Read_IT(imu->i2c,
                                 (uint16_t)(imu->address << 1),
                                 MPU6050_REG_ACCEL_XOUT_H,
                                 I2C_MEMADD_SIZE_8BIT,
                                 imu->async_data,
                                 sizeof(imu->async_data));
    if (status == HAL_OK)
        imu->state = MPU6050_STATE_READING;

    return status;
}


void Mpu6050_Service(Mpu6050 *imu, Mpu6050Sample *sample, uint32_t now_ms)
{
	/* Nếu IMU bị vô hiệu hóa thì 0 blocking và không recovery vô hạn */
    if (imu == NULL || sample == NULL || (imu->state != MPU6050_STATE_READY && imu->state != MPU6050_STATE_READING))
        return;

    if (imu->state == MPU6050_STATE_READING)
    {
        if (HAL_I2C_GetState(imu->i2c) == HAL_I2C_STATE_READY)
        {
            if (HAL_I2C_GetError(imu->i2c) != HAL_I2C_ERROR_NONE)
            {
                imu->read_error_count++;
                Mpu6050_HandleReadFailure(imu, sample);
                return;
            }

            Mpu6050_ParseSample(imu, sample, imu->async_data, imu->async_started_ms);
            imu->state = MPU6050_STATE_READY;
        }
        else if ((uint32_t)(now_ms - imu->async_started_ms) >= MPU6050_READ_DEADLINE_MS)
        {
            imu->read_error_count++;
            imu->i2c->ErrorCode |= HAL_I2C_ERROR_TIMEOUT;
            Mpu6050_HandleReadFailure(imu, sample);
            return;
        }
    }

    if (imu->state == MPU6050_STATE_READY && (uint32_t)(now_ms - imu->async_started_ms) >= MPU6050_READ_PERIOD_MS)
    {
        if (Mpu6050_StartReadAsync(imu) != HAL_OK)
            Mpu6050_HandleReadFailure(imu, sample);
    }
}
