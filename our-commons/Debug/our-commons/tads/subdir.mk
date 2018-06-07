################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../our-commons/tads/tads.c 

OBJS += \
./our-commons/tads/tads.o 

C_DEPS += \
./our-commons/tads/tads.d 


# Each subdirectory must supply rules for building sources it contributes
our-commons/tads/%.o: ../our-commons/tads/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


