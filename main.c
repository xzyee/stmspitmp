/**
  ******************************************************************************
  * @file    main.c
  * @author  yaoohui
  * @version V1.0
  * @date    2016年2月20日
  * @brief   
  ******************************************************************************
*/ 
#include "stm32f10x_it.h"
#include "stm32arduino.h"
#include "usbh_core.h"
#include "usbh_custom_core.h"
#include "usbh_custom_usr.h"
#include "ff.h"			// FATFS
#include "fatfs_sd_demo.h"	// SD卡文件系统应用函数

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

//__IO uint8_t TxBuffer[64];//USB发送缓冲区

uint8_t sd_id[20];
int8_t trytimes_0 = 1, trytimes_1 = 1, trytimes_2 = 8, loopsteps = 0;
uint8_t IsUsartCmd = 0;	// 是否为串口命令，1-是
uint8_t IsShowDebuf = 0;	// 是否显示信息，1-显示

extern volatile uint8_t RXFlag;				// 用于串口接收数据完成标志。
extern volatile uint8_t RxBuffer[];					// 串口接收缓存器
extern __IO uint16_t ReadRxBufferPointer;
//extern __IO uint16_t RxBufferDataLength;	// 串口接收数据长度，包括命令+数据+校验

//extern uint8_t USBH_HID_EnumDone;			// 枚举成功标志
FATFS fs;   /* 需要设置成全局变量，否则会出现systick反应迟钝或无法打开文件 Work area (file system object) for logical drive */
extern uint16_t FileCount;
extern FIL file;
extern __IO USBH_USR_Application_State AppState;
extern uint8_t CommunicationSteps;	// 开始通信时的发送命令的次数
extern uint8_t FlowState;			// 流程状态
extern uint8_t ParameterData[80];		// 读参数 0x01，80字节
extern uint8_t ReceiveData[4096];		// 读数据 0x80，4096字节



/* 函数声明 -------------------------------------------------------------------	*/
static void InitBoard(void);
static uint8_t CheckUSART(void);
uint8_t Stage1(void);// 配置文件
uint8_t Stage2(void);// 参数设置
uint8_t Stage3(void);// 开始通信
uint8_t Stage4(void);// 循环

void OpenFile(void);

/* 函数 -----------------------------------------------------------------------	*/
void SYSTICK_Configuration(void)
{
	if(SysTick_Config(SystemCoreClock / 1000))
	while(1);
}
/*
*********************************************************************************************************
*	函 数 名: InitBoard
*	功能说明: 初始化硬件设备
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void InitBoard(void)
{
	uint8_t i,res;
	u32 j;
	char temp[20]="/";
	uint32_t total, free;	/* Free and total space */
	char buf[30];
	
	SYSTICK_Configuration();			// 配置Systick时钟1ms系统时钟中断
	USART2_Init(115200);				// 配置串口，用于printf输出
	RTC_Initialize();					// 用于RTC时间
	LED_Init();							// LED配置
	Button_Init(BUTTON_MODE_EXTI);		// 按键初始化配置为外部中断模式
	SD_Init();                          // 初始化TF卡
	SD_GetCID(sd_id);
	
	printf("sd_id = ");
	for(i = 0; i < 20; i++)
	{
		printf("%d ",sd_id[i]);
	}
	printf("\r\n");
	
	j = SD_GetCapacity();	// 获得SD卡容量
	printf("cap = %u MB\r\n",j/1024/1024);
	
//	SD_TestWrite();
	printf("------------------------------------\r\n");
	
	delay_ms(200);
	
	/* 挂载文件系统 */
	res = f_mount(&fs, "", 0);
	if (res != FR_OK)
	{
		return;
	}
	printf(">SD card mounted！\r\n");
	
