/*
*********************************************************************************************************
*
*	模块名称 : 串口驱动
*	文件名称 : usart.h
*	版    本 : V1.0
*	说    明 : 串口控制
*				实现printf和scanf函数重定向到串口2，即支持printf信息到USART1
*				实现重定向，只需要添加2个函数:
*				int fputc(int ch, FILE *f);		int fgetc(FILE *f);
*				对于KEIL MDK编译器，编译选项中需要在 Options->Target中的Code Generation选项中的use MicorLIB前面打钩，否则不会有数据打印到串口。
*	修改记录 :
*		版本号  日期       	作者    说明
*		V1.0	2013-12-12	JOY
*
*********************************************************************************************************
*/

#ifndef __USART_H
#define __USART_H
	
#include "stm32f10x.h"
#include <stdio.h>
	
void USART2_Init(uint32_t baud);
void USART2_SendByte(unsigned char temp);

#endif

/**********************************END OF FILE**********************************/
