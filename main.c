/**
  ******************************************************************************
  * @file    main.c
  * @author  yaoohui
  * @version V1.0
  * @date    2016��2��20��
  * @brief   
  ******************************************************************************
*/ 
#include "stm32f10x_it.h"
#include "stm32arduino.h"
#include "usbh_core.h"
#include "usbh_custom_core.h"
#include "usbh_custom_usr.h"
#include "ff.h"			// FATFS
#include "fatfs_sd_demo.h"	// SD���ļ�ϵͳӦ�ú���

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USB_OTG_CORE_HANDLE           USB_OTG_Core __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USBH_HOST                     USB_Host     __ALIGN_END ;

//__IO uint8_t TxBuffer[64];//USB���ͻ�����

uint8_t sd_id[20];
int8_t trytimes_0 = 1, trytimes_1 = 1, trytimes_2 = 8, loopsteps = 0;
uint8_t IsUsartCmd = 0;	// �Ƿ�Ϊ�������1-��
uint8_t IsShowDebuf = 0;	// �Ƿ���ʾ��Ϣ��1-��ʾ

extern volatile uint8_t RXFlag;				// ���ڴ��ڽ���������ɱ�־��
extern volatile uint8_t RxBuffer[];					// ���ڽ��ջ�����
extern __IO uint16_t ReadRxBufferPointer;
//extern __IO uint16_t RxBufferDataLength;	// ���ڽ������ݳ��ȣ���������+����+У��

//extern uint8_t USBH_HID_EnumDone;			// ö�ٳɹ���־
FATFS fs;   /* ��Ҫ���ó�ȫ�ֱ�������������systick��Ӧ�ٶۻ��޷����ļ� Work area (file system object) for logical drive */
extern uint16_t FileCount;
extern FIL file;
extern __IO USBH_USR_Application_State AppState;
extern uint8_t CommunicationSteps;	// ��ʼͨ��ʱ�ķ�������Ĵ���
extern uint8_t FlowState;			// ����״̬
extern uint8_t ParameterData[80];		// ������ 0x01��80�ֽ�
extern uint8_t ReceiveData[4096];		// ������ 0x80��4096�ֽ�



/* �������� -------------------------------------------------------------------	*/
static void InitBoard(void);
static uint8_t CheckUSART(void);
uint8_t Stage1(void);// �����ļ�
uint8_t Stage2(void);// ��������
uint8_t Stage3(void);// ��ʼͨ��
uint8_t Stage4(void);// ѭ��

void OpenFile(void);

/* ���� -----------------------------------------------------------------------	*/
void SYSTICK_Configuration(void)
{
	if(SysTick_Config(SystemCoreClock / 1000))
	while(1);
}
/*
*********************************************************************************************************
*	�� �� ��: InitBoard
*	����˵��: ��ʼ��Ӳ���豸
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void InitBoard(void)
{
	uint8_t i,res;
	u32 j;
	char temp[20]="/";
	uint32_t total, free;	/* Free and total space */
	char buf[30];
	
	SYSTICK_Configuration();			// ����Systickʱ��1msϵͳʱ���ж�
	USART2_Init(115200);				// ���ô��ڣ�����printf���
	RTC_Initialize();					// ����RTCʱ��
	LED_Init();							// LED����
	Button_Init(BUTTON_MODE_EXTI);		// ������ʼ������Ϊ�ⲿ�ж�ģʽ
	SD_Init();                          // ��ʼ��TF��
	SD_GetCID(sd_id);
	
	printf("sd_id = ");
	for(i = 0; i < 20; i++)
	{
		printf("%d ",sd_id[i]);
	}
	printf("\r\n");
	
	j = SD_GetCapacity();	// ���SD������
	printf("cap = %u MB\r\n",j/1024/1024);
	
//	SD_TestWrite();
	printf("------------------------------------\r\n");
	
	delay_ms(200);
	
	/* �����ļ�ϵͳ */
	res = f_mount(&fs, "", 0);
	if (res != FR_OK)
	{
		return;
	}
	printf(">SD card mounted��\r\n");
	
