/*
*********************************************************************************************************
*
*	模块名称 : RTC模块
*	文件名称 : calendar_rtc.c
*	版    本 : V1.2
*	说    明 : 
*				增加RTC闹钟
*				
*	修改记录 :
*		版本号  日期       	作者    说明
*		v1.1    2013-10-23	JOY		
*		v1.2    2014-9-24	JOY		增加RTC闹钟
*		v1.3	2015-06-29			RTC_Initialize初始化中增加Time_SetCalendarAlarm(now);	// 设置初始默认闹钟时间
*
*********************************************************************************************************
*/
#include "calendar_rtc.H"

uint16_t RTC_year, RTC_month, RTC_date, RTC_week, RTC_hour, RTC_minute, RTC_second; // 年，月/日/週/時/分/秒

//--首先创建一个对象存放时间
//--这里存放着时间,年月日时分秒星期等等,具体请查看time.h
//注意：月份范围为1~12，Time_SetCalendarTime函数中已将tm_mon--, 符合time.h中tm_mon的范围0-11
//		星期范围为0~6，从星期日开始
struct tm now={59, 58, 15, 29, 6, 2015, 1};	// 2015-06-29 15:58:59 星期一



/**
  * @brief  Configures the nested vectored interrupt controller.
  * @param  None
  * @retval None
  */
void RTC_NVICConfiguration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure 1 bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	/* Enable the RTC Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  Configures the RTC.
  * @param  None
  * @retval None
  */
void RTC_Configuration(void)
{
	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	/* Reset Backup Domain */
	BKP_DeInit();

	/* Enable LSE */
	RCC_LSEConfig(RCC_LSE_ON);
	/* Wait till LSE is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
	{}

	/* Select LSE as RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

	/* Enable RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC registers synchronization */
	RTC_WaitForSynchro();

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Enable the RTC Second */
	RTC_ITConfig(RTC_IT_SEC, ENABLE);
//	// 使能ALARM中断
//	RTC_ITConfig(RTC_IT_ALR, ENABLE);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Set RTC prescaler: set RTC period to 1sec */
	RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
}
/*
*********************************************************************************************************
*	函 数 名: RTC_Initialize
*	功能说明: 上电时只需要调用本函数，自动检查是否需要RTC初始化，
*			  若需要重新初始化RTC，则调用RTC_Configuration()完成相应操作
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void RTC_Initialize(void)
{
	/* 在BKP的后备寄存器1中，存了一个特殊字符0xA5A5 */
    /* 第一次上电或后备电源掉电后，该寄存器数据丢失 */
    /* 表明RTC数据丢失，需要重新配置 */
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
//		printf("\r\n\n RTC not yet configured....");

		/* RTC Configuration */
		RTC_Configuration();
		RTC_NVICConfiguration();
//		printf("\r\n RTC configured....");

// 		/* Adjust time by values entred by the user on the hyperterminal */
		Time_SetCalendarTime(now);	// 设置初始默认时间
		Time_SetCalendarAlarm(now);	// 设置初始默认闹钟时间

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
		
	}
	else
	{
		RTC_NVICConfiguration();
		
		/* 若后备寄存器没有掉电，则无需重新配置RTC */
		/* 这里可以利用RCC_GetFlagStatus()函数查看本次复位类型 */
		  /* Check if the Power On Reset flag is set */
		if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
		{
//		  printf("\r\n\n Power On Reset occurred....");	// 上电复位
		}
		/* Check if the Pin Reset flag is set */
		else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
		{
//		  printf("\r\n\n External Reset occurred....");	// 外部管脚复位
		}

//		printf("\r\n No need to configure RTC....");
		/* Wait for RTC registers synchronization */
		RTC_WaitForSynchro();

		/* Enable the RTC Second */
		RTC_ITConfig(RTC_IT_SEC, ENABLE);
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
/*
	printf("\r\n 串口设置格式(10字节)：");
	printf("\r\n 1. 设置日期星期时间(年月日星期时分秒)：0xAA, 0x99, 0xF0, 0xyy, 0xmm, 0xdd, 0xww, 0xmm, 0xss, 0xss");
	printf("\r\n 2. 设置时间(时分秒 24小时制)： 0xAA, 0x99, 0xF1, 0xhh, 0xmm, 0xss, 0x00, 0x00, 0x00, 0x00");	// 设置日期时间格式
	printf("\r\n 3. 设置日期(年月日)：0xAA, 0x99, 0xF2, 0xyy, 0xmm, 0xdd, 0x00, 0x00, 0x00, 0x00");
	printf("\r\n 4. 设置闹钟时间(时分秒 24小时制)： 0xAA, 0x99, 0xF3, 0xhh, 0xmm, 0xss, 0x00, 0x00, 0x00, 0x00\r\n");	// 设置日期时间格式
	printf("------------------------------------\r\n");
*/	
    /* Clear reset flags */
	RCC_ClearFlag();	// 清除RCC中复位标志
	
	/*启用PWR和BKP的时钟（from APB1）*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP,ENABLE);
    PWR_BackupAccessCmd(ENABLE);/*后备域解锁*/
}


