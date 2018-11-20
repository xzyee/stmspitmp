/*
*********************************************************************************************************
*
*	ģ������ : ��������
*	�ļ����� : usart.c
*	��    �� : V1.0
*	˵    �� : ���ڿ���
*				ʵ��printf��scanf�����ض��򵽴���2����֧��printf��Ϣ��USART2
*				ʵ���ض���ֻ��Ҫ���2������:
*				int fputc(int ch, FILE *f);		int fgetc(FILE *f);
*				����KEIL MDK������������ѡ������Ҫ�� Options->Target�е�Code Generationѡ���е�use MicorLIBǰ��򹳣����򲻻������ݴ�ӡ�����ڡ�
*	�޸ļ�¼ :
*		�汾��  ����       	����    ˵��
*		V1.0	2013-12-12	JOY
*		V1.1	2014-03-21	JOY		�޸����ж����ȼ�
*		V1.2	2014-10-28	JOY		�޸�RX��GPIO��ΪGPIO_Mode_IPUģʽ����ֹ�ϵ�ʱRX���ֵ͵�ƽ�źš�
*
*********************************************************************************************************
*/
#include "usart.h"

/*
*********************************************************************************************************
*	�� �� ��: fputc
*	����˵��: �ض���putc��������������ʹ��printf�����Ӵ���2��ӡ���
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
int fputc(int ch, FILE *f)
{
	/* дһ���ֽڵ�USART2 */
	USART_SendData(USART2, (unsigned char) ch);

	/* �ȴ����ͽ��� */
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
	{}

	return ch;
}

/*
*********************************************************************************************************
*	�� �� ��: fgetc
*	����˵��: �ض���getc��������������ʹ��scanff�����Ӵ���2��������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
int fgetc(FILE *f)
{
	/* �ȴ�����1�������� */
	while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET);

	return (int)USART_ReceiveData(USART2);
}

/*
*********************************************************************************************************
*	�� �� ��: USART2_NVIC
*	����˵��: USART2�����ж�����
*	��    ��: ��
*	�� �� ֵ: ��
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
*	�� �� ��: USART2_Init
*	����˵��: ���ڳ�ʼ��
*	��    ��: baud-������
*	�� �� ֵ: ��
*********************************************************************************************************
*/					    
void USART2_Init(uint32_t baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* ��1��: ��GPIO��USART������ʱ�� */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* ��2��: ��USART Tx��GPIO����Ϊ���츴��ģʽ */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* ��3��: ��USART Rx��GPIO����ΪGPIO_Mode_IPUģʽ����ֹ�ϵ�ʱRX���ֵ͵�ƽ�źš�*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* ��4��: ����USART����
	    - ������   = 9600 baud
	    - ���ݳ��� = 8 Bits
	    - 1��ֹͣλ
	    - ��У��
	    - ��ֹӲ������(����ֹRTS��CTS)
	    - ʹ�ܽ��պͷ���
	*/
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	/* ��5��: ʹ�� USART�� ������� */
	USART_Cmd(USART2, ENABLE);

	//���ô����ж�
	USART2_NVIC();
	
	/* 
		CPU��Сȱ��: �������úã����ֱ��Send�����1���ֽڷ��Ͳ���ȥ
		�����������1���ֽ��޷���ȷ���ͳ�ȥ������: 
	 	�巢����ɱ�־��Transmission Complete flag 
	*/
	USART_ClearFlag(USART2, USART_FLAG_TC);  
	USART_ClearITPendingBit(USART2, USART_IT_RXNE);			// ����жϱ�־λ
	
	//�������ж�
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

void USART2_SendByte(unsigned char temp)
{
    USART_SendData(USART2, temp);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/**********************************END OF FILE**********************************/
