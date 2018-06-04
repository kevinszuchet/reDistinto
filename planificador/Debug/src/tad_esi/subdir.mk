################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/tad_esi/tad_esi.c 

OBJS += \
./src/tad_esi/tad_esi.o 

C_DEPS += \
./src/tad_esi/tad_esi.d 


# Each subdirectory must supply rules for building sources it contributes
src/tad_esi/%.o: ../src/tad_esi/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-1c-youKnowNothing/our-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