/******************************************************************************
* 函数名  : Time_ConvUnixToCalendar()
* 描述    : 转换UNIX时间戳为日历时间
* 输入    : u32 t  当前时间的UNIX时间戳
* 输出    : None
* 返回    : struct tm
* 创建日期: 2012.1.1
* 版本    : V1.00
*******************************************************************************/
struct tm Time_ConvUnixToCalendar(time_t t)
{
    struct tm *t_tm ;
    t_tm=localtime(&t);
    t_tm->tm_year+=1900 ;
    /*localtime转换结果的tm_year是相对值，需要转成绝对值*/
    return *t_tm ;
}


/******************************************************************************
* 函数名  : Time_ConvCalendarToUnix()
* 描述    : 写入RTC时钟当前时间,屏蔽了库函数从1900年开始的特性!!
* 输入    : struct tm t
* 输出    : None
* 返回    : time_t
* 创建日期: 2012.1.1
* 版本    : V1.00
*******************************************************************************/
time_t Time_ConvCalendarToUnix(struct tm t)
{
    t.tm_year-=1900 ;
    /*外部tm结构体存储的年份为2008格式*/
    /*而time.h中定义的年份格式为1900年开始的年份*/
    /*所以，在日期转换时要考虑到这个因素。*/
    return mktime(&t);
}


/******************************************************************************
* 函数名  : Time_GetUnixTime()
* 描述    : 从RTC取当前时间的Unix时间戳值
* 输入    : None
* 输出    : None
* 返回    : time_t t
* 创建日期: 2012.1.1
* 版本    : V1.00
*******************************************************************************/
time_t Time_GetUnixTime(void)
{
    return (time_t)RTC_GetCounter();
}


/******************************************************************************
* 函数名  : Time_GetCalendarTime()
* 描述    : 从RTC取当前时间的日历时间（struct tm）
* 输入    : None
* 输出    : None
* 返回    : time_t t_t
* 创建日期: 2012.1.1 
* 版本    : V1.00
*******************************************************************************/
struct tm Time_GetCalendarTime(void)
{
    time_t t_t ;
    struct tm t_tm ;
    
    t_t=(time_t)RTC_GetCounter();
    t_tm=Time_ConvUnixToCalendar(t_t);
		t_tm.tm_mon++;
    return t_tm ;
}


/******************************************************************************
* 函数名  : Time_SetUnixTime()
* 描述    : 将给定的Unix时间戳写入RTC
* 输入    : None
* 输出    : None
* 返回    : None
* 创建日期: 2012.1.1
* 版本    : V1.00
*******************************************************************************/
void Time_SetUnixTime(time_t t)
{
    RTC_WaitForLastTask();
    RTC_SetCounter((u32)t);
    RTC_WaitForLastTask();
    return ;
}

/******************************************************************************
* 函数名  : Time_SetCalendarTime()
* 描述    : 将给定的Calendar格式时间转换成UNIX时间戳写入RTC
* 输入    : None
* 输出    : None
* 返回    : None
* 创建日期: 2012.1.1
* 版本    : V1.00
*******************************************************************************/
void Time_SetCalendarTime(struct tm t)
{
    t.tm_mon--;
	Time_SetUnixTime(Time_ConvCalendarToUnix(t));
    return ;
}

///////***************增加闹钟功能*********************************************/////////
uint32_t RTC_GetAlarm(void)
{
  uint16_t tmp = 0;
  tmp = RTC->ALRL;
  return (((uint32_t)RTC->ALRH << 16 ) | tmp) ;
}

