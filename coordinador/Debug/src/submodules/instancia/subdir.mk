################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/submodules/instancia/instanciaFunctions.c 

OBJS += \
./src/submodules/instancia/instanciaFunctions.o 

C_DEPS += \
./src/submodules/instancia/instanciaFunctions.d 


# Each subdirectory must supply rules for building sources it contributes
src/submodules/instancia/%.o: ../src/submodules/instancia/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-1c-youKnowNothing/our-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


