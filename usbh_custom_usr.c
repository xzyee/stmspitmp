/**
  ******************************************************************************
  * @file    usbh_custom_usr.c
  * @author  yaoohui
  * @version V1.0
  * @date    2016��2��20��
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

uint16_t RxDataLength = 0;	// ��Ҫ�����豸�����ݳ���
uint8_t RxDataComplete = 0;	// ����������ɱ�־, 1-�������
uint16_t TxDataLength = 0;	// ��Ҫ���͵����ݳ���
uint8_t TxConfigDataTimes = 0;	// ���ʹ�����������ļ��Ĵ���
uint8_t TxParaPackTimes = 0;	// ���Ͳ������Ĵ���
uint8_t CommunicationSteps = 0;	// ��ʼͨ��ʱ�ķ�������Ĵ���
uint8_t FlowState = 0;			// ����״̬

__IO USB_Device_TypeDef 	MyDevice;	// �豸״̬�ṹ��

extern USBH_TypeDef USBH_CUSTOM_DEVICE;

uint8_t TxBuffer[1025];
uint8_t ParameterData[80];		// ������ 0x01��80�ֽ�
uint8_t ReceiveData[4096];		// ������ 0x80��4096�ֽ�
uint8_t ReceiveWaveData[1024];	// ������ 0x03,1024�ֽ�
const uint8_t PARA_PACK_11[80] = // ������11����80�ֽڣ�����[0]��[76]�ֽڿɱ�
{
0x81, 0x00, 0x0c, 0x03, 0x00, 0x30, 0x20, 0x03, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x01, 0x80, 0x02, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10, 0xc0, 0x0b, 0x00
};

extern USB_OTG_CORE_HANDLE           USB_OTG_Core;
extern __ALIGN_BEGIN uint8_t		RxBuf [4100] __ALIGN_END ;	// ���ջ�����

extern FATFS fs;   /* ��Ҫ���ó�ȫ�ֱ�������������systick��Ӧ�ٶۻ��޷����ļ� Work area (file system object) for logical drive */
extern FIL file;

extern CDC_Xfer_TypeDef		CDC_TxParam;
extern uint8_t IsUsartCmd;	// �Ƿ�Ϊ�������1-��
extern uint8_t IsShowDebuf;	// �Ƿ���ʾ��Ϣ��1-��ʾ

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


