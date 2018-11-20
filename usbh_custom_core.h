/**
  ******************************************************************************
  * @file    usbh_custom_core.h
  * @author  yaoohui
  * @version V1.0
  * @date    2016年2月20日
  * @brief   
  ******************************************************************************
*/ 


/* Define to prevent recursive  -----------------------------------------------------------------*/
#ifndef __USBH_CUSTOM_H
#define __USBH_CUSTOM_H

/* Includes -------------------------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "usbh_core.h"
#include "usbh_stdreq.h"
#include "usb_bsp.h"
#include "usbh_ioreq.h"
#include "usbh_hcs.h"

/* ----------------------------------------------------------------------------------------------*/
#define CUSTOM_DEVICE_SET         0x41
#define CUSTOM_DEVICE_GET         0xC1

/* ----------------------------------------------------------------------------------------------*/
#define IFC_ENABLE         0x00 //Enable or disable the interface
#define SET_BAUDDIV        0x01 //Set the baud rate divisor
#define GET_BAUDDIV        0x02 //Get the baud rate divisor
#define SET_LINE_CTL       0x03 //Set the line control
#define GET_LINE_CTL       0x04 //Get the line control
#define SET_BREAK          0x05 //Set a BREAK
#define IMM_CHAR           0x06 //Send character out of order
#define SET_MHS            0x07 //Set modem handshaking
#define GET_MDMSTS         0x08 //Get modem status
#define SET_XON            0x09 //Emulate XON
#define SET_XOFF           0x0A //Emulate XOFF
#define SET_EVENTMASK      0x0B //Set the event mask
#define GET_EVENTMASK      0x0C //Get the event mask
#define GET_EVENTSTATE     0x16 //Get the event state
#define SET_CHAR           0x0D //Set special character individually
#define GET_CHARS          0x0E //Get special characters
#define GET_PROPS          0x0F //Get properties
#define GET_COMM_STATUS    0x10 //Get the serial status
#define RESET              0x11 //Reset
#define PURGE              0x12 //Purge清除
#define SET_FLOW           0x13 //Set flow control
#define GET_FLOW           0x14 //Get flow control
#define EMBED_EVENTS       0x15 //Control embedding of events in the data stream
#define GET_BAUDRATE       0x1D //Get the baud rate
#define SET_BAUDRATE       0x1E //Set the baud rate
#define SET_CHARS          0x19 //Set spcial characters
#define VENDOR_SPECIFIC    0xFF //Vendor specific command


/* ----------------------------------------------------------------------------------------------*/
typedef enum
{
	CUSTOM_DEVICE_IFC_ENABLE = 0,
	CUSTOM_DEVICE_IFC_DISABLE,
	CUSTOM_DEVICE_BEGIN,
	CUSTOM_DEVICE_GET_PROPS,
	CUSTOM_DEVICE_SET_LINE_CTL,
	CUSTOM_DEVICE_SET_FLOW,
	CUSTOM_DEVICE_SET_CHARS,
	CUSTOM_DEVICE_SET_BAUDRATE,
	CUSTOM_DEVICE_PURGE,
	IDLE,
}
CUSTOM_DEVICE_CtlState;

typedef enum
{
	CUSTOM_DEVICE_IDLE = 0,
	CUSTOM_DEVICE_SEND_DATA,
	CUSTOM_DEVICE_DATA_SENT,
	CUSTOM_DEVICE_Send6,
	CUSTOM_DEVICE_Send4,
	CUSTOM_DEVICE_SendCFG,
	CUSTOM_DEVICE_Send_done,
	CUSTOM_DEVICE_Receive,
	CUSTOM_DEVICE_Receive_done,
	CUSTOM_DEVICE5,
}
CUSTOM_DEVICE_State;