//	FATFS_SpeedTest();	// SD���ٶȲ���
	
	// ���SD��
	if(scan_files(temp) == FR_OK)
	{
		printf(">Scan files done!\r\n");
		printf(">Total files: %d\r\n", FileCount);
	}
	// ���SD������ʾ�ܿռ��ʣ��ռ�
	if (FATFS_DriveSize(&total, &free) == FR_OK) 
	{
		printf(">Total space: %10d KByte.\r\n>Free space: %10d KByte.\r\n", total, free);		
		sprintf(buf,"Total: %8d KB.",total/1024);
	}
	
	//OpenFile();
	


	USBH_Init(&USB_OTG_Core, 
		#ifdef USE_USB_OTG_FS
			USB_OTG_FS_CORE_ID,
		#else
			USB_OTG_HS_CORE_ID,
		#endif
			&USB_Host,
			&USBH_cb,
			&USR_cb);
}
void ShowLED(void)
{
	if(MyDevice.isconnected)
		LEDOn(LED1);
	else
		LEDOff(LED1);	
	
	if(MyDevice.istarget)
		LEDOn(LED2);
	else
		LEDOff(LED2);
}
/*
*********************************************************************************************************
*	�� �� ��: main
*	����˵��: ������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
int main(void)
{
	__IO uint32_t i = 0;
//	static int8_t trytimes_1 = 1, trytimes_2 = 8, loopsteps = 0;
	static uint8_t initstate = 0;
    InitBoard();	// ��ʼ��Ӳ���豸
	delay_ms(500);
	
	
	while (1)
	{
		switch(initstate)
		{
			case 0:
				initstate = Stage1();
				break;
			
			case 1:
				initstate = Stage2();
				break;
			case 2:
				initstate = Stage3();
				break;
			case 3:
				initstate = Stage4();
				break;
			
			case 0xFE:// ��������
				FlowState = FLOW_IDLE;
				CommunicationSteps = 0;
				initstate = 0;
				trytimes_0 = 1; trytimes_1 = 1; trytimes_2 = 8; loopsteps = 0;			
				break;
			
			default:// 0xff
				FlowState = FLOW_IDLE;
				MyDeviceType_Init();
				CommunicationSteps = 0;
				initstate = 0;
				trytimes_0 = 1; trytimes_1 = 1; trytimes_2 = 8; loopsteps = 0;
				break;
		}
		
		ShowLED();		// LEDָʾ��
		CheckUSART();	// ��鴮�ڽ�������
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		/*
		if((USB_Host.gState == HOST_CLASS_REQUEST) && (AppState == USBH_USR_Application_IDLE))	// ʶ���豸
		{
			AppState = USBH_USR_Application_INITSTATE;	// CMD06��ѯ�����ã��ǣ�����1
		}
		
		// ���в�������
		if(trytimes_1>0 && (AppState == USBH_USR_Application_IDLE) && (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)
		{
			printf("-----------------------------------------------");
			printf("��������\r\n");
			FlowState = FLOW_SET_PARA;// ��������״̬����������
			AppState = USBH_USR_Application_WRITEPAR11;	// CMD00+80�ֽڣ�д������11
			trytimes_1--;
		}
		
		if((trytimes_2>0) && (FlowState == FLOW_COMMUNICATION) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)	// �������̣���ʼͨ��
		{
			trytimes_2--;
			switch(CommunicationSteps)
			{
				case 0:
					AppState = USBH_USR_Application_WRITEPAR31;	// next CMD00���Ͳ�����31
					break;
				case 1:
					AppState = USBH_USR_Application_READPAR;	// next CMD01������
					break;
				case 2:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80�ֽڲ���
					break;
				case 3:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80�ֽڲ���
					break;			
				case 4:
					AppState = USBH_USR_Application_READPAR;	// next CMD01������
					break;
				case 5:
					AppState = USBH_USR_Application_READPAR;	// next CMD01������
					break;
				case 6:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80�ֽڲ���
					break;
				case 7:
					FlowState = FLOW_LOOP;
					loopsteps = 0;
					MyDevice.commstate = COMM_STATE_IDLE;
					printf("-----------------------------------------------");
					printf("ѭ����ʼ\r\n");
					break;
			}
		}
		
		if((FlowState == FLOW_LOOP) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)	// �������̣�ѭ����ʼ
		{
			switch(MyDevice.commstate)
			{
				case COMM_STATE_IDLE:		
					delay_ms(100);
					switch(loopsteps)
					{
						case 0:
							AppState = USBH_USR_Application_READDATA;	// CMD80������ 4096�ֽ�
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 1:
							AppState = USBH_USR_Application_READPAR;	// CMD01������ 80�ֽ�
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 2:
							AppState = USBH_USR_Application_WRITEPARA;	// CMD00 + 80�ֽڲ���
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
					}				
					break;
					
				case COMM_STATE_OK:	// ����������ɣ�����ִ����һ������
					loopsteps++;
					loopsteps %= 3;
					MyDevice.commstate = COMM_STATE_IDLE;
					break;
				
				case COMM_STATE_BUSY:	// ���ڽ�������æ���ȴ�
					break;
				
				default:	// ��ʱ��������ݲ��ɹ����ص�ѭ����ʼ
					loopsteps = 0;
					AppState = USBH_USR_Application_READDATA;	
					MyDevice.commstate = COMM_STATE_IDLE;
					break;
			}

		}
		
//		if (i++ == 0x10000)
//		{
//			LEDToggle(LED1);
//			i = 0;
//		}    */  

	}
}
uint8_t Stage1(void)	// �����ļ�
{
//	static int8_t trytimes_0 = 1;
	while(1)
	{
		ShowLED();		// LEDָʾ��
		if(CheckUSART())	// ��鴮�ڽ�������
		{
			trytimes_0 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// �豸�Ͽ�������
		{
			trytimes_0 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		if(trytimes_0>0 && (AppState == USBH_USR_Application_IDLE) && MyDevice.isconnected && MyDevice.istarget)
		//if((USB_Host.gState == HOST_CLASS_REQUEST) && (AppState == USBH_USR_Application_IDLE))	// ʶ���豸
		{
			printf("-----------------------------------------------");
			printf("�����ļ�\r\n");
			AppState = USBH_USR_Application_INITSTATE;	// CMD06��ѯ�����ã��ǣ�����1
			trytimes_0--;
		}	
		
		if(MyDevice.configstate == CONFIG_STATE_CONFIGURED)// ��������ɺ󣬽�����һ�׶�
		{
			return 1;
		}
	}
}
uint8_t Stage2(void)	// ��������
{
//	static int8_t trytimes_1 = 1;
	while(1)
	{
		ShowLED();		// LEDָʾ��
		if(CheckUSART())	// ��鴮�ڽ�������
		{
			trytimes_1 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// �豸�Ͽ�������
		{
			trytimes_1 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		// ���в�������
		if(trytimes_1>0 && (AppState == USBH_USR_Application_IDLE) && (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)
		{
			printf("-----------------------------------------------");
			printf("��������\r\n");
			FlowState = FLOW_SET_PARA;// ��������״̬����������
			AppState = USBH_USR_Application_WRITEPAR11;	// CMD00+80�ֽڣ�д������11
			trytimes_1--;
		}	
		
		if(FlowState == FLOW_COMMUNICATION) // ����������ɣ�
		{
			return 2;
		}
	}
}
uint8_t Stage3(void)
{
//	static int8_t trytimes_2 = 8;
	
	while(1)
	{
		ShowLED();		// LEDָʾ��
		if(CheckUSART())	// ��鴮�ڽ�������
		{
			trytimes_2 = 8;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// �豸�Ͽ�������
		{
			trytimes_2 = 8;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		if((trytimes_2>0) && (FlowState == FLOW_COMMUNICATION) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)	// �������̣���ʼͨ��
		{
			trytimes_2--;
			switch(CommunicationSteps)
			{
				case 0:
					printf("-----------------------------------------------");			
					printf("��ʼͨ�ţ�\r\n");
					AppState = USBH_USR_Application_WRITEPAR31;	// next CMD00���Ͳ�����31
					break;
				case 1:
					AppState = USBH_USR_Application_READPAR;	// next CMD01������
					break;
				case 2:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80�ֽڲ���
					break;
				case 3:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80�ֽڲ���
					break;			
				case 4:
					AppState = USBH_USR_Application_READPAR;	// next CMD01������
					break;
				case 5:
					AppState = USBH_USR_Application_READPAR;	// next CMD01������
					break;
				case 6:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80�ֽڲ���
					break;
				case 7:
					FlowState = FLOW_LOOP;
//					loopsteps = 0;
					MyDevice.commstate = COMM_STATE_IDLE;
					printf("-----------------------------------------------");
					printf("ѭ����ʼ\r\n");
					return 3;
					
			}
		}
	}
}
uint8_t Stage4(void)
{
//	static int8_t loopsteps = 0;
	uint8_t status;
	
	while(1)
	{
		ShowLED();		// LEDָʾ��
		status = CheckUSART();// ��鴮�ڽ�������
		if(status)	
		{
			loopsteps = 0;
			AppState = USBH_USR_Application_IDLE;
			return status;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// �豸�Ͽ�������
		{
			loopsteps = 0;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;		
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		if((FlowState == FLOW_LOOP) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget && (IsUsartCmd==0))	// �������̣�ѭ����ʼ
		{
			
			switch(MyDevice.commstate)
			{
				case COMM_STATE_IDLE:		
					delay_ms(200);
					switch(loopsteps)
					{
						case 0:
							AppState = USBH_USR_Application_READDATA;	// CMD80������ 4096�ֽ�
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 1:
							AppState = USBH_USR_Application_READPAR;	// CMD01������ 80�ֽ�
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 2:
							AppState = USBH_USR_Application_WRITEPARA;	// CMD00 + 80�ֽڲ���
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
					}				
					break;
					
				case COMM_STATE_OK:	// ����������ɣ�����ִ����һ������
					loopsteps++;
					loopsteps %= 3;
					MyDevice.commstate = COMM_STATE_IDLE;
					break;
				
				case COMM_STATE_BUSY:	// ���ڽ�������æ���ȴ�
					break;
				
				default:	// ��ʱ��������ݲ��ɹ����ص�ѭ����ʼ
					loopsteps = 0;
					AppState = USBH_USR_Application_READDATA;	
					MyDevice.commstate = COMM_STATE_IDLE;
					break;
			}

		}		
	}
}

/*
*********************************************************************************************************
*	�� �� ��: CheckUSART
*	����˵��: �Դ��ڽ��յ���������֤��Ϊָ����ʱ��ִ����Ӧָ��
*	��    ��: ��
*	�� �� ֵ: ��
*				USB �Ƿ����		AA 55 51 AF EF
*				�Ƿ���Ŀ���豸		AA 55 52 AE EF
*				�豸�Ƿ��ʼ��		AA 55 53 AD EF
*				���������豸		AA 55 54 AC EF
*				�Ͽ�����			AA 55 55 AB EF
*				������				AA 55 A1 5F EF
*				������				AA 55 A2 5E EF
*				д����				AA 55 B1 80�ֽ����� У�� EF
*				ִ��cmd				AA 55 C1 ���� У�� EF
*				�򿪻���			AA 55 D1 2F EF
*				�رջ���			AA 55 D2 2E EF
*********************************************************************************************************
*/
static uint8_t CheckUSART(void)
{
	uint16_t i,j,index;
	uint8_t cmd, usbcmd;
	
	if(RXFlag)
	{
		index = ReadRxBufferPointer;
		cmd = RxBuffer[index];
		{
			switch(cmd)	//����
			{

				case 0x51://USB�Ƿ����
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected)
					{
						printf(">>�豸�����ӣ�\r\n");
					}
					else
					{
						printf(">>�豸δ���ӣ�\r\n");
					}
//					printf("USB_Host.gState = %d\r\n",USB_Host.gState);
					break;
					
				case 0x52://�Ƿ�Ŀ���豸
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected)
					{
						if(MyDevice.istarget)
//						if((USB_Host.device_prop.Dev_Desc.idVendor == 0x0A2D) && (USB_Host.device_prop.Dev_Desc.idProduct == 0x000F))
						{
							printf(">>��⵽Ŀ���豸��\r\n");
						}
						else
						{
							printf(">>�޷�ʶ����豸��\r\n");
						}
					}
					else
					{
						printf(">>�豸δ���ӣ�\r\n");
					}
					break;
					
				case 0x55://�Ͽ�����
					USR_cb.DeviceDisconnected();
					USBH_cb.DeInit(&USB_OTG_Core, &USB_Host);	// �Ͽ�����
					USBH_DeAllocate_AllChannel(&USB_OTG_Core); 
					printf(">>�ѶϿ��豸���ӣ�\r\n");
//					printf("USB_Host.gState = %d\r\n",USB_Host.gState);
					RXFlag = 0;
					return 0xff;
					

				case 0x53://�豸�Ƿ��ʼ��
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
					{
						AppState = USBH_USR_Application_INITSTATE;				
						IsUsartCmd = 1;
					}
					else
					{
						printf(">>�豸δ���ӻ���Ŀ���豸��\r\n");
					}
					break;
				
				case 0x54://������������
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
					{	
						RXFlag = 0;						
						return 0xFE;//AppState = USBH_USR_Application_RECONFIG;
					}
					else
					{
						printf(">>�豸δ���ӻ���Ŀ���豸��\r\n");
					}
					break;
				
				case 0xa1://��ȡ����	���������80�ֽڲ���					
					printf("-----------------------------------------------");
					printf("��ȡ����\r\n");
					for(i=0; i<80; i++)
					{
						if((i%8) == 0)
							printf("\t");
						else if((i%4) == 0)
							printf(" ");
						
						printf("%02X ", ParameterData[i]);
						if(i>0 && ((i%8) == 7))
								printf("\r\n");
					}
					printf("-----------------------------------------------");
					printf("OK\r\n");				
					break;
					
				case 0xa2://��ȡ����	���������4096�ֽ�����
					printf("-----------------------------------------------");
					printf("��ȡ����\r\n");
					for(i=0; i<4096; i++)
					{
						if((i%8) == 0)
							printf("\t");
						else if((i%4) == 0)
							printf(" ");
						
						printf("%02X ", ReceiveData[i]);
						if(i>0 && ((i%8) == 7))
								printf("\r\n");
					}
					printf("-----------------------------------------------");
					printf("OK\r\n");				
					break;
					
				case 0xb1://д����
//					if(RxBufferDataLength != 82)
//					{
//						printf("�������ݳ��ȴ���\r\n");
//					}
//					else
					{
						if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
						{						
										
							for(i=0; i<80; i++)
							{
								j = index + 1 + i;
								if(j >= RXBUFFERSIZE)
									j = j % RXBUFFERSIZE;
								ParameterData[i] = 	RxBuffer[j];
							}
							AppState = USBH_USR_Application_WRITEPARA;
							IsUsartCmd = 1;
						}
						else
						{
							printf(">>�豸δ���ӻ���Ŀ���豸��\r\n");
						}
					}
					break;
				
				case 0xc1://ִ��cmd
					usbcmd = RxBuffer[index+1];
					switch(usbcmd)
					{
						case USB_CMD03:// �����Σ�����1024�ֽ�
							AppState = USBH_USR_Application_READWAVEDATA;	
							IsUsartCmd = 1;						
							break;
						
						case USB_CMD04:// CMD04 + CMD05 �������ã���д����
							if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
							{						
								AppState = USBH_USR_Application_RECONFIG;
								IsUsartCmd = 1;
							}
							else
							{
								printf(">>�豸δ���ӻ���Ŀ���豸��\r\n");
							}
							break;
							
						case USB_CMD06:// ѯ������
							if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
							{
								AppState = USBH_USR_Application_INITSTATE;				
								IsUsartCmd = 1;
							}
							else
							{
								printf(">>�豸δ���ӻ���Ŀ���豸��\r\n");
							}
							break;
							
						case USB_CMD01:// ������
							AppState = USBH_USR_Application_READPAR;
							IsUsartCmd = 1;
							break;
						
						case USB_CMD00:// д����
//							if(RxBufferDataLength != 82)
//							{
//								printf("�������ݳ��ȴ���\r\n");
//							}
//							else
							{
								if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
								{						
									IsUsartCmd = 1;
												
									for(i=0; i<80; i++)
									{
										j = index + 2 + i;
										if(j >= RXBUFFERSIZE)
											j = j % RXBUFFERSIZE;
										ParameterData[i] = 	RxBuffer[j];
									}
									AppState = USBH_USR_Application_WRITEPARA;
								}
								else
								{
									printf(">>�豸δ���ӻ���Ŀ���豸��\r\n");
								}
							}
							break;
							
						case USB_CMD80:// ������
							AppState = USBH_USR_Application_READDATA;
							IsUsartCmd = 1;
							break;
					}
					break;
				
				case 0xd1:// �򿪻���
					IsShowDebuf = 1;
					printf(">>������\r\n");
					break;
				case 0xd2:// �رջ��ԣ�����Գ�ʼ����					
					IsShowDebuf = 0;
					printf(">>�ػ���\r\n");
					break;
				default:
					break;
			}
		}
		RXFlag = 0;
//		USART_ClearITPendingBit(USART2, USART_IT_RXNE);			// ����жϱ�־λ
//		USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	}
	return 0;
}



#ifdef  USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
		ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{}
}
#endif

/**********************************END OF FILE**********************************/
