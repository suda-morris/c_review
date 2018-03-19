################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Buffer.c \
../src/GSM_AT_Parser.c \
../src/gsm_ll.c \
../src/gsm_sys.c \
../src/main.c 

OBJS += \
./src/Buffer.o \
./src/GSM_AT_Parser.o \
./src/gsm_ll.o \
./src/gsm_sys.o \
./src/main.o 

C_DEPS += \
./src/Buffer.d \
./src/GSM_AT_Parser.d \
./src/gsm_ll.d \
./src/gsm_sys.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