//	FATFS_SpeedTest();	// SD卡速度测试
	
	// 浏览SD卡
	if(scan_files(temp) == FR_OK)
	{
		printf(">Scan files done!\r\n");
		printf(">Total files: %d\r\n", FileCount);
	}
	// 浏览SD卡，显示总空间和剩余空间
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
*	函 数 名: main
*	功能说明: 主函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int main(void)
{
	__IO uint32_t i = 0;
//	static int8_t trytimes_1 = 1, trytimes_2 = 8, loopsteps = 0;
	static uint8_t initstate = 0;
    InitBoard();	// 初始化硬件设备
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
			
			case 0xFE:// 重新配置
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
		
		ShowLED();		// LED指示灯
		CheckUSART();	// 检查串口接收数据
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		/*
		if((USB_Host.gState == HOST_CLASS_REQUEST) && (AppState == USBH_USR_Application_IDLE))	// 识别设备
		{
			AppState = USBH_USR_Application_INITSTATE;	// CMD06，询问配置，是，返回1
		}
		
		// 进行参数设置
		if(trytimes_1>0 && (AppState == USBH_USR_Application_IDLE) && (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)
		{
			printf("-----------------------------------------------");
			printf("参数设置\r\n");
			FlowState = FLOW_SET_PARA;// 程序流程状态：参数设置
			AppState = USBH_USR_Application_WRITEPAR11;	// CMD00+80字节，写参数包11
			trytimes_1--;
		}
		
		if((trytimes_2>0) && (FlowState == FLOW_COMMUNICATION) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)	// 程序流程：开始通信
		{
			trytimes_2--;
			switch(CommunicationSteps)
			{
				case 0:
					AppState = USBH_USR_Application_WRITEPAR31;	// next CMD00发送参数包31
					break;
				case 1:
					AppState = USBH_USR_Application_READPAR;	// next CMD01读参数
					break;
				case 2:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80字节参数
					break;
				case 3:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80字节参数
					break;			
				case 4:
					AppState = USBH_USR_Application_READPAR;	// next CMD01读参数
					break;
				case 5:
					AppState = USBH_USR_Application_READPAR;	// next CMD01读参数
					break;
				case 6:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80字节参数
					break;
				case 7:
					FlowState = FLOW_LOOP;
					loopsteps = 0;
					MyDevice.commstate = COMM_STATE_IDLE;
					printf("-----------------------------------------------");
					printf("循环开始\r\n");
					break;
			}
		}
		
		if((FlowState == FLOW_LOOP) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)	// 程序流程：循环开始
		{
			switch(MyDevice.commstate)
			{
				case COMM_STATE_IDLE:		
					delay_ms(100);
					switch(loopsteps)
					{
						case 0:
							AppState = USBH_USR_Application_READDATA;	// CMD80读数据 4096字节
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 1:
							AppState = USBH_USR_Application_READPAR;	// CMD01读参数 80字节
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 2:
							AppState = USBH_USR_Application_WRITEPARA;	// CMD00 + 80字节参数
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
					}				
					break;
					
				case COMM_STATE_OK:	// 接收数据完成，继续执行下一条命令
					loopsteps++;
					loopsteps %= 3;
					MyDevice.commstate = COMM_STATE_IDLE;
					break;
				
				case COMM_STATE_BUSY:	// 正在接收数据忙，等待
					break;
				
				default:	// 超时或接收数据不成功，回到循环开始
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
uint8_t Stage1(void)	// 配置文件
{
//	static int8_t trytimes_0 = 1;
	while(1)
	{
		ShowLED();		// LED指示灯
		if(CheckUSART())	// 检查串口接收数据
		{
			trytimes_0 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// 设备断开，返回
		{
			trytimes_0 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		if(trytimes_0>0 && (AppState == USBH_USR_Application_IDLE) && MyDevice.isconnected && MyDevice.istarget)
		//if((USB_Host.gState == HOST_CLASS_REQUEST) && (AppState == USBH_USR_Application_IDLE))	// 识别设备
		{
			printf("-----------------------------------------------");
			printf("配置文件\r\n");
			AppState = USBH_USR_Application_INITSTATE;	// CMD06，询问配置，是，返回1
			trytimes_0--;
		}	
		
		if(MyDevice.configstate == CONFIG_STATE_CONFIGURED)// 已配置完成后，进入下一阶段
		{
			return 1;
		}
	}
}
uint8_t Stage2(void)	// 参数设置
{
//	static int8_t trytimes_1 = 1;
	while(1)
	{
		ShowLED();		// LED指示灯
		if(CheckUSART())	// 检查串口接收数据
		{
			trytimes_1 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// 设备断开，返回
		{
			trytimes_1 = 1;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		// 进行参数设置
		if(trytimes_1>0 && (AppState == USBH_USR_Application_IDLE) && (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)
		{
			printf("-----------------------------------------------");
			printf("参数设置\r\n");
			FlowState = FLOW_SET_PARA;// 程序流程状态：参数设置
			AppState = USBH_USR_Application_WRITEPAR11;	// CMD00+80字节，写参数包11
			trytimes_1--;
		}	
		
		if(FlowState == FLOW_COMMUNICATION) // 参数设置完成！
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
		ShowLED();		// LED指示灯
		if(CheckUSART())	// 检查串口接收数据
		{
			trytimes_2 = 8;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// 设备断开，返回
		{
			trytimes_2 = 8;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		if((trytimes_2>0) && (FlowState == FLOW_COMMUNICATION) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget)	// 程序流程：开始通信
		{
			trytimes_2--;
			switch(CommunicationSteps)
			{
				case 0:
					printf("-----------------------------------------------");			
					printf("开始通信！\r\n");
					AppState = USBH_USR_Application_WRITEPAR31;	// next CMD00发送参数包31
					break;
				case 1:
					AppState = USBH_USR_Application_READPAR;	// next CMD01读参数
					break;
				case 2:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80字节参数
					break;
				case 3:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80字节参数
					break;			
				case 4:
					AppState = USBH_USR_Application_READPAR;	// next CMD01读参数
					break;
				case 5:
					AppState = USBH_USR_Application_READPAR;	// next CMD01读参数
					break;
				case 6:
					AppState = USBH_USR_Application_WRITEPARA;	// next CMD00 + 80字节参数
					break;
				case 7:
					FlowState = FLOW_LOOP;
//					loopsteps = 0;
					MyDevice.commstate = COMM_STATE_IDLE;
					printf("-----------------------------------------------");
					printf("循环开始\r\n");
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
		ShowLED();		// LED指示灯
		status = CheckUSART();// 检查串口接收数据
		if(status)	
		{
			loopsteps = 0;
			AppState = USBH_USR_Application_IDLE;
			return status;
		}
		if(HCD_IsDeviceConnected(&USB_OTG_Core) == 0)	// 设备断开，返回
		{
			loopsteps = 0;
			AppState = USBH_USR_Application_IDLE;
			return 0xff;		
		}
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core , &USB_Host);
		
		if((FlowState == FLOW_LOOP) && (AppState == USBH_USR_Application_IDLE) \
			&& (MyDevice.configstate == CONFIG_STATE_CONFIGURED) \
			&& MyDevice.isconnected && MyDevice.istarget && (IsUsartCmd==0))	// 程序流程：循环开始
		{
			
			switch(MyDevice.commstate)
			{
				case COMM_STATE_IDLE:		
					delay_ms(200);
					switch(loopsteps)
					{
						case 0:
							AppState = USBH_USR_Application_READDATA;	// CMD80读数据 4096字节
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 1:
							AppState = USBH_USR_Application_READPAR;	// CMD01读参数 80字节
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
						
						case 2:
							AppState = USBH_USR_Application_WRITEPARA;	// CMD00 + 80字节参数
							MyDevice.commstate = COMM_STATE_BUSY;
							break;
					}				
					break;
					
				case COMM_STATE_OK:	// 接收数据完成，继续执行下一条命令
					loopsteps++;
					loopsteps %= 3;
					MyDevice.commstate = COMM_STATE_IDLE;
					break;
				
				case COMM_STATE_BUSY:	// 正在接收数据忙，等待
					break;
				
				default:	// 超时或接收数据不成功，回到循环开始
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
*	函 数 名: CheckUSART
*	功能说明: 对串口接收到的数据验证，为指令码时，执行相应指令
*	形    参: 无
*	返 回 值: 无
*				USB 是否接入		AA 55 51 AF EF
*				是否是目标设备		AA 55 52 AE EF
*				设备是否初始化		AA 55 53 AD EF
*				重新配置设备		AA 55 54 AC EF
*				断开连接			AA 55 55 AB EF
*				读参数				AA 55 A1 5F EF
*				读数据				AA 55 A2 5E EF
*				写参数				AA 55 B1 80字节数据 校验 EF
*				执行cmd				AA 55 C1 命令 校验 EF
*				打开回显			AA 55 D1 2F EF
*				关闭回显			AA 55 D2 2E EF
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
			switch(cmd)	//功能
			{

				case 0x51://USB是否接入
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected)
					{
						printf(">>设备已连接！\r\n");
					}
					else
					{
						printf(">>设备未连接！\r\n");
					}
//					printf("USB_Host.gState = %d\r\n",USB_Host.gState);
					break;
					
				case 0x52://是否目标设备
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected)
					{
						if(MyDevice.istarget)
//						if((USB_Host.device_prop.Dev_Desc.idVendor == 0x0A2D) && (USB_Host.device_prop.Dev_Desc.idProduct == 0x000F))
						{
							printf(">>检测到目标设备！\r\n");
						}
						else
						{
							printf(">>无法识别的设备！\r\n");
						}
					}
					else
					{
						printf(">>设备未连接！\r\n");
					}
					break;
					
				case 0x55://断开连接
					USR_cb.DeviceDisconnected();
					USBH_cb.DeInit(&USB_OTG_Core, &USB_Host);	// 断开连接
					USBH_DeAllocate_AllChannel(&USB_OTG_Core); 
					printf(">>已断开设备连接！\r\n");
//					printf("USB_Host.gState = %d\r\n",USB_Host.gState);
					RXFlag = 0;
					return 0xff;
					

				case 0x53://设备是否初始化
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
					{
						AppState = USBH_USR_Application_INITSTATE;				
						IsUsartCmd = 1;
					}
					else
					{
						printf(">>设备未连接或不是目标设备！\r\n");
					}
					break;
				
				case 0x54://重新配置设置
					if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
					{	
						RXFlag = 0;						
						return 0xFE;//AppState = USBH_USR_Application_RECONFIG;
					}
					else
					{
						printf(">>设备未连接或不是目标设备！\r\n");
					}
					break;
				
				case 0xa1://读取参数	最近读出的80字节参数					
					printf("-----------------------------------------------");
					printf("读取参数\r\n");
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
					
				case 0xa2://读取数据	最近读出的4096字节数据
					printf("-----------------------------------------------");
					printf("读取数据\r\n");
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
					
				case 0xb1://写参数
//					if(RxBufferDataLength != 82)
//					{
//						printf("参数数据长度错误！\r\n");
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
							printf(">>设备未连接或不是目标设备！\r\n");
						}
					}
					break;
				
				case 0xc1://执行cmd
					usbcmd = RxBuffer[index+1];
					switch(usbcmd)
					{
						case USB_CMD03:// 读波形，返回1024字节
							AppState = USBH_USR_Application_READWAVEDATA;	
							IsUsartCmd = 1;						
							break;
						
						case USB_CMD04:// CMD04 + CMD05 进入配置，并写配置
							if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
							{						
								AppState = USBH_USR_Application_RECONFIG;
								IsUsartCmd = 1;
							}
							else
							{
								printf(">>设备未连接或不是目标设备！\r\n");
							}
							break;
							
						case USB_CMD06:// 询问配置
							if(HCD_IsDeviceConnected(&USB_OTG_Core) && MyDevice.isconnected && MyDevice.istarget)
							{
								AppState = USBH_USR_Application_INITSTATE;				
								IsUsartCmd = 1;
							}
							else
							{
								printf(">>设备未连接或不是目标设备！\r\n");
							}
							break;
							
						case USB_CMD01:// 读参数
							AppState = USBH_USR_Application_READPAR;
							IsUsartCmd = 1;
							break;
						
						case USB_CMD00:// 写参数
//							if(RxBufferDataLength != 82)
//							{
//								printf("参数数据长度错误！\r\n");
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
									printf(">>设备未连接或不是目标设备！\r\n");
								}
							}
							break;
							
						case USB_CMD80:// 读数据
							AppState = USBH_USR_Application_READDATA;
							IsUsartCmd = 1;
							break;
					}
					break;
				
				case 0xd1:// 打开回显
					IsShowDebuf = 1;
					printf(">>开回显\r\n");
					break;
				case 0xd2:// 关闭回显，仅针对初始化后					
					IsShowDebuf = 0;
					printf(">>关回显\r\n");
					break;
				default:
					break;
			}
		}
		RXFlag = 0;
//		USART_ClearITPendingBit(USART2, USART_IT_RXNE);			// 清除中断标志位
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
