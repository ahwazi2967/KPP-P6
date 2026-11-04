################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/cyber_gear_driver/cyber_gear.c 

OBJS += \
./Core/cyber_gear_driver/cyber_gear.o 

C_DEPS += \
./Core/cyber_gear_driver/cyber_gear.d 


# Each subdirectory must supply rules for building sources it contributes
Core/cyber_gear_driver/%.o Core/cyber_gear_driver/%.su Core/cyber_gear_driver/%.cyclo: ../Core/cyber_gear_driver/%.c Core/cyber_gear_driver/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"/home/ahwazi/ELC_FW_MAGANG_P5_LITE/Core/cyber_gear_driver" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-cyber_gear_driver

clean-Core-2f-cyber_gear_driver:
	-$(RM) ./Core/cyber_gear_driver/cyber_gear.cyclo ./Core/cyber_gear_driver/cyber_gear.d ./Core/cyber_gear_driver/cyber_gear.o ./Core/cyber_gear_driver/cyber_gear.su

.PHONY: clean-Core-2f-cyber_gear_driver

