################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tests_functions/instancias.c 

OBJS += \
./tests_functions/instancias.o 

C_DEPS += \
./tests_functions/instancias.d 


# Each subdirectory must supply rules for building sources it contributes
tests_functions/%.o: ../tests_functions/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-1c-youKnowNothing/our-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


