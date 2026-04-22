################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Inc/stm32f1xx_hal_i2c.c 

OBJS += \
./Core/Inc/stm32f1xx_hal_i2c.o 

C_DEPS += \
./Core/Inc/stm32f1xx_hal_i2c.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Inc/%.o Core/Inc/%.su Core/Inc/%.cyclo: ../Core/Inc/%.c Core/Inc/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -IC:/Users/KlausKupferschmid/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.6/Drivers/STM32F1xx_HAL_Driver/Inc -IC:/Users/KlausKupferschmid/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.6/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -IC:/Users/KlausKupferschmid/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.6/Drivers/CMSIS/Device/ST/STM32F1xx/Include -IC:/Users/KlausKupferschmid/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.6/Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Inc

clean-Core-2f-Inc:
	-$(RM) ./Core/Inc/stm32f1xx_hal_i2c.cyclo ./Core/Inc/stm32f1xx_hal_i2c.d ./Core/Inc/stm32f1xx_hal_i2c.o ./Core/Inc/stm32f1xx_hal_i2c.su

.PHONY: clean-Core-2f-Inc

