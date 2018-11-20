/*
*********************************************************************************************************
*	                                  
*	模块名称 : 中断模块
*	文件名称 : stm32f10x_it.c
*	固件库版本: V3.5.0
*	说    明 : 本文件存放所有的中断服务函数。为了便于他人了解程序用到的中断，我们不建议将中断函数移到其他
*			的文件。
*			
*			我们只需要添加需要的中断函数即可。一般中断函数名是固定的，除非您修改了启动文件: 
*				Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\startup\arm\startup_stm32f10x_hd.s
*			
*			启动文件是汇编语言文件，定了每个中断的服务函数，这些函数使用了WEAK 关键字，表示弱定义，因此如
*			果我们在c文件中重定义了该服务函数（必须和它同名），那么启动文件的中断函数将自动无效。这也就
*			函数重定义的概念，这和C++中的函数重载的意义类似。
*
*				
*	修改记录 :
*		版本号  日期       	作者    	说明
*		V1.1	2014-1-22	JOY			接收数据格式，针对RTC设置日期时间
*										0xAA, 0x99,			// 前导符号
*										0xF1, +3bytes+补齐10字节		// 功能，长度，后面3字节为小时，分钟，秒(24小时制)
*
*********************************************************************************************************
*/

#include "stm32f10x_it.h"
#include "stm32arduino.h"
#include "usb_hcd_int.h"
#include "usbh_core.h"



__IO uint32_t TimeDisplayFlag = 0;	// 用于RTC秒中断标志，置位时更新显示
__IO uint8_t AlarmFlag = 0;			// 用于RTC闹钟中断标志
__IO uint8_t RXFlag = 0;			// 用于串口接收数据完成标志。
__IO uint8_t RxBuffer[RXBUFFERSIZE];
__IO uint16_t ReadRxBufferPointer = 0;
__IO uint16_t WriteRxBufferPointer = 0;
__IO uint16_t RxBufferDataLength = 0;	// 串口接收数据长度，包括命令+数据+校验

/* 全局运行时间，单位1ms，用于计算系统运行时间 */
__IO int32_t g_iRunTime = 0;

extern USB_OTG_CORE_HANDLE          USB_OTG_Core;
extern USBH_HOST                    USB_Host;

extern void USB_OTG_BSP_TimerIRQ (void);

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/*******************************************************************************
* Function Name  : NMI_Handler
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NMI_Handler(void)
{
}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void HardFault_Handler(void)
{
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1)
	{
	}
}

/*******************************************************************************
* Function Name  : MemManage_Handler
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
	}
}

/*******************************************************************************
* Function Name  : BusFault_Handler
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
	}
}

/*******************************************************************************
* Function Name  : UsageFault_Handler
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
	}
}

/*******************************************************************************
* Function Name  : SVC_Handler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVC_Handler(void)
{
}

/*******************************************************************************
* Function Name  : DebugMon_Handler
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DebugMon_Handler(void)
{
}

/*******************************************************************************
* Function Name  : PendSV_Handler
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PendSV_Handler(void)
{
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SysTick_Handler(void)
{
	SysTick_ISR();
}

/******************************************************************************/
/*            STM32F10x Peripherals Interrupt Handlers                        */
/******************************************************************************/

/*******************************************************************************
* Function Name  : USB_LP_CAN1_RX0_IRQHandler
* Description    : This function handles USB Low Priority or CAN RX0 interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	//USB_Istr();
}

/*******************************************************************************
* Function Name  : EXTI9_5_IRQHandler
* Description    : This function handles External lines 9 to 5 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI9_5_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USBWakeUp_IRQHandler
* Description    : This function handles USB WakeUp interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBWakeUp_IRQHandler(void)
{
	//EXTI_ClearITPendingBit(EXTI_Line18);
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/*******************************************************************************
* Function Name  : PPP_IRQHandler
* Description    : This function handles PPP interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @brief  EXTI1_IRQHandler
  *         This function handles External line 1 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI1_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line1) != RESET)
	{
		USB_Host.usr_cb->OverCurrentDetected();
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}

void TIM2_IRQHandler(void)
{
	USB_OTG_BSP_TimerIRQ();
}

/**
  * @brief  OTG_FS_IRQHandler
  *          This function handles USB-On-The-Go FS global interrupt request.
  *          requests.
  * @param  None
  * @retval None
  */
#ifdef USE_USB_OTG_FS
void OTG_FS_IRQHandler(void)
#else
void OTG_HS_IRQHandler(void)
#endif
{
	USBH_OTG_ISR_Handler(&USB_OTG_Core);
}

/*
*********************************************************************************************************
*	函 数 名: EXTI0_IRQHandler
*	功能说明: 按键PA0中断服务函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void EXTI0_IRQHandler(void)
{
	Button_IRQ();
}


/*
*********************************************************************************************************
*	函 数 名: USART2_IRQHandler
*	功能说明: 串口2接收中断
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void USART2_IRQHandler(void)
{
	static uint8_t sum = 0;
	uint16_t lastindex;
	static uint8_t headflag = 0, endflag = 0;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{		
		RxBuffer[WriteRxBufferPointer] = USART_ReceiveData(USART2);		// 从接收寄存器读取1字节数据
		lastindex = (RXBUFFERSIZE + WriteRxBufferPointer - 1) % RXBUFFERSIZE;
		
		
		if((RxBuffer[lastindex] == 0xAA) && (RxBuffer[WriteRxBufferPointer]==0x55))	// 帧头
		{
			headflag = 1;
			sum = 0;
			RxBufferDataLength = 0;
			ReadRxBufferPointer = WriteRxBufferPointer + 1;
		}
		else if(RxBuffer[WriteRxBufferPointer] == 0xEF)
		{
			endflag = 1;
			if(sum == 0)	// 校验正确
			{
				RXFlag = 1;
			}
			else
			{
				sum = 0;
				RxBufferDataLength = 0;
			}
				
		}			
		else if((headflag == 1) && (endflag == 0))
		{
			sum += RxBuffer[WriteRxBufferPointer];
			RxBufferDataLength++;	// 长度包括命令+数据+校验
		}
				
		
		WriteRxBufferPointer++;
		if(WriteRxBufferPointer >= RXBUFFERSIZE)
		{
			WriteRxBufferPointer = 0;
		}
		
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);			// 清除中断标志位
	}
	
}

/*
*********************************************************************************************************
*	函 数 名: RTC_IRQHandler
*	功能说明: RTC时钟秒中断
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void RTC_IRQHandler(void)
{
	if (RTC_GetITStatus(RTC_IT_ALR) != RESET)	// 闹钟中断
	{
		RTC_ClearITPendingBit(RTC_IT_ALR);
		AlarmFlag=1;
	}
	
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET)	// 1秒中断
	{
		/* Clear the RTC Second interrupt */
		RTC_ClearITPendingBit(RTC_IT_SEC);
		
		TimeDisplayFlag = 1;
	}
}

/*
*********************************************************************************************************
*	函 数 名: TIM3_IRQHandler
*	功能说明: 1ms定时中断
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_FLAG_Update);
		g_iRunTime++;
	}
}

/**********************************END OF FILE**********************************/
