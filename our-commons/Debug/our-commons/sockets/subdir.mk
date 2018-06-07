################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../our-commons/sockets/client.c \
../our-commons/sockets/server.c 

OBJS += \
./our-commons/sockets/client.o \
./our-commons/sockets/server.o 

C_DEPS += \
./our-commons/sockets/client.d \
./our-commons/sockets/server.d 


# Each subdirectory must supply rules for building sources it contributes
our-commons/sockets/%.o: ../our-commons/sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