/******************************************************************************
* 函数名  : Time_GetCalendarAlarm()
* 描述    : 从RTC的闹钟寄存器取当前时间的日历时间（struct tm）
* 输入    : None
* 输出    : None
* 返回    : time_t t_t
* 创建日期: 2012.1.1 
* 版本    : V1.00
*******************************************************************************/
struct tm Time_GetCalendarAlarm(void)
{
    time_t t_t ;
    struct tm t_tm ;
    
    t_t=(time_t)RTC_GetAlarm();
    t_tm=Time_ConvUnixToCalendar(t_t);
		t_tm.tm_mon++;
    return t_tm ;
}

/******************************************************************************
* 函数名  : Time_SetUnixAlarm()
* 描述    : 将给定的Unix时间戳写入RTC的闹钟寄存器(RTC_ALR)
* 输入    : struct tm t：时间结构体
* 输出    : None
* 返回    : None
* 创建日期: 2014.9.24
* 版本    : V1.00
*******************************************************************************/
void Time_SetUnixAlarm(time_t t)
{
    RTC_WaitForLastTask();
    RTC_SetAlarm((u32)t);
    RTC_WaitForLastTask();
    return ;
}
/******************************************************************************
* 函数名  : Time_SetCalendarAlarm()
* 描述    : 将给定的Calendar格式时间转换成UNIX时间戳写入RTC的闹钟寄存器(RTC_ALR)
* 输入    : struct tm t：时间结构体
* 输出    : None
* 返回    : None
* 创建日期: 2014.9.24
* 版本    : V1.00
*******************************************************************************/
void Time_SetCalendarAlarm(struct tm t)
{
    t.tm_mon--;
	Time_SetUnixAlarm(Time_ConvCalendarToUnix(t));
    return ;
}

/******************************************************************************
* 函数名  : Time_CalendarCopy()
* 描述    : 将Calendar2中的时间数据拷贝到Calendar1中
* 输入    : Calendar1，Calendar2:时间数据
* 输出    : None
* 返回    : None
* 作者    : 红叶嵌入式添加
* 创建日期: 2012.1.1
* 版本    : V1.00
*******************************************************************************/
void Time_CalendarCopy(struct tm *Calendar1,struct tm *Calendar2)
{
	Calendar1->tm_sec   = Calendar2->tm_sec;   /* seconds after the minute, 0 to 60*/ 
    Calendar1->tm_min   = Calendar2->tm_min;   /* minutes after the hour, 0 to 59 */
    Calendar1->tm_hour  = Calendar2->tm_hour;  /* hours since midnight, 0 to 23 */
    Calendar1->tm_mday  = Calendar2->tm_mday;  /* day of the month, 1 to 31 */
    Calendar1->tm_mon   = Calendar2->tm_mon;   /* months since January, 0 to 11 */
    Calendar1->tm_year  = Calendar2->tm_year;  /* years since 1900 */
    Calendar1->tm_wday  = Calendar2->tm_wday;  /* days since Sunday, 0 to 6 */
    Calendar1->tm_yday  = Calendar2->tm_yday;  /* days since January 1, 0 to 365 */
    Calendar1->tm_isdst = Calendar2->tm_isdst; /* Daylight Savings Time flag */    
}

