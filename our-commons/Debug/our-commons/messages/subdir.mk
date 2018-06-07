################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../our-commons/messages/operation_codes.c \
../our-commons/messages/serialization.c 

OBJS += \
./our-commons/messages/operation_codes.o \
./our-commons/messages/serialization.o 

C_DEPS += \
./our-commons/messages/operation_codes.d \
./our-commons/messages/serialization.d 


# Each subdirectory must supply rules for building sources it contributes
our-commons/messages/%.o: ../our-commons/messages/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


