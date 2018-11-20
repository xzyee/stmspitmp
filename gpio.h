/*
*********************************************************************************************************
*
*	模块名称 : 按键及LED驱动
*	文件名称 : gpio.h
*	版    本 : V1.0
*	说    明 : 按键及LED引脚配置
*	修改记录 :
*		版本号  日期       	作者    说明
*		V1.0	2013-12-12	JOY
*
*********************************************************************************************************
*/
#ifndef __GPIO_H
#define __GPIO_H

#include "stm32arduino.h"
		
#define LEDn		3	// LED数量

#define LED_GPIO_PORT		GPIOC
#define LED_GPIO_CLK		RCC_APB2Periph_GPIOC	
#define LED1_PIN			GPIO_Pin_8
#define LED2_PIN			GPIO_Pin_7
#define LED3_PIN			GPIO_Pin_6  


#define BUTTON_PIN				GPIO_Pin_0
#define BUTTON_GPIO_PORT		GPIOA
#define BUTTON_GPIO_CLK			RCC_APB2Periph_GPIOA
#define BUTTON_EXTI_LINE		EXTI_Line0
#define BUTTON_EXTI_PORT_SOURCE	GPIO_PortSourceGPIOA
#define BUTTON_EXTI_PIN_SOURCE	GPIO_PinSource0
#define BUTTON_EXTI_IRQn		EXTI0_IRQn

extern uint8_t Key_Pressed;

void LED_Init(void);
void LEDOn(Led_TypeDef Led);
void LEDOff(Led_TypeDef Led);
void LEDToggle(Led_TypeDef Led);
void Button_Init(ButtonMode_TypeDef Button_Mode);
uint8_t Button_GetState(void);
void Button_IRQ(void);

#endif

