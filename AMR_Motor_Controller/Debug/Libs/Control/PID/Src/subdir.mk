################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libs/Control/PID/Src/pid.c 

OBJS += \
./Libs/Control/PID/Src/pid.o 

C_DEPS += \
./Libs/Control/PID/Src/pid.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/Control/PID/Src/%.o: ../Libs/Control/PID/Src/%.c Libs/Control/PID/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Libs/Communication/Inc -I../Libs/App/Inc -I../Libs/Control/PID/Inc -I../Libs/Control/WheelController/Inc -I../Libs/Control/MotionProfile/Inc -I../Libs/Drivers/Encoder/Inc -I../Libs/Drivers/MotorDriver/Inc -I../Libs/Robot/Inc -I../Libs/Robot/DifferentialDrive/Inc -I../Libs/Safety/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

