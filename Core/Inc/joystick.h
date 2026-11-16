/*
 * joystick.h
 *
 *  Created on: Nov 15, 2026
 *      Author: shige
 */

#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "main.h"
#include <stdint.h>

typedef struct
{
    uint16_t x_raw;
    uint16_t y_raw;
    int8_t steering;
    int8_t throttle;
} Joystick_Data_t;

HAL_StatusTypeDef Joystick_Init(void);
HAL_StatusTypeDef Joystick_Read(Joystick_Data_t *data);

#endif
