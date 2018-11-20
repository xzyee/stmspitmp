/*
*********************************************************************************************************
*
*	模块名称 : 串口驱动
*	文件名称 : usart.c
*	版    本 : V1.0
*	说    明 : 串口控制
*				实现printf和scanf函数重定向到串口2，即支持printf信息到USART2
*				实现重定向，只需要添加2个函数:
*				int fputc(int ch, FILE *f);		int fgetc(FILE *f);
*				对于KEIL MDK编译器，编译选项中需要在 Options->Target中的Code Generation选项中的use MicorLIB前面打钩，否则不会有数据打印到串口。
*	修改记录 :
*		版本号  日期       	作者    说明
*		V1.0	2013-12-12	JOY
*		V1.1	2014-03-21	JOY		修改了中断优先级
*		V1.2	2014-10-28	JOY		修改RX的GPIO口为GPIO_Mode_IPU模式，防止上电时RX出现低电平信号。
*
*********************************************************************************************************
*/
#include "usart.h"

/*
*********************************************************************************************************
*	函 数 名: fputc
*	功能说明: 重定义putc函数，这样可以使用printf函数从串口2打印输出
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int fputc(int ch, FILE *f)
{
	/* 写一个字节到USART2 */
	USART_SendData(USART2, (unsigned char) ch);

	/* 等待发送结束 */
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
	{}

	return ch;
}

/*
*********************************************************************************************************
*	函 数 名: fgetc
*	功能说明: 重定义getc函数，这样可以使用scanff函数从串口2输入数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int fgetc(FILE *f)
{
	/* 等待串口1输入数据 */
	while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET);

	return (int)USART_ReceiveData(USART2);
}

/*
*********************************************************************************************************
*	函 数 名: USART2_NVIC
*	功能说明: USART2串口中断设置
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void USART2_NVIC(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* Configure the NVIC Preemption Priority Bits */  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); 
	
	/* Enable the USART2 Interrupt */ 
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 
}

/*
*********************************************************************************************************
*	函 数 名: USART2_Init
*	功能说明: 串口初始化
*	形    参: baud-波特率
*	返 回 值: 无
*********************************************************************************************************
*/					    
void USART2_Init(uint32_t baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* 第1步: 打开GPIO和USART部件的时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* 第2步: 将USART Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第3步: 将USART Rx的GPIO配置为GPIO_Mode_IPU模式，防止上电时RX出现低电平信号。*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第4步: 配置USART参数
	    - 波特率   = 9600 baud
	    - 数据长度 = 8 Bits
	    - 1个停止位
	    - 无校验
	    - 禁止硬件流控(即禁止RTS和CTS)
	    - 使能接收和发送
	*/
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	/* 第5步: 使能 USART， 配置完毕 */
	USART_Cmd(USART2, ENABLE);

	//配置串口中断
	USART2_NVIC();
	
	/* 
		CPU的小缺陷: 串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题: 
	 	清发送完成标志，Transmission Complete flag 
	*/
	USART_ClearFlag(USART2, USART_FLAG_TC);  
	USART_ClearITPendingBit(USART2, USART_IT_RXNE);			// 清除中断标志位
	
	//开串口中断
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

void USART2_SendByte(unsigned char temp)
{
    USART_SendData(USART2, temp);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/**********************************END OF FILE**********************************/
