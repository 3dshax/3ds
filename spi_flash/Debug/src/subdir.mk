################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/flash.c \
../src/io.c 

XC_SRCS += \
../src/main.xc \
../src/master.xc 

OBJS += \
./src/flash.o \
./src/io.o \
./src/main.o \
./src/master.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	xcc -O0 -g -Wall -c -std=gnu89 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.xc
	@echo 'Building file: $<'
	@echo 'Invoking: XC Compiler'
	xcc -O0 -g -Wall -c -o "$@" "$<" "../XK-1.xn"
	@echo 'Finished building: $<'
	@echo ' '


