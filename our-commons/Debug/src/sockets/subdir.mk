################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/sockets/client.c \
../src/sockets/server.c 

OBJS += \
./src/sockets/client.o \
./src/sockets/server.o 

C_DEPS += \
./src/sockets/client.d \
./src/sockets/server.d 


# Each subdirectory must supply rules for building sources it contributes
src/sockets/%.o: ../src/sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


