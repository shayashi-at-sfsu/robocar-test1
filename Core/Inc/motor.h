#ifndef MOTOR_H
#define MOTOR_H

#include "main.h"

void Motor_Init(void);
void Motor_Stop(void);
void Motor_Forward(uint8_t speed);
void Motor_Backward(uint8_t speed);
void Motor_Left(uint8_t speed);
void Motor_Right(uint8_t speed);
void Motor_Drive(int8_t left, int8_t right);

#endif
