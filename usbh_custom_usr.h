/**
  ******************************************************************************
  * @file    usbh_custom_usr.h
  * @author  yaoohui
  * @version V1.0
  * @date    2016��2��20��
  * @brief   
  ******************************************************************************
*/ 

#ifndef __USH_USR_H
#define __USH_USR_H

#include "usbh_core.h"
#include "usb_conf.h"
#include <stdio.h>
#include "gpio.h"

// ��������״̬
typedef enum
{
	FLOW_IDLE = 0,
	FLOW_SET_PARA,		// �������ý׶�
	FLOW_COMMUNICATION,	// ͨ�Ž׶�
	FLOW_LOOP,			// ѭ���׶�
}FLOW_STATE;

// USBͨ������
typedef enum
{
	USB_CMD00 = 0,		// д���������ݳ���80�ֽڣ�����1 �� 0
	USB_CMD01,			// ������������80�ֽڲ���
	USB_CMD02,			// д���ݣ����ݳ���1024�ֽڣ�����1 �� 0
	USB_CMD03,			// �����Σ�����1024�ֽڲ���
	USB_CMD04,			// �������ã�����1 �� 0
	USB_CMD05,			// д���ã����ݳ��Ȳο�����������1 �� 0
	USB_CMD06,			// ѯ�����ã�����1 �� 0
	USB_CMD80 = 0x80,	// �����ݣ�����4096�ֽ�����
}
USB_DEVICE_CMD;

// Ӧ�ó���״̬��
typedef enum {
	USBH_USR_Application_IDLE = 0,
	USBH_USR_Application_INITSTATE,		// CMD06 ѯ�����ã�����1��0
	USBH_USR_Application_RECONFIG,		// CMD04 �������ã�����1��0
	USBH_USR_Application_READPAR,		// DMC01 ������������80�ֽ�
	USBH_USR_Application_READDATA,		// CMD80 �����ݣ�����4096�ֽ�
	USBH_USR_Application_WRITEDATA,		// CMD02 д���ݣ�д��1024�ֽ�
	USBH_USR_Application_READWAVEDATA,	// CMD03 ���������ݣ�����1024�ֽ�
	USBH_USR_Application_WRITEPARA,		// CMD00 д������д��������صĲ�����80�ֽ�
	USBH_USR_Application_WRITEPAR11,	// д������������11
	USBH_USR_Application_WRITEPAR12,	// д������������12
	USBH_USR_Application_WRITEPAR21,	// д������������21
	USBH_USR_Application_WRITEPAR22,	// д������������22
	USBH_USR_Application_WRITEPAR23,	// д������������23(ż��û�д˰���������)
	USBH_USR_Application_WRITEPAR31,	// д������������31
	USBH_USR_Application_WRITEPAR32,	// д������������32
	USBH_USR_Application_WRITEPARA_CHANGE,// д����������������4�ֽ��滻Ϊ0x00
	USBH_USR_Application_WAITRECEIVE,	// �ȴ��������
	USBH_USR_Application_SENDCONFIG0,	// ���������ļ�
	USBH_USR_Application_WAITING,
	USBH_USR_Application_SENDCONFIG1,
}USBH_USR_Application_State;

// �豸״̬��ؽṹ��
typedef struct _Device{
	uint8_t		isconnected;	// �Ƿ����USB�豸��0-δ���ӣ�1-����
	uint8_t		istarget;		// �Ƿ�ΪĿ���豸��0-����Ŀ���豸��1-Ŀ���豸
	uint8_t 	configstate;	// �豸����״̬����USB_DEVICE_CONFIG_STATE
	uint8_t		commstate;		// �豸ͨ��״̬��0-ͨ�����ã�1-����OK��2-���մ���3-��ʱ
}USB_Device_TypeDef;

// �豸����״̬
typedef enum{
	CONFIG_STATE_NOTCONFIG = 0,		// �豸δ����
	CONFIG_STATE_CONFIGURED,		// �豸������
	CONFIG_STATE_ERROR,				// ����
	CONFIG_STATE_TIMEOUT,			// ��ʱ
}USB_DEVICE_CONFIG_STATE;

// �豸ͨ��״̬
typedef enum{
	COMM_STATE_IDLE = 0,		// ����
	COMM_STATE_BUSY,			// æ�����ڽ�������
	COMM_STATE_OK,				// ��������OK
	COMM_STATE_ERROR,			// ��������ʧ��
	COMM_STATE_TIMEOUT,			// ��ʱ��δ���յ�����
}USB_DEVICE_COMM_STATE;

extern  USBH_Usr_cb_TypeDef USR_cb;
extern __IO USB_Device_TypeDef 	MyDevice;	// �豸״̬�ṹ��

//-----------------------------------------
void USBH_USR_Init(void);
void USBH_USR_DeInit(void);
void USBH_USR_DeviceAttached(void);
void USBH_USR_ResetDevice(void);
void USBH_USR_DeviceDisconnected (void);
void USBH_USR_OverCurrentDetected (void);
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed); 
void USBH_USR_Device_DescAvailable(void *);
void USBH_USR_DeviceAddressAssigned(void);
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc);
void USBH_USR_Manufacturer_String(void *);
void USBH_USR_Product_String(void *);
void USBH_USR_SerialNum_String(void *);
void USBH_USR_EnumerationDone(void);
USBH_USR_Status USBH_USR_UserInput(void);
int USBH_USR_Application(void);
void USBH_USR_DeviceNotSupported(void);
void USBH_USR_UnrecoveredError(void);
//-----------------------------------------
void MyDeviceType_Init(void);


#endif

/**********************************END OF FILE**********************************/
