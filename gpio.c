/*
*********************************************************************************************************
*
*	模块名称 : 按键及LED驱动
*	文件名称 : gpio.c
*	版    本 : V1.0
*	说    明 : 按键及LED引脚配置
*	修改记录 :
*		版本号  日期       	作者    说明
*		V1.0	2013-12-12	JOY
*
*********************************************************************************************************
*/

#include "gpio.h"

const uint16_t GPIO_PIN[LEDn] = {LED1_PIN, LED2_PIN, LED3_PIN};
uint8_t Key_Pressed = 0;	// 按键PA0状态标志，1-按下

/*
*********************************************************************************************************
*	函 数 名: LED_Init
*	功能说明: LED引脚初始化
*	形    参: 无
*	返 回 值: 无
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
*	函 数 名: LEDOn
*	功能说明: LED控制——亮
*	形    参: Led_TypeDef Led: LED1 or LED2 or LED3
*	返 回 值: 无
*********************************************************************************************************
*/
void LEDOn(Led_TypeDef Led)
{
	GPIO_SetBits(LED_GPIO_PORT, GPIO_PIN[Led]);
}

/*
*********************************************************************************************************
*	函 数 名: LEDOff
*	功能说明: LED控制——灭
*	形    参: Led_TypeDef Led: LED1 or LED2 or LED3
*	返 回 值: 无
*********************************************************************************************************
*/
void LEDOff(Led_TypeDef Led)
{
	GPIO_ResetBits(LED_GPIO_PORT, GPIO_PIN[Led]);
}

/*
*********************************************************************************************************
*	函 数 名: LEDToggle
*	功能说明: LED控制——闪烁
*	形    参: Led_TypeDef Led: LED1 or LED2 or LED3
*	返 回 值: 无
*********************************************************************************************************
*/
void LEDToggle(Led_TypeDef Led)
{
	LED_GPIO_PORT->ODR ^= GPIO_PIN[Led];
}


/*
*********************************************************************************************************
*	函 数 名: Button_Init
*	功能说明: 按键引脚初始化，PA0不用于唤醒控制，按下为高电平
*	形    参: ButtonMode_TypeDef Button_Mode: BUTTON_MODE_GPIO or BUTTON_MODE_EXTI
*				GPIO引脚模式或中断模式
*	返 回 值: 无
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
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;	// PA0 按下为高
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
*	函 数 名: Button_GetState
*	功能说明: 返回PA0按键的状态，按下为高电平
*	形    参: 无
*	返 回 值: PA0引脚值
*********************************************************************************************************
*/
uint8_t Button_GetState(void)
{
  return GPIO_ReadInputDataBit(BUTTON_GPIO_PORT, BUTTON_PIN);
}

/*
*********************************************************************************************************
*	函 数 名: Button_IRQ
*	功能说明: BUTTON_MODE_EXTI 模式下，中断服务函数
*	形    参: 无
*	返 回 值: 无
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

