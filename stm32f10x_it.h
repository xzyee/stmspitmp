#ifndef __STM32F10x_IT_H
#define __STM32F10x_IT_H

#include "stm32f10x.h"

#define RXBUFFERSIZE   256


void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

void USB_LP_CAN1_RX0_IRQHandler(void);

void EXTI9_5_IRQHandler(void);

void USBWakeUp_IRQHandler(void);

void TIM3_IRQHandler(void);

#endif /* __STM32F10x_IT_H */

/**********************************END OF FILE**********************************/
