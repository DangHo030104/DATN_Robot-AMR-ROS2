#include "serial_comm.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>


#define SERIAL_RX_BUFFER_SIZE 128U
#define SERIAL_LINE_SIZE      64U

#define SERIAL_MAX_LINEAR   0.10f
#define SERIAL_MAX_ANGULAR  0.50f


static UART_HandleTypeDef *serial_uart = NULL;

static uint8_t rx_byte;

/* ===== Ring Buffer ===== */
static volatile uint8_t rx_buffer[SERIAL_RX_BUFFER_SIZE];

static volatile uint8_t rx_head = 0;		// Vị trí ghi byte tiếp theo.
static volatile uint8_t rx_tail = 0;		// Vị trí đọc byte tiếp theo.

static char line_buffer[SERIAL_LINE_SIZE];	// Ghép các byte thành chuỗi CMD
static uint8_t line_length = 0;				// Vị trí ký tự đã ghép
static uint8_t line_overflow = 0;

static volatile uint32_t rx_overflow_count = 0;		// Đếm số byte bị bỏ do Ring Buffer đã đầy
static volatile uint32_t rx_error_count = 0;
static volatile uint8_t rx_discard_line = 0;

static uint8_t cmd_pending = 0;
static SerialVelocityCommand latest_cmd;

static uint32_t valid_cmd_count = 0;
static uint32_t invalid_cmd_count = 0;

static uint8_t Serial_ParseCommand(const char *line, SerialVelocityCommand *command);

/* Khoi tao module UART */
void Serial_Comm_Init(UART_HandleTypeDef *huart)
{
    serial_uart = huart;

    rx_head = 0;
    rx_tail = 0;
    line_length = 0;
    line_overflow = 0;
    rx_discard_line = 0;
    rx_overflow_count = 0;
    rx_error_count = 0;
    cmd_pending = 0;
    valid_cmd_count = 0;
    invalid_cmd_count = 0;

    HAL_UART_Receive_IT(serial_uart, &rx_byte, 1);
}


/* Callback nhan mot byte */
void Serial_Comm_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != serial_uart)
        return;

    // % 128 tạo tính chất vòng (Ring buffer), vd: rx_head = 127 → next_head = 0.
    uint8_t next_head = (uint8_t)((rx_head + 1) % SERIAL_RX_BUFFER_SIZE);

    // Kiểm tra buffer đã đầy?
    if (next_head != rx_tail)
    {
        rx_buffer[rx_head] = rx_byte;
        rx_head = next_head;
    }
    else
    {
        rx_overflow_count++;
        rx_discard_line = 1;
    }

    /* Tiep tuc nhan byte tiep theo */
    HAL_UART_Receive_IT(serial_uart, &rx_byte, 1);
}

void Serial_Comm_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != serial_uart)
        return;

    rx_error_count++;
    rx_discard_line = 1;
    HAL_UART_Receive_IT(serial_uart, &rx_byte, 1);
}

/* Xu ly du lieu ngoai ngat UART */
void Serial_Comm_Process(void)
{
	// Còn byte chưa xử lý
    while (rx_tail != rx_head)
    {
        uint8_t data = rx_buffer[rx_tail];

        rx_tail = (uint8_t)((rx_tail + 1) % SERIAL_RX_BUFFER_SIZE);

        if (rx_discard_line)
        {
            line_length = 0;
            line_overflow = 0;
            if (data == '\n')
                rx_discard_line = 0;
            continue;
        }

        if (data == '\n')
        {
            if (!line_overflow)
            {
                line_buffer[line_length] = '\0';

                /* Bo ky tu CR neu co */
                if (line_length > 0 && line_buffer[line_length - 1] == '\r')
                {
                    line_buffer[line_length - 1] = '\0';
                }

                SerialVelocityCommand command;
                if (Serial_ParseCommand(line_buffer, &command))
                {
                    latest_cmd = command;
                    cmd_pending = 1;
                    valid_cmd_count++;
                }
                else
                {
                    invalid_cmd_count++;
                }
            }

            line_length = 0;
            line_overflow = 0;
        }
        else if (!line_overflow)
        {
            if (line_length < SERIAL_LINE_SIZE - 1)
            {
                line_buffer[line_length++] = (char)data;
            }
            else
            {
                line_length = 0;
                line_overflow = 1;
                invalid_cmd_count++;
            }
        }
    }
}


/* Gui tong xung Encoder cua 2 banh.
 * Dinh dang: ENC,left,right\n */
HAL_StatusTypeDef Serial_Comm_SendEncoder(int32_t left_count, int32_t right_count)
{
    if (serial_uart == NULL)
    {
        return HAL_ERROR;
    }

    char tx_buffer[64];

    int length = snprintf(tx_buffer, sizeof(tx_buffer), "ENC,%ld,%ld\n", (long)left_count, (long)right_count);

    if (length <= 0 || length >= (int)sizeof(tx_buffer))
    {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit(serial_uart, (uint8_t *)tx_buffer, (uint16_t)length, 5);
}


uint8_t Serial_Comm_GetCommand(SerialVelocityCommand *command)
{
    if (command == NULL || !cmd_pending)
        return 0;

    *command = latest_cmd;
    cmd_pending = 0;

    return 1;
}

uint32_t Serial_Comm_GetValidCmdCount(void)
{
    return valid_cmd_count;
}

uint32_t Serial_Comm_GetInvalidCmdCount(void)
{
    return invalid_cmd_count;
}

uint32_t Serial_Comm_GetRxOverflowCount(void)
{
    return rx_overflow_count;
}

uint32_t Serial_Comm_GetRxErrorCount(void)
{
    return rx_error_count;
}


static uint8_t Serial_ParseCommand(const char *line, SerialVelocityCommand *command)
{
    if (line == NULL || command == NULL)
    	return 0;

    if (strcmp(line, "ARM") == 0)
    {
        command->type = SERIAL_CMD_ARM;
        command->linear_x = 0.0f;
        command->angular_z = 0.0f;
        command->received_ms = HAL_GetTick();
        return 1;
    }

    if (strcmp(line, "DISARM") == 0)
    {
        command->type = SERIAL_CMD_DISARM;
        command->linear_x = 0.0f;
        command->angular_z = 0.0f;
        command->received_ms = HAL_GetTick();
        return 1;
    }

    if (strncmp(line, "CMD,", 4) != 0)
        return 0;


    const char *cursor = line + 4;
    char *endptr;

    errno = 0;
    float v = strtof(cursor, &endptr);

    if (endptr == cursor || errno == ERANGE || *endptr != ',')
        return 0;

    cursor = endptr + 1;

    errno = 0;
    float w = strtof(cursor, &endptr);

    if (endptr == cursor || errno == ERANGE || *endptr != '\0')
        return 0;

    if (!isfinite(v) || !isfinite(w))
        return 0;

    if (fabsf(v) > SERIAL_MAX_LINEAR || fabsf(w) > SERIAL_MAX_ANGULAR)
        return 0;

    command->type = SERIAL_CMD_VELOCITY;
    command->linear_x = v;
    command->angular_z = w;
    command->received_ms = HAL_GetTick();

    return 1;
}

