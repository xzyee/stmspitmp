/**
  ******************************************************************************
  * @file    usbh_custom_usr.c
  * @author  yaoohui
  * @version V1.0
  * @date    2016年2月20日
  * @brief   
  ******************************************************************************
*/ 

#include "usbh_custom_usr.h"
#include "usbh_custom_core.h"
#include "usbh_ioreq.h"
#include <stdio.h>
#include "delay.h"
#include "ff.h"
#include <string.h>

#define TIMEOUTMAX	0x2FFFF
__IO USBH_USR_Application_State AppState;

uint16_t RxDataLength = 0;	// 需要接收设备的数据长度
uint8_t RxDataComplete = 0;	// 接收数据完成标志, 1-接收完成
uint16_t TxDataLength = 0;	// 需要发送的数据长度
uint8_t TxConfigDataTimes = 0;	// 发送带命令的配置文件的次数
uint8_t TxParaPackTimes = 0;	// 发送参数包的次数
uint8_t CommunicationSteps = 0;	// 开始通信时的发送命令的次数
uint8_t FlowState = 0;			// 流程状态

__IO USB_Device_TypeDef 	MyDevice;	// 设备状态结构体

extern USBH_TypeDef USBH_CUSTOM_DEVICE;

uint8_t TxBuffer[1025];
uint8_t ParameterData[80];		// 读参数 0x01，80字节
uint8_t ReceiveData[4096];		// 读数据 0x80，4096字节
uint8_t ReceiveWaveData[1024];	// 读波形 0x03,1024字节
const uint8_t PARA_PACK_11[80] = // 参数包11，共80字节，其中[0]和[76]字节可变
{
0x81, 0x00, 0x0c, 0x03, 0x00, 0x30, 0x20, 0x03, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x01, 0x80, 0x02, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10, 0xc0, 0x0b, 0x00
};

extern USB_OTG_CORE_HANDLE           USB_OTG_Core;
extern __ALIGN_BEGIN uint8_t		RxBuf [4100] __ALIGN_END ;	// 接收缓冲区

extern FATFS fs;   /* 需要设置成全局变量，否则会出现systick反应迟钝或无法打开文件 Work area (file system object) for logical drive */
extern FIL file;

extern CDC_Xfer_TypeDef		CDC_TxParam;
extern uint8_t IsUsartCmd;	// 是否为串口命令，1-是
extern uint8_t IsShowDebuf;	// 是否显示信息，1-显示

//USB_OTG_CORE_HANDLE *pdev;

FRESULT OpenConfigFile(void);
FRESULT ReadConfigFile(uint8_t index);

/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */

USBH_Usr_cb_TypeDef USR_cb =
{
	USBH_USR_Init,
	USBH_USR_DeInit,
	USBH_USR_DeviceAttached,
	USBH_USR_ResetDevice,
	USBH_USR_DeviceDisconnected,
	USBH_USR_OverCurrentDetected,
	USBH_USR_DeviceSpeedDetected,
	USBH_USR_Device_DescAvailable,
	USBH_USR_DeviceAddressAssigned,
	USBH_USR_Configuration_DescAvailable,
	USBH_USR_Manufacturer_String,
	USBH_USR_Product_String,
	USBH_USR_SerialNum_String,
	USBH_USR_EnumerationDone,
	USBH_USR_UserInput,
	USBH_USR_Application,
	USBH_USR_DeviceNotSupported,
	USBH_USR_UnrecoveredError,
};

/*--------------- Messages ---------------*/
const uint8_t MSG_HOST_INIT[]          = " > Host Library Initialized\r\n";
const uint8_t MSG_DEV_ATTACHED[]       = " > Device Attached\r\n";
const uint8_t MSG_DEV_DISCONNECTED[]   = " > Device Disconnected\r\n";
const uint8_t MSG_DEV_ENUMERATED[]     = "  > Enumeration completed\r\n";
const uint8_t MSG_DEV_HIGHSPEED[]      = "  > High speed device detected\r\n";
const uint8_t MSG_DEV_FULLSPEED[]      = "  > Full speed device detected\r\n";
const uint8_t MSG_DEV_LOWSPEED[]       = "  > Low speed device detected\r\n";
const uint8_t MSG_DEV_ERROR[]          = "  > Device fault \r\n";
const uint8_t MSG_UNREC_ERROR[]        = " > UNRECOVERED ERROR STATE\r\n";