/******************************************************************************
* 函数名  : Time_GetTimeString()
* 描述    : 将当前时间(小时:分钟)转换为字符串
*		  		:	Show_HM模式(只显示时和分)共11个字符,例如返回:"12 : 08 PM"
*		  		:	Show_HMS模式(显示时分秒)共12个字符,例如返回:"12:08:50 PM"
*		  		:	Show_YMD模式(显示年月日)共15个字符,例如返回:"2012年01月01日 "
*				:	Show_HMS24(显示时分秒，24小时格式)共9个字符
*				:	Show_WEEK(显示星期)共7个字符，例如"星期日"
*		  		: 不论那种显示模式最后一个字符用来填充结束符'\0'
* 输入    : tm:unix格式的时间对象 *Timestr:字符串存储的位置 Show_Mode:要获取的时间类型
* 输出    : None
* 返回    : None
* 作者    : 红叶嵌入式添加+ JOY
* 创建日期: 2012.1.1
* 版本    : V1.1
* 备注	  : 不论那种模式,请先定义一个
* 备注	  : char bufname[Show_HM_Buf或Show_HMS_Buf或Show_YMD_Buf]的数组，
*		  : 然后调用Time_GetTimeString(tm,bufname,Show_HM);
*		  : 或者	Time_GetTimeString(tm,bufname,Show_HMS);
*		  : 或者	Time_GetTimeString(tm,bufname,Show_YMD);
*		  : 这样时间的字符串就会存储在bufname[]中,以供使用
*******************************************************************************/
void Time_GetTimeString(struct tm *timedata, char *Timestr, uint8_t Show_Mode)
{
    char *pTimestr = Timestr;
	char chinese[6] = "年月日";
	char *pto_chinese = chinese;
	uint8_t Is_AMorPM = 0;
	
	if(Show_Mode == Show_YMD)
	{
		//--存储年
		*pTimestr++ = timedata->tm_year/1000+'0';//--年千位
		*pTimestr++ = timedata->tm_year/100%10+'0'; //--年百位
		*pTimestr++ = timedata->tm_year/10%10+'0';  //--年十位	
		*pTimestr++ = timedata->tm_year%10+'0';  //--年十位
		*pTimestr++	= *pto_chinese++;//--把汉字"年"复制进去
		*pTimestr++ = *pto_chinese++;
		//--存储月	
		*pTimestr++ = timedata->tm_mon/10+'0';//--月十位
		*pTimestr++ = timedata->tm_mon%10+'0';//--月个位
		*pTimestr++	= *pto_chinese++;//--把汉字"月"复制进去
		*pTimestr++ = *pto_chinese++;
		//--存储日
		*pTimestr++ = timedata->tm_mday/10+'0';//--日十位
		*pTimestr++ = timedata->tm_mday%10+'0';//--日个位
		*pTimestr++	= *pto_chinese++;//--把汉字"日"复制进去
		*pTimestr++ = *pto_chinese++;
		*pTimestr = '\0';
		return;
	}
	
	if(Show_Mode == Show_WEEK)	//显示星期
	{
		switch(timedata->tm_wday)
		{
			case Sunday:
				sprintf(pTimestr, "星期日");
				break;
			case Monday:
				sprintf(pTimestr, "星期一");
				break;
			case Tuesday:
				sprintf(pTimestr, "星期二");
				break;
			case Wednesday:
				sprintf(pTimestr, "星期三");
				break;
			case Thursday:
				sprintf(pTimestr, "星期四");
				break;
			case Friday:
				sprintf(pTimestr, "星期五");
				break;
			case Saturday:
				sprintf(pTimestr, "星期六");
				break;
			default:
				break;
		}
		return;
	}
	
	if(Show_Mode == Show_HMS24)	// 24小时制
	{
		*pTimestr++ = timedata->tm_hour/10+'0';		//--时十位
		*pTimestr++ = timedata->tm_hour%10+'0';		//--时个位
		*pTimestr++ = ':';							//--冒号
		*pTimestr++ = (timedata->tm_min/10)%6+'0';	//--分十位
		*pTimestr++ = timedata->tm_min%10+'0';		//--分个位
		*pTimestr++ = ':';						  	//--冒号
		*pTimestr++ = (timedata->tm_sec/10)%6+'0';	//--秒十位
		*pTimestr++ = timedata->tm_sec%10+'0';		//--秒个位
		*pTimestr = '\0';							//--填充结束符   	
	}
	else	// 12小时制
	{		
		if( timedata->tm_hour > 12 ) //--12小时制式，所以要先判断上下午	
		{	
			timedata->tm_hour -= 12;
			Is_AMorPM = 1;
		}
		*pTimestr++ = timedata->tm_hour/10+'0';			  //--时十位
		*pTimestr++ = timedata->tm_hour%10+'0';			  //--时个位
		if(Show_Mode == Show_HM)	*pTimestr++ = ' ';//--空格
		*pTimestr++ = ':';						      //--冒号
		if(Show_Mode == Show_HM)	*pTimestr++ = ' ';//--空格
		*pTimestr++ = (timedata->tm_min/10)%6+'0';		  //--分十位
		*pTimestr++ = timedata->tm_min%10+'0';    		  //--分个位
		if(Show_Mode == Show_HMS)
		{
			*pTimestr++ = ':';						  
			*pTimestr++ = (timedata->tm_sec/10)%6+'0';	  //--秒十位
			*pTimestr++ = timedata->tm_sec%10+'0';		  //--秒个位
		}
		*pTimestr++ = ' ';	  						  //--空格
		if(Is_AMorPM)	
		{
			*pTimestr++ = 'P';  		  //--判断是上午还是下午
		}
		else			
		{
			*pTimestr++ = 'A';
		}
		*pTimestr++ = 'M';
		*pTimestr = '\0';							  //--填充结束符   
	}
}



