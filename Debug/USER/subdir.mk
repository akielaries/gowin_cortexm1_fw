################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../USER/GOWIN_M1_it.c \
../USER/delay.c \
../USER/gpio.c \
../USER/main.c 

OBJS += \
./USER/GOWIN_M1_it.o \
./USER/delay.o \
./USER/gpio.o \
./USER/main.o 

C_DEPS += \
./USER/GOWIN_M1_it.d \
./USER/delay.d \
./USER/gpio.d \
./USER/main.d 


# Each subdirectory must supply rules for building sources it contributes
USER/%.o: ../USER/%.c USER/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU Arm Cross C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m1 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -I"/home/akiel/GMD_workspace/softcore_fw_example/CORE" -I"/home/akiel/GMD_workspace/softcore_fw_example/SYSTEM" -I"/home/akiel/GMD_workspace/softcore_fw_example/PERIPHERAL/inc" -I"/home/akiel/GMD_workspace/softcore_fw_example/USER" -I"/home/akiel/GMD_workspace/softcore_fw_example/include" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


