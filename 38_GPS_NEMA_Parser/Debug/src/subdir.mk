################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Buffer.c \
../src/GPS_NEMA_Parser.c \
../src/main.c 

OBJS += \
./src/Buffer.o \
./src/GPS_NEMA_Parser.o \
./src/main.o 

C_DEPS += \
./src/Buffer.d \
./src/GPS_NEMA_Parser.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