// 设备状态相关结构体变量初始化
void MyDeviceType_Init(void)
{
	MyDevice.configstate = CONFIG_STATE_NOTCONFIG;	// 未配置
	MyDevice.isconnected = 0;						// 未连接
	MyDevice.istarget = 0;							// 不是目标设备
	MyDevice.commstate = COMM_STATE_IDLE;			// 空闲	
}

/**
* @brief  USBH_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBH_USR_Init(void)
{
	static uint8_t startup = 0;  
  
	if(startup == 0 )
	{
		startup = 1;
		Button_Init(BUTTON_MODE_GPIO);

		// 设备状态相关结构体变量初始化
		MyDeviceType_Init();
		
		#ifdef USE_USB_OTG_HS 
			printf("USB OTG HS Host\r\n");
		#else
			printf("USB OTG FS Host\r\n");
		#endif
		
		printf(" > USB Host library started.\r\n");

	}
}

/**
* @brief  USBH_USR_DeviceAttached 
*         Displays the message on LCD on device attached
* @param  None
* @retval None
*/
void USBH_USR_DeviceAttached(void)
{  
	MyDevice.isconnected = 1;	// USB设备已连接
	printf((void*)MSG_DEV_ATTACHED);
}

/**
* @brief  USBH_USR_UnrecoveredError
* @param  None
* @retval None
*/
void USBH_USR_UnrecoveredError (void)
{
	printf((void*)MSG_UNREC_ERROR);
}

/**
* @brief  USBH_DisconnectEvent
*         Device disconnect event
* @param  None
* @retval None
*/
void USBH_USR_DeviceDisconnected (void)
{
	MyDeviceType_Init();		// 设备状态相关结构体变量初始化
	printf((void*)MSG_DEV_DISCONNECTED);
}

/**
* @brief  USBH_USR_ResetUSBDevice 
*         Reset USB Device
* @param  None
* @retval None
*/
void USBH_USR_ResetDevice(void)
{
	/* Users can do their application actions here for the USB-Reset */
}


/**
* @brief  USBH_USR_DeviceSpeedDetected 
*         Displays the message on LCD for device speed
* @param  Devicespeed : Device Speed
* @retval None
*/
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
	if(DeviceSpeed == HPRT0_PRTSPD_HIGH_SPEED)
	{
		printf((void*)MSG_DEV_HIGHSPEED);
	}  
	else if(DeviceSpeed == HPRT0_PRTSPD_FULL_SPEED)
	{
		printf((void*)MSG_DEV_FULLSPEED);
	}
	else if(DeviceSpeed == HPRT0_PRTSPD_LOW_SPEED)
	{
		printf((void*)MSG_DEV_LOWSPEED);
	}
	else
	{
		printf((void*)MSG_DEV_ERROR);
	}
}

/**
* @brief  USBH_USR_Device_DescAvailable 
*         Displays the message on LCD for device descriptor
* @param  DeviceDesc : device descriptor
* @retval None
*/
void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{
	USBH_DevDesc_TypeDef *hs;
	hs = DeviceDesc;  

	printf("  > VID : %04Xh \r\n" , (uint32_t)(*hs).idVendor);
	printf("  > PID : %04Xh \r\n" , (uint32_t)(*hs).idProduct);
}

/**
* @brief  USBH_USR_DeviceAddressAssigned 
*         USB device is successfully assigned the Address 
* @param  None
* @retval None
*/
void USBH_USR_DeviceAddressAssigned(void)
{
  
}


/**
* @brief  USBH_USR_Conf_Desc 
*         Displays the message on LCD for configuration descriptor
* @param  ConfDesc : Configuration descriptor
* @retval None
*/
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc)
{
	USBH_InterfaceDesc_TypeDef *id;

	id = itfDesc;  

	if((*id).bInterfaceClass  == 0x08)
	{
//		printf((void *)MSG_MSC_CLASS);
	}
	else if((*id).bInterfaceClass  == 0x03)
	{
//		printf((void *)MSG_HID_CLASS);
	}
}

