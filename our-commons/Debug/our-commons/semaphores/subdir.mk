################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../our-commons/semaphores/semaphores.c 

OBJS += \
./our-commons/semaphores/semaphores.o 

C_DEPS += \
./our-commons/semaphores/semaphores.d 


# Each subdirectory must supply rules for building sources it contributes
our-commons/semaphores/%.o: ../our-commons/semaphores/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


