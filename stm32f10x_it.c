/*
*********************************************************************************************************
*	                                  
*	ģ������ : �ж�ģ��
*	�ļ����� : stm32f10x_it.c
*	�̼���汾: V3.5.0
*	˵    �� : ���ļ�������е��жϷ�������Ϊ�˱��������˽�����õ����жϣ����ǲ����齫�жϺ����Ƶ�����
*			���ļ���
*			
*			����ֻ��Ҫ�����Ҫ���жϺ������ɡ�һ���жϺ������ǹ̶��ģ��������޸��������ļ�: 
*				Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\startup\arm\startup_stm32f10x_hd.s
*			
*			�����ļ��ǻ�������ļ�������ÿ���жϵķ���������Щ����ʹ����WEAK �ؼ��֣���ʾ�����壬�����
*			��������c�ļ����ض����˸÷��������������ͬ��������ô�����ļ����жϺ������Զ���Ч����Ҳ��
*			�����ض���ĸ�����C++�еĺ������ص��������ơ�
*
*				
*	�޸ļ�¼ :
*		�汾��  ����       	����    	˵��
*		V1.1	2014-1-22	JOY			�������ݸ�ʽ�����RTC��������ʱ��
*										0xAA, 0x99,			// ǰ������
*										0xF1, +3bytes+����10�ֽ�		// ���ܣ����ȣ�����3�ֽ�ΪСʱ�����ӣ���(24Сʱ��)
*
*********************************************************************************************************
*/

#include "stm32f10x_it.h"
#include "stm32arduino.h"
#include "usb_hcd_int.h"
#include "usbh_core.h"



__IO uint32_t TimeDisplayFlag = 0;	// ����RTC���жϱ�־����λʱ������ʾ
__IO uint8_t AlarmFlag = 0;			// ����RTC�����жϱ�־
__IO uint8_t RXFlag = 0;			// ���ڴ��ڽ���������ɱ�־��
__IO uint8_t RxBuffer[RXBUFFERSIZE];
__IO uint16_t ReadRxBufferPointer = 0;
__IO uint16_t WriteRxBufferPointer = 0;
__IO uint16_t RxBufferDataLength = 0;	// ���ڽ������ݳ��ȣ���������+����+У��

/* ȫ������ʱ�䣬��λ1ms�����ڼ���ϵͳ����ʱ�� */
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
*	�� �� ��: EXTI0_IRQHandler
*	����˵��: ����PA0�жϷ�����
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void EXTI0_IRQHandler(void)
{
	Button_IRQ();
}


/*
*********************************************************************************************************
*	�� �� ��: USART2_IRQHandler
*	����˵��: ����2�����ж�
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void USART2_IRQHandler(void)
{
	static uint8_t sum = 0;
	uint16_t lastindex;
	static uint8_t headflag = 0, endflag = 0;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{		
		RxBuffer[WriteRxBufferPointer] = USART_ReceiveData(USART2);		// �ӽ��ռĴ�����ȡ1�ֽ�����
		lastindex = (RXBUFFERSIZE + WriteRxBufferPointer - 1) % RXBUFFERSIZE;
		
		
		if((RxBuffer[lastindex] == 0xAA) && (RxBuffer[WriteRxBufferPointer]==0x55))	// ֡ͷ
		{
			headflag = 1;
			sum = 0;
			RxBufferDataLength = 0;
			ReadRxBufferPointer = WriteRxBufferPointer + 1;
		}
		else if(RxBuffer[WriteRxBufferPointer] == 0xEF)
		{
			endflag = 1;
			if(sum == 0)	// У����ȷ
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
			RxBufferDataLength++;	// ���Ȱ�������+����+У��
		}
				
		
		WriteRxBufferPointer++;
		if(WriteRxBufferPointer >= RXBUFFERSIZE)
		{
			WriteRxBufferPointer = 0;
		}
		
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);			// ����жϱ�־λ
	}
	
}

/*
*********************************************************************************************************
*	�� �� ��: RTC_IRQHandler
*	����˵��: RTCʱ�����ж�
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void RTC_IRQHandler(void)
{
	if (RTC_GetITStatus(RTC_IT_ALR) != RESET)	// �����ж�
	{
		RTC_ClearITPendingBit(RTC_IT_ALR);
		AlarmFlag=1;
	}
	
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET)	// 1���ж�
	{
		/* Clear the RTC Second interrupt */
		RTC_ClearITPendingBit(RTC_IT_SEC);
		
		TimeDisplayFlag = 1;
	}
}

/*
*********************************************************************************************************
*	�� �� ��: TIM3_IRQHandler
*	����˵��: 1ms��ʱ�ж�
*	��    �Σ���
*	�� �� ֵ: ��
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