/**
* @brief  USBH_USR_Manufacturer_String 
*         Displays the message on LCD for Manufacturer String 
* @param  ManufacturerString : Manufacturer String of Device
* @retval None
*/
void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
	printf("  > Manufacturer : %s \r\n", (char *)ManufacturerString);
}

/**
* @brief  USBH_USR_Product_String 
*         Displays the message on LCD for Product String
* @param  ProductString : Product String of Device
* @retval None
*/
void USBH_USR_Product_String(void *ProductString)
{
	printf("  > Product : %s \r\n", (char *)ProductString);
}

/**
* @brief  USBH_USR_SerialNum_String 
*         Displays the message on LCD for SerialNum_String 
* @param  SerialNumString : SerialNum_String of device
* @retval None
*/
void USBH_USR_SerialNum_String(void *SerialNumString)
{
	printf("  > Serial Number : %s \r\n", (char *)SerialNumString);
} 

/**
* @brief  EnumerationDone 
*         User response request is displayed to ask for
*         application jump to class
* @param  None
* @retval None
*/
void USBH_USR_EnumerationDone(void)
{
	/* Enumeration complete */
	printf((void *)MSG_DEV_ENUMERATED);
	printf("------------------------------------\r\n");
//	printf("To start the class operations: \r\n");
//	printf("Press Key...\r\n");
} 

/**
* @brief  USBH_USR_DeviceNotSupported
*         Device is not supported
* @param  None
* @retval None
*/
void USBH_USR_DeviceNotSupported(void)
{
	MyDevice.istarget = 0;	// 不是目标设备
	printf(" > Device not supported. \r\n");
}  


/**
* @brief  USBH_USR_UserInput
*         User Action for application state entry
* @param  None
* @retval USBH_USR_Status : User response for key button
*/
USBH_USR_Status USBH_USR_UserInput(void)
{
	USBH_USR_Status usbh_usr_status;
//	usbh_usr_status = USBH_USR_NO_RESP;  
//	if(Button_GetState() != RESET) 	// Arduino 板子上的唤醒键，按下为高电平
//	{
		usbh_usr_status = USBH_USR_RESP_OK;
//	}
	return usbh_usr_status;
} 


