#include "motor.h"
#include "tim.h"

#define IN1_PORT GPIOB
#define IN1_PIN  GPIO_PIN_0

#define IN2_PORT GPIOC
#define IN2_PIN  GPIO_PIN_0

#define IN3_PORT GPIOC
#define IN3_PIN  GPIO_PIN_1

#define IN4_PORT GPIOA
#define IN4_PIN  GPIO_PIN_4

static void Motor_SetSpeed(uint8_t left, uint8_t right)
{
    if (left > 100) left = 100;
    if (right > 100) right = 100;

    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim2);

    __HAL_TIM_SET_COMPARE(
        &htim2, TIM_CHANNEL_1,
        ((period + 1) * left) / 100);

    __HAL_TIM_SET_COMPARE(
        &htim2, TIM_CHANNEL_2,
        ((period + 1) * right) / 100);
}

static void Motor_SetDirection(
    GPIO_PinState in1,
    GPIO_PinState in2,
    GPIO_PinState in3,
    GPIO_PinState in4)
{
    HAL_GPIO_WritePin(IN1_PORT, IN1_PIN, in1);
    HAL_GPIO_WritePin(IN2_PORT, IN2_PIN, in2);
    HAL_GPIO_WritePin(IN3_PORT, IN3_PIN, in3);
    HAL_GPIO_WritePin(IN4_PORT, IN4_PIN, in4);
}

void Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    Motor_Stop();
}

void Motor_Stop(void)
{
    Motor_SetSpeed(0, 0);

    Motor_SetDirection(
        GPIO_PIN_RESET, GPIO_PIN_RESET,
        GPIO_PIN_RESET, GPIO_PIN_RESET);
}

static uint8_t Motor_MapPWM(int16_t command)
{
    uint16_t magnitude =
        (command < 0) ? -command : command;

    if (magnitude == 0)
        return 0;

    if (magnitude > 100)
        magnitude = 100;

    /* 40% minimum starting duty cycle */
    return 40U + (uint8_t)((magnitude * 60U) / 100U);
}

void Motor_Drive(int8_t left, int8_t right)
{
    uint8_t leftPWM = Motor_MapPWM(left);
    uint8_t rightPWM = Motor_MapPWM(right);

    /* Disable both bridges before changing direction. */
    Motor_SetSpeed(0, 0);

    /*
     * Confirmed physical polarity:
     * Right forward: IN1=1, IN2=0
     * Left forward:  IN3=0, IN4=1
     */

    Motor_SetDirection(
        right > 0 ? GPIO_PIN_SET   : GPIO_PIN_RESET,
        right < 0 ? GPIO_PIN_SET   : GPIO_PIN_RESET,
        left  < 0 ? GPIO_PIN_SET   : GPIO_PIN_RESET,
        left  > 0 ? GPIO_PIN_SET   : GPIO_PIN_RESET
    );

    /*
     * Existing Motor_SetSpeed() arguments:
     * CH1 = right wheels, CH2 = left wheels.
     */
    Motor_SetSpeed(rightPWM, leftPWM);
}
