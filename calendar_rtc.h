/*
*********************************************************************************************************
*
*	ģ������ : RTCģ��
*	�ļ����� : calendar_rtc.h
*	��    �� : V1.0
*	˵    �� : 
*				ʹ��RTC����7'������ʾʱ��
*				
*	�޸ļ�¼ :
*		�汾��  ����       	����    ˵��
*		v1.0    2013-10-23	JOY		
*
*
*********************************************************************************************************
*/
#ifndef _CALENDER_RTC_H_
#define _CALENDER_RTC_H_

#include "stm32f10x.h"		/* ���Ҫ��ST�Ĺ̼��⣬�����������ļ� */
#include <time.h>
#include <stdio.h>


//---- ʱ���������------------------------------------------------------------
typedef enum { 
	Msecond = 1			,	//--����ʱ�䵽(Ϊ�˷����ʵ�ֹ��ܣ���1��ʼ��ֵ!)
	Second					,	//--��ʱ�䵽
	Minute 	  			,	//--��ʱ�䵽
	Hour 	  				,	//--Сʱʱ�䵽
	AllTime					,	//--����ȫ��ʱ��
#define	 Show_HM_Buf	11	//--��ʾʱ��ģʽ�Ļ����С
#define	 Show_HMS_Buf	12	//--��ʾʱ����ģʽ�Ļ����С
#define	 Show_YMD_Buf	15	//--��ʾ������ģʽ�Ļ����С
#define	 Show_WEEK_Buf	7	//--��ʾ������ģʽ�Ļ����С
	Show_HM				,	//--ֻ��ʾʱ�ͷ� 12Сʱ��
	Show_HMS			,	//--��ʾʱ���� 12Сʱ��
	Show_YMD      , //--��ʾ������
	Show_HMS24,      //--��ʾʱ���� 24Сʱ��
	Show_WEEK		//��ʾ����
	
}TIME_STATE;

typedef enum {
	Sunday = 0	,//--������
	Monday			,//--����һ
	Tuesday			,//--���ڶ�
	Wednesday		,//--������
	Thursday		,//--������
	Friday			,//--������
	Saturday		 //--������
}WEEK;


// /** ����    : ת��UNIXʱ���Ϊ����ʱ��*/
// struct tm Time_ConvUnixToCalendar(time_t t);

// /** ����    : д��RTCʱ�ӵ�ǰʱ��,�����˿⺯����1900�꿪ʼ������!!*/
// time_t Time_ConvCalendarToUnix(struct tm t);

// /** ����    : ��RTCȡ��ǰʱ���Unixʱ���ֵ*/
// time_t Time_GetUnixTime(void);

/** ����    : ��RTCȡ��ǰʱ�������ʱ�䣨struct tm��*/
struct tm Time_GetCalendarTime(void);

// /** ����    : ��������Unixʱ���д��RTC*/
// void Time_SetUnixTime(time_t);

/** ����    : ��������Calendar��ʽʱ��ת����UNIXʱ���д��RTC*/
void Time_SetCalendarTime(struct tm t);

/** ����    :�ϵ�ʱֻ��Ҫ���ñ��������Զ�����Ƿ���ҪRTC��ʼ����
*			����Ҫ���³�ʼ��RTC�������RTC_Configuration()�����Ӧ����*/
void RTC_Initialize(void);

// /** ����    : ��Calendar2�е�ʱ�����ݿ�����Calendar1��*/
// void Time_CalendarCopy(struct tm *Calendar1,struct tm *Calendar2);

/** ����    : ����ǰʱ��(Сʱ:����)ת��Ϊ�ַ���,��8���ַ�,����"12:08 PM"*/
void Time_GetTimeString(struct tm *timedata, char *Timestr, uint8_t Show_Mode);

void RTC_NVICConfiguration(void);
void Time_SetCalendarAlarm(struct tm t);
struct tm Time_GetCalendarAlarm(void);

#endif
