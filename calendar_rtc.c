/*
*********************************************************************************************************
*
*	ģ������ : RTCģ��
*	�ļ����� : calendar_rtc.c
*	��    �� : V1.2
*	˵    �� : 
*				����RTC����
*				
*	�޸ļ�¼ :
*		�汾��  ����       	����    ˵��
*		v1.1    2013-10-23	JOY		
*		v1.2    2014-9-24	JOY		����RTC����
*		v1.3	2015-06-29			RTC_Initialize��ʼ��������Time_SetCalendarAlarm(now);	// ���ó�ʼĬ������ʱ��
*
*********************************************************************************************************
*/
#include "calendar_rtc.H"

uint16_t RTC_year, RTC_month, RTC_date, RTC_week, RTC_hour, RTC_minute, RTC_second; // �꣬��/��/�L/�r/��/��

//--���ȴ���һ��������ʱ��
//--��������ʱ��,������ʱ�������ڵȵ�,������鿴time.h
//ע�⣺�·ݷ�ΧΪ1~12��Time_SetCalendarTime�������ѽ�tm_mon--, ����time.h��tm_mon�ķ�Χ0-11
//		���ڷ�ΧΪ0~6���������տ�ʼ
struct tm now={59, 58, 15, 29, 6, 2015, 1};	// 2015-06-29 15:58:59 ����һ



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
//	// ʹ��ALARM�ж�
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
*	�� �� ��: RTC_Initialize
*	����˵��: �ϵ�ʱֻ��Ҫ���ñ��������Զ�����Ƿ���ҪRTC��ʼ����
*			  ����Ҫ���³�ʼ��RTC�������RTC_Configuration()�����Ӧ����
*	��    �Σ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void RTC_Initialize(void)
{
	/* ��BKP�ĺ󱸼Ĵ���1�У�����һ�������ַ�0xA5A5 */
    /* ��һ���ϵ��󱸵�Դ����󣬸üĴ������ݶ�ʧ */
    /* ����RTC���ݶ�ʧ����Ҫ�������� */
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
//		printf("\r\n\n RTC not yet configured....");

		/* RTC Configuration */
		RTC_Configuration();
		RTC_NVICConfiguration();
//		printf("\r\n RTC configured....");

// 		/* Adjust time by values entred by the user on the hyperterminal */
		Time_SetCalendarTime(now);	// ���ó�ʼĬ��ʱ��
		Time_SetCalendarAlarm(now);	// ���ó�ʼĬ������ʱ��

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
		
	}
	else
	{
		RTC_NVICConfiguration();
		
		/* ���󱸼Ĵ���û�е��磬��������������RTC */
		/* �����������RCC_GetFlagStatus()�����鿴���θ�λ���� */
		  /* Check if the Power On Reset flag is set */
		if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
		{
//		  printf("\r\n\n Power On Reset occurred....");	// �ϵ縴λ
		}
		/* Check if the Pin Reset flag is set */
		else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
		{
//		  printf("\r\n\n External Reset occurred....");	// �ⲿ�ܽŸ�λ
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
	printf("\r\n �������ø�ʽ(10�ֽ�)��");
	printf("\r\n 1. ������������ʱ��(����������ʱ����)��0xAA, 0x99, 0xF0, 0xyy, 0xmm, 0xdd, 0xww, 0xmm, 0xss, 0xss");
	printf("\r\n 2. ����ʱ��(ʱ���� 24Сʱ��)�� 0xAA, 0x99, 0xF1, 0xhh, 0xmm, 0xss, 0x00, 0x00, 0x00, 0x00");	// ��������ʱ���ʽ
	printf("\r\n 3. ��������(������)��0xAA, 0x99, 0xF2, 0xyy, 0xmm, 0xdd, 0x00, 0x00, 0x00, 0x00");
	printf("\r\n 4. ��������ʱ��(ʱ���� 24Сʱ��)�� 0xAA, 0x99, 0xF3, 0xhh, 0xmm, 0xss, 0x00, 0x00, 0x00, 0x00\r\n");	// ��������ʱ���ʽ
	printf("------------------------------------\r\n");
*/	
    /* Clear reset flags */
	RCC_ClearFlag();	// ���RCC�и�λ��־
	
	/*����PWR��BKP��ʱ�ӣ�from APB1��*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP,ENABLE);
    PWR_BackupAccessCmd(ENABLE);/*�������*/
}


/******************************************************************************
* ������  : Time_ConvUnixToCalendar()
* ����    : ת��UNIXʱ���Ϊ����ʱ��
* ����    : u32 t  ��ǰʱ���UNIXʱ���
* ���    : None
* ����    : struct tm
* ��������: 2012.1.1
* �汾    : V1.00
*******************************************************************************/
struct tm Time_ConvUnixToCalendar(time_t t)
{
    struct tm *t_tm ;
    t_tm=localtime(&t);
    t_tm->tm_year+=1900 ;
    /*localtimeת�������tm_year�����ֵ����Ҫת�ɾ���ֵ*/
    return *t_tm ;
}


/******************************************************************************
* ������  : Time_ConvCalendarToUnix()
* ����    : д��RTCʱ�ӵ�ǰʱ��,�����˿⺯����1900�꿪ʼ������!!
* ����    : struct tm t
* ���    : None
* ����    : time_t
* ��������: 2012.1.1
* �汾    : V1.00
*******************************************************************************/
time_t Time_ConvCalendarToUnix(struct tm t)
{
    t.tm_year-=1900 ;
    /*�ⲿtm�ṹ��洢�����Ϊ2008��ʽ*/
    /*��time.h�ж������ݸ�ʽΪ1900�꿪ʼ�����*/
    /*���ԣ�������ת��ʱҪ���ǵ�������ء�*/
    return mktime(&t);
}


/******************************************************************************
* ������  : Time_GetUnixTime()
* ����    : ��RTCȡ��ǰʱ���Unixʱ���ֵ
* ����    : None
* ���    : None
* ����    : time_t t
* ��������: 2012.1.1
* �汾    : V1.00
*******************************************************************************/
time_t Time_GetUnixTime(void)
{
    return (time_t)RTC_GetCounter();
}


/******************************************************************************
* ������  : Time_GetCalendarTime()
* ����    : ��RTCȡ��ǰʱ�������ʱ�䣨struct tm��
* ����    : None
* ���    : None
* ����    : time_t t_t
* ��������: 2012.1.1 
* �汾    : V1.00
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
* ������  : Time_SetUnixTime()
* ����    : ��������Unixʱ���д��RTC
* ����    : None
* ���    : None
* ����    : None
* ��������: 2012.1.1
* �汾    : V1.00
*******************************************************************************/
void Time_SetUnixTime(time_t t)
{
    RTC_WaitForLastTask();
    RTC_SetCounter((u32)t);
    RTC_WaitForLastTask();
    return ;
}

/******************************************************************************
* ������  : Time_SetCalendarTime()
* ����    : ��������Calendar��ʽʱ��ת����UNIXʱ���д��RTC
* ����    : None
* ���    : None
* ����    : None
* ��������: 2012.1.1
* �汾    : V1.00
*******************************************************************************/
void Time_SetCalendarTime(struct tm t)
{
    t.tm_mon--;
	Time_SetUnixTime(Time_ConvCalendarToUnix(t));
    return ;
}

///////***************�������ӹ���*********************************************/////////
uint32_t RTC_GetAlarm(void)
{
  uint16_t tmp = 0;
  tmp = RTC->ALRL;
  return (((uint32_t)RTC->ALRH << 16 ) | tmp) ;
}

/******************************************************************************
* ������  : Time_GetCalendarAlarm()
* ����    : ��RTC�����ӼĴ���ȡ��ǰʱ�������ʱ�䣨struct tm��
* ����    : None
* ���    : None
* ����    : time_t t_t
* ��������: 2012.1.1 
* �汾    : V1.00
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
* ������  : Time_SetUnixAlarm()
* ����    : ��������Unixʱ���д��RTC�����ӼĴ���(RTC_ALR)
* ����    : struct tm t��ʱ��ṹ��
* ���    : None
* ����    : None
* ��������: 2014.9.24
* �汾    : V1.00
*******************************************************************************/
void Time_SetUnixAlarm(time_t t)
{
    RTC_WaitForLastTask();
    RTC_SetAlarm((u32)t);
    RTC_WaitForLastTask();
    return ;
}
/******************************************************************************
* ������  : Time_SetCalendarAlarm()
* ����    : ��������Calendar��ʽʱ��ת����UNIXʱ���д��RTC�����ӼĴ���(RTC_ALR)
* ����    : struct tm t��ʱ��ṹ��
* ���    : None
* ����    : None
* ��������: 2014.9.24
* �汾    : V1.00
*******************************************************************************/
void Time_SetCalendarAlarm(struct tm t)
{
    t.tm_mon--;
	Time_SetUnixAlarm(Time_ConvCalendarToUnix(t));
    return ;
}

/******************************************************************************
* ������  : Time_CalendarCopy()
* ����    : ��Calendar2�е�ʱ�����ݿ�����Calendar1��
* ����    : Calendar1��Calendar2:ʱ������
* ���    : None
* ����    : None
* ����    : ��ҶǶ��ʽ���
* ��������: 2012.1.1
* �汾    : V1.00
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
* ������  : Time_GetTimeString()
* ����    : ����ǰʱ��(Сʱ:����)ת��Ϊ�ַ���
*		  		:	Show_HMģʽ(ֻ��ʾʱ�ͷ�)��11���ַ�,���緵��:"12 : 08 PM"
*		  		:	Show_HMSģʽ(��ʾʱ����)��12���ַ�,���緵��:"12:08:50 PM"
*		  		:	Show_YMDģʽ(��ʾ������)��15���ַ�,���緵��:"2012��01��01�� "
*				:	Show_HMS24(��ʾʱ���룬24Сʱ��ʽ)��9���ַ�
*				:	Show_WEEK(��ʾ����)��7���ַ�������"������"
*		  		: ����������ʾģʽ���һ���ַ�������������'\0'
* ����    : tm:unix��ʽ��ʱ����� *Timestr:�ַ����洢��λ�� Show_Mode:Ҫ��ȡ��ʱ������
* ���    : None
* ����    : None
* ����    : ��ҶǶ��ʽ���+ JOY
* ��������: 2012.1.1
* �汾    : V1.1
* ��ע	  : ��������ģʽ,���ȶ���һ��
* ��ע	  : char bufname[Show_HM_Buf��Show_HMS_Buf��Show_YMD_Buf]�����飬
*		  : Ȼ�����Time_GetTimeString(tm,bufname,Show_HM);
*		  : ����	Time_GetTimeString(tm,bufname,Show_HMS);
*		  : ����	Time_GetTimeString(tm,bufname,Show_YMD);
*		  : ����ʱ����ַ����ͻ�洢��bufname[]��,�Թ�ʹ��
*******************************************************************************/
void Time_GetTimeString(struct tm *timedata, char *Timestr, uint8_t Show_Mode)
{
    char *pTimestr = Timestr;
	char chinese[6] = "������";
	char *pto_chinese = chinese;
	uint8_t Is_AMorPM = 0;
	
	if(Show_Mode == Show_YMD)
	{
		//--�洢��
		*pTimestr++ = timedata->tm_year/1000+'0';//--��ǧλ
		*pTimestr++ = timedata->tm_year/100%10+'0'; //--���λ
		*pTimestr++ = timedata->tm_year/10%10+'0';  //--��ʮλ	
		*pTimestr++ = timedata->tm_year%10+'0';  //--��ʮλ
		*pTimestr++	= *pto_chinese++;//--�Ѻ���"��"���ƽ�ȥ
		*pTimestr++ = *pto_chinese++;
		//--�洢��	
		*pTimestr++ = timedata->tm_mon/10+'0';//--��ʮλ
		*pTimestr++ = timedata->tm_mon%10+'0';//--�¸�λ
		*pTimestr++	= *pto_chinese++;//--�Ѻ���"��"���ƽ�ȥ
		*pTimestr++ = *pto_chinese++;
		//--�洢��
		*pTimestr++ = timedata->tm_mday/10+'0';//--��ʮλ
		*pTimestr++ = timedata->tm_mday%10+'0';//--�ո�λ
		*pTimestr++	= *pto_chinese++;//--�Ѻ���"��"���ƽ�ȥ
		*pTimestr++ = *pto_chinese++;
		*pTimestr = '\0';
		return;
	}
	
	if(Show_Mode == Show_WEEK)	//��ʾ����
	{
		switch(timedata->tm_wday)
		{
			case Sunday:
				sprintf(pTimestr, "������");
				break;
			case Monday:
				sprintf(pTimestr, "����һ");
				break;
			case Tuesday:
				sprintf(pTimestr, "���ڶ�");
				break;
			case Wednesday:
				sprintf(pTimestr, "������");
				break;
			case Thursday:
				sprintf(pTimestr, "������");
				break;
			case Friday:
				sprintf(pTimestr, "������");
				break;
			case Saturday:
				sprintf(pTimestr, "������");
				break;
			default:
				break;
		}
		return;
	}
	
	if(Show_Mode == Show_HMS24)	// 24Сʱ��
	{
		*pTimestr++ = timedata->tm_hour/10+'0';		//--ʱʮλ
		*pTimestr++ = timedata->tm_hour%10+'0';		//--ʱ��λ
		*pTimestr++ = ':';							//--ð��
		*pTimestr++ = (timedata->tm_min/10)%6+'0';	//--��ʮλ
		*pTimestr++ = timedata->tm_min%10+'0';		//--�ָ�λ
		*pTimestr++ = ':';						  	//--ð��
		*pTimestr++ = (timedata->tm_sec/10)%6+'0';	//--��ʮλ
		*pTimestr++ = timedata->tm_sec%10+'0';		//--���λ
		*pTimestr = '\0';							//--��������   	
	}
	else	// 12Сʱ��
	{		
		if( timedata->tm_hour > 12 ) //--12Сʱ��ʽ������Ҫ���ж�������	
		{	
			timedata->tm_hour -= 12;
			Is_AMorPM = 1;
		}
		*pTimestr++ = timedata->tm_hour/10+'0';			  //--ʱʮλ
		*pTimestr++ = timedata->tm_hour%10+'0';			  //--ʱ��λ
		if(Show_Mode == Show_HM)	*pTimestr++ = ' ';//--�ո�
		*pTimestr++ = ':';						      //--ð��
		if(Show_Mode == Show_HM)	*pTimestr++ = ' ';//--�ո�
		*pTimestr++ = (timedata->tm_min/10)%6+'0';		  //--��ʮλ
		*pTimestr++ = timedata->tm_min%10+'0';    		  //--�ָ�λ
		if(Show_Mode == Show_HMS)
		{
			*pTimestr++ = ':';						  
			*pTimestr++ = (timedata->tm_sec/10)%6+'0';	  //--��ʮλ
			*pTimestr++ = timedata->tm_sec%10+'0';		  //--���λ
		}
		*pTimestr++ = ' ';	  						  //--�ո�
		if(Is_AMorPM)	
		{
			*pTimestr++ = 'P';  		  //--�ж������绹������
		}
		else			
		{
			*pTimestr++ = 'A';
		}
		*pTimestr++ = 'M';
		*pTimestr = '\0';							  //--��������   
	}
}



