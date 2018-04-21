################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sockets/client.c \
../sockets/server.c 

OBJS += \
./sockets/client.o \
./sockets/server.o 

C_DEPS += \
./sockets/client.d \
./sockets/server.d 


# Each subdirectory must supply rules for building sources it contributes
sockets/%.o: ../sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


