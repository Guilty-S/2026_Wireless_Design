//
// Created by Administrator on 2026/6/29.
//
#include "bsp.h"
#include "main.h"

#include "stm32f1xx_hal.h"

void LED_Toggle(){
    HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
}