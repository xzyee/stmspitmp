#ifndef __STM32ARDUINO_H
#define __STM32ARDUINO_H

#include "stm32f10x.h"

typedef enum 
{
  LED1 = 0,
  LED2 = 1,
  LED3 = 2,
} Led_TypeDef;
  
typedef enum 
{  
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

#include "gpio.h"
#include "usart.h"
#include "delay.h"
#include "calendar_rtc.h"
#include "spi_sd.h"
#endif

