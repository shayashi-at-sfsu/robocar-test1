/*
 * joystick.c
 *
 *  Created on: Nov 15, 2026
 *      Author: shige
 */


#include "joystick.h"
#include "adc.h"
#include <stdio.h>

#define JOYSTICK_X_CENTER  2015
#define JOYSTICK_Y_CENTER  2008
#define JOYSTICK_DEADBAND  120

static volatile uint16_t adc_buffer[2] = {0, 0};
static volatile uint8_t adc_ready = 0;

static int8_t Joystick_MapAxis(uint16_t raw, int32_t center)
{
    int32_t value = (int32_t)raw - center;
    int32_t output;

    if (value > -JOYSTICK_DEADBAND &&
        value < JOYSTICK_DEADBAND)
        return 0;

    if (value > 0)
    {
        output = (value - JOYSTICK_DEADBAND) * 100 /
                 (4095 - center - JOYSTICK_DEADBAND);
    }
    else
    {
        output = (value + JOYSTICK_DEADBAND) * 100 /
                 (center - JOYSTICK_DEADBAND);
    }

    if (output > 100) output = 100;
    if (output < -100) output = -100;

    return (int8_t)output;
}

HAL_StatusTypeDef Joystick_Init(void)
{
    HAL_StatusTypeDef status;

    adc_ready = 0;

    status = HAL_ADCEx_Calibration_Start(
        &hadc1, ADC_SINGLE_ENDED);

    if (status != HAL_OK)
    {
        printf("ADC calibration failed: %d\r\n", status);
        return status;
    }

    status = HAL_ADC_Start_DMA(
        &hadc1,
        (uint32_t *)adc_buffer,
        2);

    if (status == HAL_OK)
    {
        __HAL_DMA_DISABLE_IT(
            hadc1.DMA_Handle,
            DMA_IT_HT);
    } else {
        printf("ADC DMA start failed: %d\r\n", status);
        return status;
    }

    return status;
}

HAL_StatusTypeDef Joystick_Read(Joystick_Data_t *data)
{
    if (data == NULL)
        return HAL_ERROR;

    if (!adc_ready)
        return HAL_BUSY;

    uint16_t x = adc_buffer[0];
    uint16_t y = adc_buffer[1];

    data->x_raw = x;
    data->y_raw = y;

    data->steering = Joystick_MapAxis(
        x, JOYSTICK_X_CENTER);

    data->throttle = Joystick_MapAxis(
        y, JOYSTICK_Y_CENTER);

    return HAL_OK;
}

/* Called when both ADC channels have been converted. */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_ready = 1;
    }
}