// �豸״̬��ؽṹ�������ʼ��
void MyDeviceType_Init(void)
{
	MyDevice.configstate = CONFIG_STATE_NOTCONFIG;	// δ����
	MyDevice.isconnected = 0;						// δ����
	MyDevice.istarget = 0;							// ����Ŀ���豸
	MyDevice.commstate = COMM_STATE_IDLE;			// ����	
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

		// �豸״̬��ؽṹ�������ʼ��
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
	MyDevice.isconnected = 1;	// USB�豸������
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
	MyDeviceType_Init();		// �豸״̬��ؽṹ�������ʼ��
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
	MyDevice.istarget = 0;	// ����Ŀ���豸
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
//	if(Button_GetState() != RESET) 	// Arduino �����ϵĻ��Ѽ�������Ϊ�ߵ�ƽ
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
	
	if(MyDevice.commstate == COMM_STATE_BUSY)	// æ
	{
		LEDOn(LED3);
		timeoutcnt--;
		if(timeoutcnt == 0)
		{
			MyDevice.commstate = COMM_STATE_TIMEOUT;
			timeoutcnt = TIMEOUTMAX;
			AppState = USBH_USR_Application_IDLE;
			printf("�������ݳ�ʱ��\r\n");
			IsUsartCmd = 0;
		}
	}
	
	switch (AppState)
	{
		case USBH_USR_Application_INITSTATE://�豸�Ƿ����ã�����0x06������1������
			cmd = USB_CMD06;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);			// ����
			CDC_StartReception();							// ʹ�ܽ���
			RxDataLength = 1;								// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(1)\tCMD06\r\n");
			}
			timeoutcnt = TIMEOUTMAX;						// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_RECONFIG:// ���������ļ�
			cmd = USB_CMD04;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			TxConfigDataTimes = 0;	// ���������ļ��Ĵ�������
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(1)\tCMD04\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
				
		case USBH_USR_Application_READPAR:	// ��ȡ����, ��0x01����80�ֽڲ���
			cmd = USB_CMD01;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 80;		// ���ճ���Ϊ80�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(1)\tCMD01\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEPARA:// д��������0x00 + 80�ֽڶ��صĲ�������0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], ParameterData, 80);
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 0;	// ���Ͳ������Ĵ�����0
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;		
		
		case USBH_USR_Application_READDATA:	// ��ȡ���ݣ���0x80����4096�ֽ�����
			cmd = USB_CMD80;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 4096;	// ���ճ���Ϊ4096�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(1)\tCMD80\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEDATA:	// д���ݣ���0x01 + 1024�ֽ�����0����0x01 OK
			cmd = USB_CMD02;
			memset(&TxBuffer[0], 0, 1025);
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1025);
			//TxParaPackTimes = 2;	// ���Ͳ������Ĵ�����2	
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEPAR11:	// д��������0x00 + 80�ֽڲ�����11����0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 1;	// ���Ͳ������Ĵ�����1
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//CMD00(������11) ");
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEPAR12:	// д��������0x00 + 80�ֽڲ�����12����0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x00;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 2;	// ���Ͳ������Ĵ�����2	
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(������12) ");
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEPAR21:	// д��������0x00 + 80�ֽڲ�����21����0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x01;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 3;	// ���Ͳ������Ĵ�����3
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(������21) ");
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEPAR22:	// д��������0x00 + 80�ֽڲ�����22����0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x00;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 4;	// ���Ͳ������Ĵ�����4
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(������22) ");
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEPAR31:	// д��������0x00 + 80�ֽڲ�����31����0x01 OK
//			printf("-----------------------------------------------");			
//			printf("��ʼͨ�ţ�\r\n");
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x00;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 5;	// ���Ͳ������Ĵ�����5
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(������31) ");
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_WRITEPAR32:	// д��������0x00 + 80�ֽڲ�����32����0x01 OK 
			cmd = USB_CMD00;
			TxBuffer[0] = cmd;
			memcpy(&TxBuffer[1], PARA_PACK_11, 80);
			TxBuffer[1] = 0x01;
			TxBuffer[77] = 0x04;
			CDC_SendData((uint8_t *)TxBuffer, 81);
			TxParaPackTimes = 6;	// ���Ͳ������Ĵ�����6
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(81)\tCMD00\r\n");//("CMD00(������32) ");
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
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_READWAVEDATA:	// �����Σ���0x03����1024�ֽڲ���
			cmd = USB_CMD03;
			TxBuffer[0] = cmd;
			CDC_SendData((uint8_t *)TxBuffer, 1);
			CDC_StartReception();	// ʹ�ܽ���
			RxDataLength = 1024;	// ���ճ���Ϊ1024�ֽ�
			AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
			if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
			{;
			}
			else
			{
				printf("OUT(1)\tCMD03\r\n");
			}
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		
		
		case USBH_USR_Application_WAITRECEIVE:	// �ȴ��������
			if(RxDataComplete)	
			{	
				RxDataComplete = 0;
				if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
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
					case USB_CMD06://USBH_USR_Application_INITSTATE://�豸�Ƿ����ã�����0x06������1������
						if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
						{
							MyDevice.configstate = CONFIG_STATE_CONFIGURED;	// �Ѿ�����
							MyDevice.commstate = COMM_STATE_OK;
							printf("�Ѿ����ã�\r\n\r\n");
							AppState = USBH_USR_Application_IDLE;
							IsUsartCmd = 0;
						}
						else
						{
							MyDevice.configstate = CONFIG_STATE_NOTCONFIG;		// δ����
							MyDevice.commstate = COMM_STATE_ERROR;
							printf("δ���ã�\r\n\r\n");
							AppState = USBH_USR_Application_RECONFIG;	// ����CMD04��׼������
							IsUsartCmd = 0;
						}
						break;
						
					case USB_CMD04://USBH_USR_Application_RECONFIG:// ���������ļ�
						if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
						{
							MyDevice.commstate = COMM_STATE_OK;
							if(TxConfigDataTimes < 3)	// �����ļ�����δ�������
							{
								if(OpenConfigFile()==0)// ���ļ��ɹ�
								{
									TxConfigDataTimes++;		// ���������ļ��Ĵ���
									if(ReadConfigFile(1)==0)	// ��һ�ζ��ļ���ʵ�ʶ�ȡ�ļ��ֽ���Ϊ511�ֽڣ���ͷΪ1�ֽ�����
									{
										AppState = USBH_USR_Application_SENDCONFIG0;// ��һ�η��ʹ����������
									}
									else
									{
										MyDevice.commstate = COMM_STATE_ERROR;
										AppState = USBH_USR_Application_IDLE;
									}
								}
							}
							else if(TxConfigDataTimes == 3)	// �����ļ����ݷ������
							{
								printf("�����ļ�������ɣ�\r\n\r\n");
								MyDevice.commstate = COMM_STATE_OK;
								f_close(&file);
								AppState = USBH_USR_Application_INITSTATE;// CMD06��ѯ���Ƿ�����
								IsUsartCmd = 0;
							}
						}
						else
						{
							MyDevice.commstate = COMM_STATE_ERROR;
							printf("�����ļ�����ʧ�ܣ�\r\n\r\n");
							AppState = USBH_USR_Application_IDLE;
							IsUsartCmd = 0;
						}
						break;
					
					case USB_CMD01://USBH_USR_Application_READPAR:	// ��ȡ����, ��0x01����80�ֽڲ���
						MyDevice.commstate = COMM_STATE_OK;
						// ���浽ȫ��������
						memcpy(ParameterData, RxBuf, RxDataLength);
//						if(IsUsartCmd && IsShowDebuf==0)	// ���ڲ�������
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
//							printf("��ȡ����80�ֽ���ɣ�(CMD01)\r\n\r\n");
//						}
//						else
						{
							if(CommunicationSteps == 1)
							{
								CommunicationSteps++;
								ParameterData[76] = 0;	// ���صĲ���������4�ֽ��滻Ϊ0.							
							}
							else if((CommunicationSteps == 4) || (CommunicationSteps == 5))
							{
								CommunicationSteps++;
							}
							
							if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
							{;
							}
							else				
							{
								IsUsartCmd = 0;
								printf("��ȡ����80�ֽ���ɣ�(CMD01)\r\n\r\n");
							}
						}						
						AppState = USBH_USR_Application_IDLE;	
						break;
					
					case USB_CMD80://USBH_USR_Application_READDATA:	// ��ȡ���ݣ���0x80����4096�ֽ�����
						MyDevice.commstate = COMM_STATE_OK;
						memcpy(ReceiveData, RxBuf, RxDataLength);
						AppState = USBH_USR_Application_IDLE;
						if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
						{;
						}
						else
						{
							IsUsartCmd = 0;
							printf("��ȡ����4096�ֽ���ɣ�(CMD80)\r\n\r\n");
						}
//						if(IsUsartCmd && IsShowDebuf==0)	// ���ڲ�������
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
//							printf("��ȡ����4096�ֽ���ɣ�(CMD80)\r\n\r\n");
//						}
						break;
					
					case USB_CMD00://USBH_USR_Application_WRITEPAR:	// д��������0x00 + 80�ֽڲ�������0x01 OK 
						switch(TxParaPackTimes)	// ���Ͳ������Ĵ���
						{
							case 0:	// ���淢�Ͷ��صĲ���
								if((CommunicationSteps == 2) || (CommunicationSteps==3) || (CommunicationSteps==6))
								{
									if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
									{
										MyDevice.commstate = COMM_STATE_OK;
										CommunicationSteps++;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
										{;
										}
										else
										{
											printf("д������ɣ�(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}
									else
									{
										MyDevice.commstate = COMM_STATE_ERROR;
										CommunicationSteps = 0;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
										{;
										}
										else
										{
											printf("д����ʧ�ܣ�(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}	
								}
								else
								{
									if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
									{
										MyDevice.commstate = COMM_STATE_OK;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
										{;
										}
										else
										{
											IsUsartCmd = 0;
											printf("д������ɣ�(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}
									else
									{
										MyDevice.commstate = COMM_STATE_ERROR;
										TxParaPackTimes = 0;
										if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
										{;
										}
										else
										{
											IsUsartCmd = 0;
											printf("д����ʧ�ܣ�(CMD00)\r\n\r\n");
										}
										AppState = USBH_USR_Application_IDLE;
									}
								}
								break;
							
							
							case 1:	// ��1�Σ����Ͳ�����11
								if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д������ɣ���������11��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEPAR12;	// next CMD00���Ͳ�����12
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д����ʧ�ܣ���������11��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
							
							case 2:	// ��2�Σ����Ͳ�����12
								if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д������ɣ���������12��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEDATA;	// next ����CMD02
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д����ʧ�ܣ���������12��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
							
							case 3:	// ��3�Σ����Ͳ�����21
								if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д������ɣ���������21��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEPAR22;	// next CMD00���Ͳ�����22
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д����ʧ�ܣ���������21��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
							
							case 4:	// ��4�Σ����Ͳ�����22
								if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д������ɣ���������22��(CMD00)\r\n");
										printf("����������ɣ�\r\n\r\n");
									}
									FlowState = FLOW_COMMUNICATION;// ��������״̬����ʼͨ��
									AppState = USBH_USR_Application_IDLE;//USBH_USR_Application_WRITEPAR31;	// next CMD00���Ͳ�����31
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д����ʧ�ܣ���������22��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
								
							case 5:	// ��5�Σ����Ͳ�����31
								if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									CommunicationSteps = 1;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д������ɣ���������31��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_WRITEPAR32;	// next CMD00���Ͳ�����32
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									TxParaPackTimes = 0;
									CommunicationSteps = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д����ʧ�ܣ���������31��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
								
							case 6:	// ��6�Σ����Ͳ�����32
								if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
								{
									MyDevice.commstate = COMM_STATE_OK;
									CommunicationSteps = 1;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д������ɣ���������32��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;//USBH_USR_Application_READPAR;	// next CMD01������
								}
								else
								{
									MyDevice.commstate = COMM_STATE_ERROR;
									CommunicationSteps = 0;
									TxParaPackTimes = 0;
									if((FlowState == FLOW_LOOP) && (IsShowDebuf==0))// ���Թرգ�������ѭ�����̣�����ӡ
									{;
									}
									else
									{
										printf("д����ʧ�ܣ���������32��(CMD00)\r\n\r\n");
									}
									AppState = USBH_USR_Application_IDLE;
								}	
								break;
								
						}						
						break;
						
					case USB_CMD02://USBH_USR_Application_WRITEDATA:	// д���ݣ���0x01 + 1024�ֽ�����0����0x01 OK
						if(RxBuf[RxDataLength-1] == 0x01)// ����0x01
						{
							MyDevice.commstate = COMM_STATE_OK;
							if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
							{;
							}
							else
							{
								IsUsartCmd = 0;
								printf("д������ɣ�(CMD02)\r\n\r\n");
							}
							AppState = USBH_USR_Application_WRITEPAR21;	// next CMD00���Ͳ�����21
						}
						else
						{
							MyDevice.commstate = COMM_STATE_ERROR;
							if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
							{;
							}
							else
							{
								IsUsartCmd = 0;
								printf("д����ʧ�ܣ�(CMD02)\r\n\r\n");
							}
							AppState = USBH_USR_Application_IDLE;
						}	
						break;
					
					case USB_CMD03://USBH_USR_Application_READWAVEDATA:	// �����Σ���0x03����1024�ֽڲ���
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
//							printf("��ȡ����1024�ֽ���ɣ�(CMD80)\r\n\r\n");
//						}
						if((FlowState == FLOW_LOOP) && (IsShowDebuf==0) && (IsUsartCmd==0))// ���Թرգ�������ѭ�����̣�����ӡ
						{;
						}
						else
						{
							IsUsartCmd = 0;
							printf("��ȡ����1024�ֽ���ɣ�(CMD03)\r\n\r\n");
						}
						AppState = USBH_USR_Application_IDLE;
						break;

				}
				LEDOff(LED3);
			}
			break;
			
			
		case USBH_USR_Application_SENDCONFIG0:// ��������+�������ݣ���512�ֽ�			
			CDC_SendData(TxBuffer, 512);
			AppState = USBH_USR_Application_WAITING;
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;
		
		case USBH_USR_Application_SENDCONFIG1:// �����������ݣ���512�ֽ�	
			CDC_SendData(TxBuffer, TxDataLength);
			AppState = USBH_USR_Application_WAITING;
			timeoutcnt = TIMEOUTMAX;	// ���ó�ʱʱ��
			MyDevice.commstate = COMM_STATE_BUSY;			// ����ͨ��״̬
			break;

		case USBH_USR_Application_WAITING:// �ȴ��������ݷ������
			if(CDC_TxParam.CDCState == CDC_IDLE)	// �������
			{
				if(TxDataLength < 512)		// ���һ�����ݷ�����ɣ��ȴ�����Ӧ��
				{
					CDC_StartReception();	// ʹ�ܽ���
					RxDataLength = 1;		// ���ճ���Ϊ1�ֽ�
					AppState = USBH_USR_Application_WAITRECEIVE;	// �ȴ��������
				}
				else
				{
					if(TxConfigDataTimes < 3) 	// ���������ļ��Ĵ�����ǰ2��
					{
						if(ReadConfigFile(2)==0)	// �ٴζ�ȡ�ļ�����
							AppState = USBH_USR_Application_SENDCONFIG1;
						else
							AppState = USBH_USR_Application_IDLE;
					}
					else	// ��3��
					{
						if(ReadConfigFile(3)==0)	// �ٴζ�ȡ�ļ�����
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
		printf("���ļ�����(%d)\r\n", result);
	}
	else
	{
//		printf(">>��ȡ�����ļ���\r\n");
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
	
	if(times == 1)// һ������ 61424�ֽ�����
	{
		
		f_lseek(&file, filepointer);	
		
		result = f_read(&file, &TxBuffer[1], 511, &readnum);
		if(result)
		{
			printf("��ȡ�ļ�����(%d)����\r\n", result);
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
			printf("���ݳ��ȶ�ȡ����\r\n");
			return 1;
		}
	}
	else if(times == 2)// ���ݳ���Ϊ61424�ֽ�
	{
		f_lseek(&file, filepointer);
		
		result = f_read(&file, &TxBuffer[0], 512, &readnum);
		if(result)
		{
			printf("��ȡ�ļ����󣡷���\r\n");
			return result;
		}
		
		filelen += readnum;// �ܶ�ȡ�ļ�����
		if((filelen >= 61424))
		{
			TxDataLength = 497;
			filepointer += 497;
			//filelen = filelen - readnum + 424;	// �������Ѷ������ݳ���
			filelen = 0;
		}	
		else
		{
			TxDataLength = readnum;
			filepointer += readnum;
		}
		result = 0;
	}
	else if(times == 3)// ���ݳ���Ϊ44117�ֽ�
	{
		f_lseek(&file, filepointer);
		
		result = f_read(&file, &TxBuffer[0], 512, &readnum);
		if(result)
		{
			printf("��ȡ�ļ����󣡷���\r\n");
			return result;
		}
		
		filelen += readnum;// �ܶ�ȡ�ļ�����
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
//	printf("\r\n�ļ��ֽ�����%d\r\n", filelen);
	
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
	// �豸״̬��ؽṹ�������ʼ��
	MyDeviceType_Init();
}

/**********************************END OF FILE**********************************/
