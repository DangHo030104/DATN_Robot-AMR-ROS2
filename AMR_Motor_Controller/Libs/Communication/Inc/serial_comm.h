#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* Kieu lenh serial */
typedef enum
{
    SERIAL_CMD_VELOCITY = 0,    // Lenh van toc (linear_x, angular_z)
    SERIAL_CMD_ARM,             // Kích hoạt động cơ
    SERIAL_CMD_DISARM           // Vô hiệu hóa động cơ
} SerialCommandType;

typedef struct
{
    SerialCommandType type;
    float linear_x;
    float angular_z;
    uint32_t received_ms;
} SerialVelocityCommand;

/* Lay lenh ARM, DISARM hoac CMD hop le moi nhat */
uint8_t Serial_Comm_GetCommand(SerialVelocityCommand *command);

/* So lenh CMD hop le da nhan */
uint32_t Serial_Comm_GetValidCmdCount(void);

/* So lenh CMD bi tu choi */
uint32_t Serial_Comm_GetInvalidCmdCount(void);
uint32_t Serial_Comm_GetRxOverflowCount(void);
uint32_t Serial_Comm_GetRxErrorCount(void);


void Serial_Comm_Init(UART_HandleTypeDef *huart);
void Serial_Comm_Process(void);
void Serial_Comm_RxCpltCallback(UART_HandleTypeDef *huart);
void Serial_Comm_ErrorCallback(UART_HandleTypeDef *huart);

HAL_StatusTypeDef Serial_Comm_SendEncoder(int32_t left_count, int32_t right_count);

#endif

