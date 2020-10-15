################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/UART/UART.c 

OBJS += \
./src/UART/UART.o 

C_DEPS += \
./src/UART/UART.d 


# Each subdirectory must supply rules for building sources it contributes
src/UART/%.o: ../src/UART/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -DDEBUG -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -I"E:/STM32_ARM/MY_LIBRARIES/BMP280/StdPeriph_Driver/inc" -I"E:/STM32_ARM/MY_LIBRARIES/BMP280/inc" -I"E:/STM32_ARM/MY_LIBRARIES/BMP280/CMSIS/device" -I"E:/STM32_ARM/MY_LIBRARIES/BMP280/CMSIS/core" -I"E:/STM32_ARM/MY_LIBRARIES/BMP280/src/I2C" -I"E:/STM32_ARM/MY_LIBRARIES/BMP280/src/UART" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