/* ----------------------------------------------------------------------------------------------*/
typedef struct _USBH_Process
{
	uint8_t              hc_num_in;
	uint8_t              hc_num_out;
	uint8_t              BulkOutEp;
	uint8_t              BulkInEp;
	uint16_t             BulkInEpSize;
	uint16_t             BulkOutEpSize;
	
	uint8_t              *buffout;//输出缓冲
	uint8_t              *buffin;//输入缓冲，
	uint32_t             lengthout;// buffout的长度
	uint32_t             lengthin;	// buffin的长度
	uint8_t              bmAttributes;
//	CUSTOM_DEVICE_CtlState      ctl_state;
	CUSTOM_DEVICE_State         state;
}
USBH_TypeDef;

/* States for CDC State Machine */
typedef enum
{
	CDC_IDLE= 0,
	CDC_READ_DATA,
	CDC_SEND_DATA,
	CDC_DATA_SENT,
	CDC_BUSY,
	CDC_GET_DATA,   
	CDC_POLL,
	CDC_CTRL_STATE
}
CDC_State;

/* CDC Transfer State */
typedef struct _CDCXfer
{
	volatile CDC_State CDCState;
	uint8_t* pRxTxBuff;
	uint8_t* pFillBuff;
	uint8_t* pEmptyBuff;
	uint32_t BufferLen;
	uint16_t DataLength;
} CDC_Xfer_TypeDef;

/* ----------------------------------------------------------------------------------------------*/
extern USBH_Class_cb_TypeDef  USBH_cb;
extern USBH_TypeDef USBH_CUSTOM_DEVICE;

/* ----------------------------------------------------------------------------------------------*/
static USBH_Status USBH_CUSTOM_DEVICE_InterfaceInit (USB_OTG_CORE_HANDLE *pdev,
                                              void *phost);

void USBH_CUSTOM_DEVICE_InterfaceDeInit (USB_OTG_CORE_HANDLE *pdev,
                                  void *phost);

static USBH_Status USBH_CUSTOM_DEVICE_ClassRequest (USB_OTG_CORE_HANDLE *pdev,
                                            void *phost);

static USBH_Status USBH_CUSTOM_DEVICE_Handle (USB_OTG_CORE_HANDLE *pdev,
                                       void   *phost);

static USBH_Status USBH_CUSTOM_DEVICE_BEGIN (USB_OTG_CORE_HANDLE *pdev,
                                      USBH_HOST *phost,
                                      uint8_t *buff,
                                      uint16_t length);

static USBH_Status USBH_CUSTOM_DEVICE_GET_PROPS (USB_OTG_CORE_HANDLE *pdev,
                                          USBH_HOST *phost,
                                          uint16_t length);

static USBH_Status USBH_CUSTOM_DEVICE_IFC_ENABLE (USB_OTG_CORE_HANDLE *pdev,
                                           USBH_HOST *phost,
                                           uint16_t length);

static USBH_Status USBH_CUSTOM_DEVICE_IFC_DISABLE (USB_OTG_CORE_HANDLE *pdev,
                                            USBH_HOST *phost,
                                            uint16_t length);

static USBH_Status USBH_CUSTOM_DEVICE_SET_LINE_CTL (USB_OTG_CORE_HANDLE *pdev,
                                             USBH_HOST *phost,
                                             uint16_t length);

static USBH_Status USBH_CUSTOM_DEVICE_SET_FLOW (USB_OTG_CORE_HANDLE *pdev,
                                         USBH_HOST *phost,
                                         uint16_t length);

static USBH_Status USBH_CUSTOM_DEVICE_SET_CHARS (USB_OTG_CORE_HANDLE *pdev,
                                          USBH_HOST *phost,
                                          uint16_t length);

static USBH_Status USBH_CUSTOM_DEVICE_SET_BAUDRATE (USB_OTG_CORE_HANDLE *pdev,
                                             USBH_HOST *phost,
                                             uint16_t length);

void  CDC_SendData(uint8_t *data, uint16_t length);
void  CDC_StartReception(void);
void  CDC_StopReception(void);

#endif

/*************************************** END OF FILE *********************************************/