int USBH_USR_Application(void)
{	
	uint16_t i;
	static uint8_t cmd;
	static uint32_t timeoutcnt = TIMEOUTMAX;
	
	if(MyDevice.commstate == COMM_STATE_BUSY)	// 忙
	{
		LEDOn(LED3);
		timeoutcnt--;
		if(timeoutcnt == 0)
		{
			MyDevice.commstate = COMM_STATE_TIMEOUT;
			timeoutcnt = TIMEOUTMAX;
			AppState = USBH_USR_Application_IDLE;
			printf("接收数据超时！\r\n");
			IsUsartCmd = 0;
		}
	}
	
	switch (AppState)
	{
		case USBH_USR_Application_INITSTATE://设备是否配置，发送0x06，返回1已配置
			cmd = USB_CMD06;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);			// 发送
			CDC_StartReception();							// 使能接收
			RxDataLength = 1;								// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(1)\tCMD06\r\n");
			}
			timeoutcnt = TIMEOUTMAX;						// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_RECONFIG:// 发送配置文件
			cmd = USB_CMD04;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			TxConfigDataTimes = 0;	// 发送配置文件的次数清零
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(1)\tCMD04\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
				
		case USBH_USR_Application_READPAR:	// 读取参数, 发0x01，收80字节参数
			cmd = USB_CMD01;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			CDC_StartReception();	// 使能接收
			RxDataLength = 80;		// 接收长度为80字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(1)\tCMD01\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEPARA:// 写参数，发0x00 + 80字节读回的参数，收0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], ParameterData, 80);
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 0;	// 发送参数包的次数：0
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				IsUsartCmd = 0;
				printf("OUT(81)\tCMD00\r\n");
				for(i=0; i<80; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;		
		
		case USBH_USR_Application_READDATA:	// 读取数据，发0x80，收4096字节数据
			cmd = USB_CMD80;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			CDC_StartReception();	// 使能接收
			RxDataLength = 4096;	// 接收长度为4096字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(1)\tCMD80\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEDATA:	// 写数据，发0x01 + 1024字节数据0，收0x01 OK
			cmd = USB_CMD02;
			memset(&TxBuffer[0], 0, 1025);
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1025);
			//TxParaPackTimes = 2;	// 发送参数包的次数：2	
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(1024)\tCMD02\r\n");
				for(i=0; i<1024; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEPAR11:	// 写参数，发0x00 + 80字节参数包11，收0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 1;	// 发送参数包的次数：1
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//CMD00(参数包11) ");
				for(i=0; i<80; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEPAR12:	// 写参数，发0x00 + 80字节参数包12，收0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x00;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 2;	// 发送参数包的次数：2	
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(参数包12) ");
				for(i=0; i<80; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEPAR21:	// 写参数，发0x00 + 80字节参数包21，收0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x01;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 3;	// 发送参数包的次数：3
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(参数包21) ");
				for(i=0; i<80; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEPAR22:	// 写参数，发0x00 + 80字节参数包22，收0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x00;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 4;	// 发送参数包的次数：4
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(参数包22) ");
				for(i=0; i<80; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEPAR31:	// 写参数，发0x00 + 80字节参数包31，收0x01 OK
//			printf("-----------------------------------------------");			
//			printf("开始通信！\r\n");
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x00;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 5;	// 发送参数包的次数：5
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(参数包31) ");
				for(i=0; i<80; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_WRITEPAR32:	// 写参数，发0x00 + 80字节参数包32，收0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x04;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 6;	// 发送参数包的次数：6
			CDC_StartReception();	// 使能接收
			RxDataLength = 1;		// 接收长度为1字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(参数包32) ");
				for(i=0; i<80; i++)
				{
					if((i%8) == 0)
						printf("\t");
					else if((i%4) == 0)
						printf(" ");
					
					printf("%02X ", TxBuffer[i+1]);
					
					if(i>0 && ((i%8) == 7))
						printf("\r\n");
				}
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_READWAVEDATA:	// 读波形，发0x03，收1024字节波形
			cmd = USB_CMD03;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			CDC_StartReception();	// 使能接收
			RxDataLength = 1024;	// 接收长度为1024字节
			AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
			{;
			}
			else
			{
				printf("OUT(1)\tCMD03\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		
		
		case USBH_USR_Application_WAITRECEIVE:	// 等待接收完成
			if(RxDataComplete)	
			{	
				RxDataComplete = 0;
				if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
				{;
				}
				else
				{
					printf("IN(%d)", RxDataLength);
					{
						for(i=0; i<RxDataLength; i++)
						{
							if((i%8) == 0)
								printf("\t");
							else if((i%4) == 0)
								printf(" ");
							
							printf("%02X ", RxBuf[i]);
							
							if(i>0 && ((i%8) == 7))
								printf("\r\n");
						}
						printf("\r\n");
					}
				}
				
				switch(cmd)
				{
					case USB_CMD06://USBH_USR_Application_INITSTATE://设备是否配置，发送0x06，返回1已配置
						if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
						{
							MyDevice.configstate = CONFIG_STATE_CONFIGURED;	// 已经配置
							MyDevice.commstate = COMM_STATE_OK;
							printf("已经配置！\r\n\r\n");
							AppState = USBH_USR_Application_IDLE;
							IsUsartCmd = 0;
						}
						else
						{
							MyDevice.configstate = CONFIG_STATE_NOTCONFIG;		// 未配置
							MyDevice.commstate = COMM_STATE_ERROR;
							printf("未配置！\r\n\r\n");
							AppState = USBH_USR_Application_RECONFIG;	// 进入CMD04，准备配置
							IsUsartCmd = 0;
						}
						break;
						
					case USB_CMD04://USBH_USR_Application_RECONFIG:// 发送配置文件
						if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
						{
							MyDevice.commstate = COMM_STATE_OK;
							if(TxConfigDataTimes < 3)	// 配置文件数据未发送完成
							{
								if(OpenConfigFile()==0)// 打开文件成功
								{
									TxConfigDataTimes++;		// 发送配置文件的次数
									if(ReadConfigFile(1)==0)	// 第一次读文件，实际读取文件字节数为511字节，开头为1字节命令
									{
										AppState = USBH_USR_Application_SENDCONFIG0;// 第一次发送带命令的数据
									}
									else
									{
										MyDevice.commstate = COMM_STATE_ERROR;
										AppState = USBH_USR_Application_IDLE;
									}
								}
							}
							else if(TxConfigDataTimes == 3)	// 配置文件数据发送完成
							{
								printf("配置文件发送完成！\r\n\r\n");
								MyDevice.commstate = COMM_STATE_OK;
								f_close(&file);
								AppState = USBH_USR_Application_INITSTATE;// CMD06，询问是否配置
								IsUsartCmd = 0;
							}
						}
						else
						{
							MyDevice.commstate = COMM_STATE_ERROR;
							printf("配置文件发送失败！\r\n\r\n");
							AppState = USBH_USR_Application_IDLE;
							IsUsartCmd = 0;
						}
						break;
					
					case USB_CMD01://USBH_USR_Application_READPAR:	// 读取参数, 发0x01，收80字节参数
						MyDevice.commstate = COMM_STATE_OK;
						// 保存到全局数组中
						memcpy(ParameterData, RxBuf, RxDataLength);
//						if(IsUsartCmd && IsShowDebuf==0)	// 串口插入命令
//						{		
//							IsUsartCmd = 0;
//							for(i=0; i<RxDataLength; i++)
//							{
//								if((i%8) == 0)
//									printf("\t");
//								else if((i%4) == 0)
//									printf(" ");
//								
//								printf("%02X ", RxBuf[i]);
//								
//								if(i>0 && ((i%8) == 7))
//									printf("\r\n");
//							}
//							printf("读取参数80字节完成！(CMD01)\r\n\r\n");
//						}
//						else
						{
							if(CommunicationSteps == 1)
							{
								CommunicationSteps++;
								ParameterData[76] = 0;	// 读回的参数倒数第4字节替换为0.							
							}
							else if((CommunicationSteps == 4) || (CommunicationSteps == 5))
							{
								CommunicationSteps++;
							}
							
							if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
							{;
							}
							else				
							{
								IsUsartCmd = 0;
								printf("读取参数80字节完成！(CMD01)\r\n\r\n");
							}
						}						
						AppState = USBH_USR_Application_IDLE;	
						break;
					
					case USB_CMD80://USBH_USR_Application_READDATA:	// 读取数据，发0x80，收4096字节数据
						MyDevice.commstate = COMM_STATE_OK;
						memcpy(ReceiveData, RxBuf, RxDataLength);
						AppState = USBH_USR_Application_IDLE;
						if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
						{;
						}
						else
						{
							IsUsartCmd = 0;
							printf("读取数据4096字节完成！(CMD80)\r\n\r\n");
						}
//						if(IsUsartCmd && IsShowDebuf==0)	// 串口插入命令
//						{		
//							IsUsartCmd = 0;
//							for(i=0; i<RxDataLength; i++)
//							{
//								if((i%8) == 0)
//									printf("\t");
//								else if((i%4) == 0)
//									printf(" ");
//								
//								printf("%02X ", RxBuf[i]);
//								
//								if(i>0 && ((i%8) == 7))
//									printf("\r\n");
//							}
//							printf("读取数据4096字节完成！(CMD80)\r\n\r\n");
//						}
						break;
					
					case USB_CMD00://USBH_USR_Application_WRITEPAR:	// 写参数，发0x00 + 80字节参数，收0x01 OK 
						switch(TxParaPackTimes)	// 发送参数包的次数
						{
							case 0:	// 常规发送读回的参数
								if((CommunicationSteps == 2) || (CommunicationSteps==3) || (CommunicationSteps==6))
								{
									if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
									{
										MyDevice.commstate = COMM_STATE_OK;
										CommunicationSteps++;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
										{;
										}
										else
										{
											printf("写参数完成！(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}
									else
									{
										MyDevice.commstate = COMM_STATE_ERROR;
										CommunicationSteps = 0;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
										{;
										}
										else
										{
											printf("写参数失败！(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}	
								}
								else
								{
									if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
									{
										MyDevice.commstate = COMM_STATE_OK;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
										{;
										}
										else
										{
											IsUsartCmd = 0;
											printf("写参数完成！(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}
									else
									{
										MyDevice.commstate = COMM_STATE_ERROR;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
										{;
										}
										else
										{
											IsUsartCmd = 0;
											printf("写参数失败！(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}
								}
								break;
							
							
							case 1:	// 第1次，发送参数包11
								if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数完成！（参数包11）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEPAR12;	// next CMD00发送参数包12
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数失败！（参数包11）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
							
							case 2:	// 第2次，发送参数包12
								if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数完成！（参数包12）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEDATA;	// next 发送CMD02
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数失败！（参数包12）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
							
							case 3:	// 第3次，发送参数包21
								if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数完成！（参数包21）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEPAR22;	// next CMD00发送参数包22
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数失败！（参数包21）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
							
							case 4:	// 第4次，发送参数包22
								if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数完成！（参数包22）(CMD00)\r\n");
										printf("参数设置完成！\r\n\r\n");
									}
									FlowState = FLOW_COMMUNICATION;// 程序流程状态：开始通信
									AppState = USBH_USR_Application_IDLE;//USBH_USR_Application_WRITEPAR31;	// next CMD00发送参数包31
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数失败！（参数包22）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
								
							case 5:	// 第5次，发送参数包31
								if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									CommunicationSteps = 1;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数完成！（参数包31）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEPAR32;	// next CMD00发送参数包32
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									CommunicationSteps = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数失败！（参数包31）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
								
							case 6:	// 第6次，发送参数包32
								if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									CommunicationSteps = 1;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数完成！（参数包32）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;//USBH_USR_Application_READPAR;	// next CMD01读参数
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									CommunicationSteps = 0;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// 回显关闭，并且在循环流程，不打印
									{;
									}
									else
									{
										printf("写参数失败！（参数包32）(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
								
						}						
						break;
						
					case USB_CMD02://USBH_USR_Application_WRITEDATA:	// 写数据，发0x01 + 1024字节数据0，收0x01 OK
						if(RxBuf[RxDataLength-1] == 0x01)// 接收0x01
						{
							MyDevice.commstate = COMM_STATE_OK;
							if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
							{;
							}
							else
							{
								IsUsartCmd = 0;
								printf("写数据完成！(CMD02)\r\n\r\n");
							}
							AppState = USBH_USR_Application_WRITEPAR21;	// next CMD00发送参数包21
						}
						else
						{
							MyDevice.commstate = COMM_STATE_ERROR;
							if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
							{;
							}
							else
							{
								IsUsartCmd = 0;
								printf("写数据失败！(CMD02)\r\n\r\n");
							}
							AppState = USBH_USR_Application_IDLE;
						}	
						break;
					
					case USB_CMD03://USBH_USR_Application_READWAVEDATA:	// 读波形，发0x03，收1024字节波形
						MyDevice.commstate = COMM_STATE_OK;
						memcpy(ReceiveWaveData, RxBuf, RxDataLength);
//						if(IsUsartCmd && IsShowDebuf==0)
//						{
//							IsUsartCmd = 0;
//							for(i=0; i<RxDataLength; i++)
//							{
//								if((i%8) == 0)
//									printf("\t");
//								else if((i%4) == 0)
//									printf(" ");
//								
//								printf("%02X ", RxBuf[i]);
//								
//								if(i>0 && ((i%8) == 7))
//									printf("\r\n");
//							}
//							printf("读取波形1024字节完成！(CMD80)\r\n\r\n");
//						}
						if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// 回显关闭，并且在循环流程，不打印
						{;
						}
						else
						{
							IsUsartCmd = 0;
							printf("读取波形1024字节完成！(CMD03)\r\n\r\n");
						}
						AppState = USBH_USR_Application_IDLE;
						break;

				}
				LEDOff(LED3);
			}
			break;
			
			
		case USBH_USR_Application_SENDCONFIG0:// 发送命令+配置数据，共512字节			
			CDC_SendData(TxBuffer, 512);
			AppState = USBH_USR_Application_WAITING;
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;
		
		case USBH_USR_Application_SENDCONFIG1:// 发送配置数据，共512字节	
			CDC_SendData(TxBuffer, TxDataLength);
			AppState = USBH_USR_Application_WAITING;
			timeoutcnt = TIMEOUTMAX;	// 设置超时时间
			MyDevice.commstate = COMM_STATE_BUSY;			// 设置通信状态
			break;

		case USBH_USR_Application_WAITING:// 等待配置数据发送完成
			if(CDC_TxParam.CDCState == CDC_IDLE)	// 发送完成
			{
				if(TxDataLength < 512)		// 最后一次数据发送完成，等待接收应答
				{
					CDC_StartReception();	// 使能接收
					RxDataLength = 1;		// 接收长度为1字节
					AppState = USBH_USR_Application_WAITRECEIVE;	// 等待接收完成
				}
				else
				{
					if(TxConfigDataTimes < 3) 	// 发送配置文件的次数，前2次
					{
						if(ReadConfigFile(2)==0)	// 再次读取文件数据
							AppState = USBH_USR_Application_SENDCONFIG1;
						else
							AppState = USBH_USR_Application_IDLE;
					}
					else	// 第3次
					{
						if(ReadConfigFile(3)==0)	// 再次读取文件数据
							AppState = USBH_USR_Application_SENDCONFIG1;
						else
							AppState = USBH_USR_Application_IDLE;					
					}
				}
			}

			break;
		default:
			break;

	}
	return 0;
}

FRESULT OpenConfigFile(void)
{
	
	FRESULT result;
	const TCHAR* pathname = "cfg.rcc";
	UINT readnum;
	uint16_t i,j;
	uint32_t filelen = 0;
	
	result = f_open(&file, (const TCHAR*)pathname, FA_OPEN_EXISTING | FA_READ);
	
	if(result) 
	{
		printf("打开文件错误！(%d)\r\n", result);
	}
	else
	{
//		printf(">>读取配置文件：\r\n");
//		ReadConfigFile();
	}
//	f_close(&file);
	return result;
}

FRESULT ReadConfigFile(uint8_t times)
{
	
	FRESULT result;
	UINT readnum;
	uint16_t i,j;
	static uint32_t filelen = 0;
	static uint32_t filepointer = 0;
	
	if(times == 1)// 一共发送 61424字节数据
	{
		
		f_lseek(&file, filepointer);	
		
		result = f_read(&file, &TxBuffer[1], 511, &readnum);
		if(result)
		{
			printf("读取文件错误！(%d)返回\r\n", result);
			return result;
		}
		
		filelen += readnum;
		TxDataLength = 512;//readnum;
		if(readnum == 511)
		{
			TxBuffer[0] = USB_CMD05;
			filepointer += 511;
			result = 0;
		}
		else
		{
			printf("数据长度读取错误\r\n");
			return 1;
		}
	}
	else if(times == 2)// 数据长度为61424字节
	{
		f_lseek(&file, filepointer);
		
		result = f_read(&file, &TxBuffer[0], 512, &readnum);
		if(result)
		{
			printf("读取文件错误！返回\r\n");
			return result;
		}
		
		filelen += readnum;// 总读取文件长度
		if((filelen >= 61424))
		{
			TxDataLength = 497;
			filepointer += 497;
			//filelen = filelen - readnum + 424;	// 修正总已读出数据长度
			filelen = 0;
		}	
		else
		{
			TxDataLength = readnum;
			filepointer += readnum;
		}
		result = 0;
	}
	else if(times == 3)// 数据长度为44117字节
	{
		f_lseek(&file, filepointer);
		
		result = f_read(&file, &TxBuffer[0], 512, &readnum);
		if(result)
		{
			printf("读取文件错误！返回\r\n");
			return result;
		}
		
		filelen += readnum;// 总读取文件长度
		if((filelen >= 44117))
		{
			TxDataLength = 86;
			filepointer = 0;
			filelen = 0;
		}		
		else
		{
			TxDataLength = readnum;
			filepointer += readnum;
		}
		result = 0;
	}
//	printf("\r\n文件字节数：%d\r\n", filelen);
	
	return result;
}
/**
* @brief  USBH_USR_OverCurrentDetected
*         Device Overcurrent detection event
* @param  None
* @retval None
*/
void USBH_USR_OverCurrentDetected (void)
{
	printf(" > Overcurrent detected. \r\n");
}

/**
* @brief  USBH_USR_DeInit
*         Deint User state and associated variables
* @param  None
* @retval None
*/
void USBH_USR_DeInit(void)
{
	// 设备状态相关结构体变量初始化
	MyDeviceType_Init();
}

/**********************************END OF FILE**********************************/
