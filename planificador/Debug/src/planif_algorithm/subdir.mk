################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/planif_algorithm/planif_algorithm.c 

OBJS += \
./src/planif_algorithm/planif_algorithm.o 

C_DEPS += \
./src/planif_algorithm/planif_algorithm.d 


# Each subdirectory must supply rules for building sources it contributes
src/planif_algorithm/%.o: ../src/planif_algorithm/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-1c-youKnowNothing/our-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


