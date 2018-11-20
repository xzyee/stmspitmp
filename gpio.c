/*
*********************************************************************************************************
*
*	ģ������ : ������LED����
*	�ļ����� : gpio.c
*	��    �� : V1.0
*	˵    �� : ������LED��������
*	�޸ļ�¼ :
*		�汾��  ����       	����    ˵��
*		V1.0	2013-12-12	JOY
*
*********************************************************************************************************
*/

#include "gpio.h"

const uint16_t GPIO_PIN[LEDn] = {LED1_PIN, LED2_PIN, LED3_PIN};
uint8_t Key_Pressed = 0;	// ����PA0״̬��־��1-����

/*
*********************************************************************************************************
*	�� �� ��: LED_Init
*	����˵��: LED���ų�ʼ��
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(LED_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = LED1_PIN | LED2_PIN | LED3_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);
}

/*
*********************************************************************************************************
*	�� �� ��: LEDOn
*	����˵��: LED���ơ�����
*	��    ��: Led_TypeDef Led: LED1 or LED2 or LED3
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void LEDOn(Led_TypeDef Led)
{
	GPIO_SetBits(LED_GPIO_PORT, GPIO_PIN[Led]);
}

/*
*********************************************************************************************************
*	�� �� ��: LEDOff
*	����˵��: LED���ơ�����
*	��    ��: Led_TypeDef Led: LED1 or LED2 or LED3
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void LEDOff(Led_TypeDef Led)
{
	GPIO_ResetBits(LED_GPIO_PORT, GPIO_PIN[Led]);
}

/*
*********************************************************************************************************
*	�� �� ��: LEDToggle
*	����˵��: LED���ơ�����˸
*	��    ��: Led_TypeDef Led: LED1 or LED2 or LED3
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void LEDToggle(Led_TypeDef Led)
{
	LED_GPIO_PORT->ODR ^= GPIO_PIN[Led];
}


/*
*********************************************************************************************************
*	�� �� ��: Button_Init
*	����˵��: �������ų�ʼ����PA0�����ڻ��ѿ��ƣ�����Ϊ�ߵ�ƽ
*	��    ��: ButtonMode_TypeDef Button_Mode: BUTTON_MODE_GPIO or BUTTON_MODE_EXTI
*				GPIO����ģʽ���ж�ģʽ
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void Button_Init(ButtonMode_TypeDef Button_Mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the BUTTON Clock */
  RCC_APB2PeriphClockCmd(BUTTON_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

  /* Configure Button pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Pin = BUTTON_PIN;
  GPIO_Init(BUTTON_GPIO_PORT, &GPIO_InitStructure);


  if (Button_Mode == BUTTON_MODE_EXTI)
  {
    /* Connect Button EXTI Line to Button GPIO Pin */
    GPIO_EXTILineConfig(BUTTON_EXTI_PORT_SOURCE, BUTTON_EXTI_PIN_SOURCE);

    /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = BUTTON_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;	// PA0 ����Ϊ��
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = BUTTON_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure); 
  }
}

/*
*********************************************************************************************************
*	�� �� ��: Button_GetState
*	����˵��: ����PA0������״̬������Ϊ�ߵ�ƽ
*	��    ��: ��
*	�� �� ֵ: PA0����ֵ
*********************************************************************************************************
*/
uint8_t Button_GetState(void)
{
  return GPIO_ReadInputDataBit(BUTTON_GPIO_PORT, BUTTON_PIN);
}

/*
*********************************************************************************************************
*	�� �� ��: Button_IRQ
*	����˵��: BUTTON_MODE_EXTI ģʽ�£��жϷ�����
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void Button_IRQ(void)
{
	if(EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		EXTI_ClearITPendingBit(EXTI_Line0);
		Key_Pressed = 1;
	}
}

