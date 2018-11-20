/**
  ******************************************************************************
  * @file    usbh_custom_usr.h
  * @author  yaoohui
  * @version V1.0
  * @date    2016年2月20日
  * @brief   
  ******************************************************************************
*/ 

#ifndef __USH_USR_H
#define __USH_USR_H

#include "usbh_core.h"
#include "usb_conf.h"
#include <stdio.h>
#include "gpio.h"

// 程序流程状态
typedef enum
{
	FLOW_IDLE = 0,
	FLOW_SET_PARA,		// 参数设置阶段
	FLOW_COMMUNICATION,	// 通信阶段
	FLOW_LOOP,			// 循环阶段
}FLOW_STATE;

// USB通信命令
typedef enum
{
	USB_CMD00 = 0,		// 写参数，数据长度80字节，返回1 或 0
	USB_CMD01,			// 读参数，返回80字节参数
	USB_CMD02,			// 写数据，数据长度1024字节，返回1 或 0
	USB_CMD03,			// 读波形，返回1024字节波形
	USB_CMD04,			// 进入配置，返回1 或 0
	USB_CMD05,			// 写配置，数据长度参考监听，返回1 或 0
	USB_CMD06,			// 询问配置，返回1 或 0
	USB_CMD80 = 0x80,	// 读数据，返回4096字节数据
}
USB_DEVICE_CMD;

// 应用程序状态机
typedef enum {
	USBH_USR_Application_IDLE = 0,
	USBH_USR_Application_INITSTATE,		// CMD06 询问配置，返回1或0
	USBH_USR_Application_RECONFIG,		// CMD04 进入配置，返回1或0
	USBH_USR_Application_READPAR,		// DMC01 读参数，返回80字节
	USBH_USR_Application_READDATA,		// CMD80 读数据，返回4096字节
	USBH_USR_Application_WRITEDATA,		// CMD02 写数据，写入1024字节
	USBH_USR_Application_READWAVEDATA,	// CMD03 读波形数据，返回1024字节
	USBH_USR_Application_WRITEPARA,		// CMD00 写参数，写入最近读回的参数，80字节
	USBH_USR_Application_WRITEPAR11,	// 写参数，参数包11
	USBH_USR_Application_WRITEPAR12,	// 写参数，参数包12
	USBH_USR_Application_WRITEPAR21,	// 写参数，参数包21
	USBH_USR_Application_WRITEPAR22,	// 写参数，参数包22
	USBH_USR_Application_WRITEPAR23,	// 写参数，参数包23(偶尔没有此包，可跳过)
	USBH_USR_Application_WRITEPAR31,	// 写参数，参数包31
	USBH_USR_Application_WRITEPAR32,	// 写参数，参数包32
	USBH_USR_Application_WRITEPARA_CHANGE,// 写参数，参数倒数第4字节替换为0x00
	USBH_USR_Application_WAITRECEIVE,	// 等待接收完成
	USBH_USR_Application_SENDCONFIG0,	// 发送配置文件
	USBH_USR_Application_WAITING,
	USBH_USR_Application_SENDCONFIG1,
}USBH_USR_Application_State;

// 设备状态相关结构体
typedef struct _Device{
	uint8_t		isconnected;	// 是否接入USB设备，0-未连接，1-连接
	uint8_t		istarget;		// 是否为目标设备，0-不是目标设备，1-目标设备
	uint8_t 	configstate;	// 设备配置状态，见USB_DEVICE_CONFIG_STATE
	uint8_t		commstate;		// 设备通信状态，0-通信良好，1-接收OK，2-接收错误，3-超时
}USB_Device_TypeDef;

// 设备配置状态
typedef enum{
	CONFIG_STATE_NOTCONFIG = 0,		// 设备未配置
	CONFIG_STATE_CONFIGURED,		// 设备已配置
	CONFIG_STATE_ERROR,				// 错误
	CONFIG_STATE_TIMEOUT,			// 超时
}USB_DEVICE_CONFIG_STATE;

// 设备通信状态
typedef enum{
	COMM_STATE_IDLE = 0,		// 空闲
	COMM_STATE_BUSY,			// 忙，正在接收数据
	COMM_STATE_OK,				// 接收数据OK
	COMM_STATE_ERROR,			// 接收数据失败
	COMM_STATE_TIMEOUT,			// 超时，未接收到数据
}USB_DEVICE_COMM_STATE;

extern  USBH_Usr_cb_TypeDef USR_cb;
extern __IO USB_Device_TypeDef 	MyDevice;	// 设备状态结构体

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
