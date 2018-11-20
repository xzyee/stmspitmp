/*
*********************************************************************************************************
*
*	模块名称 : RTC模块
*	文件名称 : calendar_rtc.h
*	版    本 : V1.0
*	说    明 : 
*				使用RTC，在7'屏上显示时间
*				
*	修改记录 :
*		版本号  日期       	作者    说明
*		v1.0    2013-10-23	JOY		
*
*
*********************************************************************************************************
*/
#ifndef _CALENDER_RTC_H_
#define _CALENDER_RTC_H_

#include "stm32f10x.h"		/* 如果要用ST的固件库，必须包含这个文件 */
#include <time.h>
#include <stdio.h>


//---- 时间参数定义------------------------------------------------------------
typedef enum { 
	Msecond = 1			,	//--毫秒时间到(为了方便的实现功能，从1开始赋值!)
	Second					,	//--秒时间到
	Minute 	  			,	//--分时间到
	Hour 	  				,	//--小时时间到
	AllTime					,	//--代表全部时间
#define	 Show_HM_Buf	11	//--显示时分模式的缓存大小
#define	 Show_HMS_Buf	12	//--显示时分秒模式的缓存大小
#define	 Show_YMD_Buf	15	//--显示年月日模式的缓存大小
#define	 Show_WEEK_Buf	7	//--显示年月日模式的缓存大小
	Show_HM				,	//--只显示时和分 12小时制
	Show_HMS			,	//--显示时分秒 12小时制
	Show_YMD      , //--显示年月日
	Show_HMS24,      //--显示时分秒 24小时制
	Show_WEEK		//显示星期
	
}TIME_STATE;

typedef enum {
	Sunday = 0	,//--星期日
	Monday			,//--星期一
	Tuesday			,//--星期二
	Wednesday		,//--星期三
	Thursday		,//--星期四
	Friday			,//--星期五
	Saturday		 //--星期六
}WEEK;


// /** 描述    : 转换UNIX时间戳为日历时间*/
// struct tm Time_ConvUnixToCalendar(time_t t);

// /** 描述    : 写入RTC时钟当前时间,屏蔽了库函数从1900年开始的特性!!*/
// time_t Time_ConvCalendarToUnix(struct tm t);

// /** 描述    : 从RTC取当前时间的Unix时间戳值*/
// time_t Time_GetUnixTime(void);

/** 描述    : 从RTC取当前时间的日历时间（struct tm）*/
struct tm Time_GetCalendarTime(void);

// /** 描述    : 将给定的Unix时间戳写入RTC*/
// void Time_SetUnixTime(time_t);

/** 描述    : 将给定的Calendar格式时间转换成UNIX时间戳写入RTC*/
void Time_SetCalendarTime(struct tm t);

/** 描述    :上电时只需要调用本函数，自动检查是否需要RTC初始化，
*			若需要重新初始化RTC，则调用RTC_Configuration()完成相应操作*/
void RTC_Initialize(void);

// /** 描述    : 将Calendar2中的时间数据拷贝到Calendar1中*/
// void Time_CalendarCopy(struct tm *Calendar1,struct tm *Calendar2);

/** 描述    : 将当前时间(小时:分钟)转换为字符串,共8个字符,例如"12:08 PM"*/
void Time_GetTimeString(struct tm *timedata, char *Timestr, uint8_t Show_Mode);

void RTC_NVICConfiguration(void);
void Time_SetCalendarAlarm(struct tm t);
struct tm Time_GetCalendarAlarm(void);

#endif
