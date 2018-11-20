/**
  ******************************************************************************
  * @file    usbh_custom_core.c
  * @author  yaoohui
  * @version V1.0
  * @date    2016年2月20日
  * @brief   
  ******************************************************************************
*/ 


/* Includes -------------------------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "usbh_custom_core.h"
#include "usart.h"
#include "delay.h"
#include "usbh_custom_usr.h"

/* ----------------------------------------------------------------------------------------------*/
#define CDC_BUFFER_SIZE                 1024

__ALIGN_BEGIN
	USBH_TypeDef USBH_CUSTOM_DEVICE
__ALIGN_END;

CDC_Xfer_TypeDef                    CDC_TxParam;
CDC_Xfer_TypeDef                    CDC_RxParam;

__ALIGN_BEGIN uint8_t               TxBuf [CDC_BUFFER_SIZE] __ALIGN_END ;	// 发送缓冲区
__ALIGN_BEGIN uint8_t               RxBuf [4100] __ALIGN_END ;	// 接收缓冲区

/* ----------------------------------------------------------------------------------------------*/
USBH_Class_cb_TypeDef  USBH_cb = 
{
	USBH_CUSTOM_DEVICE_InterfaceInit,//USB类初始化，包括设备释放被支持和打开通信管道
	USBH_CUSTOM_DEVICE_InterfaceDeInit,//释放USB资源
	USBH_CUSTOM_DEVICE_ClassRequest,//USB类请求
	USBH_CUSTOM_DEVICE_Handle//USB数据处理线程
};


/* ----------------------------------------------------------------------------------------------*/
uint8_t buff_SetBaudrate[6] = {0x60, 0x09, 0, 0};
// uint8_t buff_SetBaudrate[6] = {0x80, 0x25, 0, 0};
uint8_t buff_SetFlow1[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t buff_SetFlow2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00};
uint8_t buff_SetFlow3[16] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
uint8_t buff_SetChars[6] = {0, 0, 0, 0, 0x11, 0x13};
uint8_t SendBuff[34] = {0x3C};

//extern __IO uint8_t TxBuffer[64];
uint8_t Uart2_Rx;

uint8_t                             RX_Enabled = 0;

extern uint16_t RxDataLength;	// 需要接收设备的数据长度
extern uint8_t RxDataComplete;	// 接收数据完成标志

static void CDC_ProcessTransmission(USB_OTG_CORE_HANDLE *pdev, USBH_HOST  *phost);
static void CDC_ProcessReception(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost);
//static void CDC_ReceiveData(CDC_Xfer_TypeDef *cdc_Data);


/* ----------------------------------------------------------------------------------------------*/
//
//初始化CUSTOM_DEVICE接口
//
static USBH_Status USBH_CUSTOM_DEVICE_InterfaceInit (USB_OTG_CORE_HANDLE *pdev, void *phost)
{	
	uint8_t maxEP;
	uint8_t num = 0;
	
	USBH_HOST *pphost = phost;

	USBH_Status status = USBH_BUSY;

	if((pphost->device_prop.Dev_Desc.idVendor == 0x0A2D) && (pphost->device_prop.Dev_Desc.idProduct == 0x000F)) //VID和PID是指定连接的设备时
	{
		MyDevice.istarget = 1;	// 为目标设备
		printf(" > Detected Target Device\r\n");
		
		maxEP = ( (pphost->device_prop.Itf_Desc[0].bNumEndpoints <= USBH_MAX_NUM_ENDPOINTS) ? 
					pphost->device_prop.Itf_Desc[0].bNumEndpoints :
					USBH_MAX_NUM_ENDPOINTS);
//		printf("  > bNumEndpoints : %d \r\n" , maxEP);

		for (num=0; num < maxEP; num++)
		{
			if(pphost->device_prop.Ep_Desc[0][num].bEndpointAddress & 0x80)
			{
				USBH_CUSTOM_DEVICE.BulkInEp = pphost->device_prop.Ep_Desc[0][num].bEndpointAddress;
				USBH_CUSTOM_DEVICE.hc_num_in  = USBH_Alloc_Channel(pdev, pphost->device_prop.Ep_Desc[0][num].bEndpointAddress);
				USBH_CUSTOM_DEVICE.BulkInEpSize  = pphost->device_prop.Ep_Desc[0][num].wMaxPacketSize;
				USBH_CUSTOM_DEVICE.bmAttributes  = pphost->device_prop.Ep_Desc[0][num].bmAttributes;

				// Open channel for IN endpoint
				USBH_Open_Channel  (pdev,
									USBH_CUSTOM_DEVICE.hc_num_in,
									pphost->device_prop.address,
									pphost->device_prop.speed,
									EP_TYPE_BULK,
									USBH_CUSTOM_DEVICE.BulkInEpSize); 
//				printf("   > bEndpointAddress : 0x%02X \r\n" , USBH_CUSTOM_DEVICE.BulkInEp);
//				printf("    > bmAttributes : 0x%02X \r\n" , USBH_CUSTOM_DEVICE.bmAttributes);
//				printf("    > wMaxPacketSize : 0x%02X \r\n" , USBH_CUSTOM_DEVICE.BulkInEpSize);
				
				// 初始化接收结构体
				CDC_RxParam.CDCState = CDC_IDLE;
				CDC_RxParam.DataLength = 0;
				CDC_RxParam.pFillBuff = RxBuf;  
				CDC_RxParam.pEmptyBuff = RxBuf;
				CDC_RxParam.BufferLen = sizeof(RxBuf);      
			}
			else
			{
				USBH_CUSTOM_DEVICE.BulkOutEp = pphost->device_prop.Ep_Desc[0][num].bEndpointAddress;
				USBH_CUSTOM_DEVICE.hc_num_out  = USBH_Alloc_Channel(pdev, pphost->device_prop.Ep_Desc[0][num].bEndpointAddress);
				USBH_CUSTOM_DEVICE.BulkOutEpSize  = pphost->device_prop.Ep_Desc[0][num].wMaxPacketSize;
				USBH_CUSTOM_DEVICE.bmAttributes  = pphost->device_prop.Ep_Desc[0][num].bmAttributes;

				// Open channel for OUT endpoint
				USBH_Open_Channel  (pdev,
									USBH_CUSTOM_DEVICE.hc_num_out,
									pphost->device_prop.address,
									pphost->device_prop.speed,
									EP_TYPE_BULK,
									USBH_CUSTOM_DEVICE.BulkOutEpSize); 
//				printf("   > bEndpointAddress : 0x%02X \r\n" , USBH_CUSTOM_DEVICE.BulkOutEp);
//				printf("    > bmAttributes : 0x%02X \r\n" , USBH_CUSTOM_DEVICE.bmAttributes);
//				printf("    > wMaxPacketSize : 0x%02X \r\n" , USBH_CUSTOM_DEVICE.BulkOutEpSize);
				
				// 初始化发送结构体
				CDC_TxParam.CDCState = CDC_IDLE;
				CDC_TxParam.DataLength = 0;
				CDC_TxParam.pRxTxBuff = TxBuf;
			}
			
			status = USBH_OK;
//			USBH_CUSTOM_DEVICE.ctl_state = CUSTOM_DEVICE_BEGIN;
			USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_IDLE;
		}   
	}
	else //不是指定的设备
	{
		MyDevice.istarget = 0;	// 不是目标设备
		pphost->usr_cb->DeviceNotSupported();
	}
//	printf("------------------------------------\r\n");

	return status;
}


//
//De-Initialize interface by freeing host channels allocated to interface
//
void USBH_CUSTOM_DEVICE_InterfaceDeInit (USB_OTG_CORE_HANDLE *pdev, void *phost)
{	
	if(USBH_CUSTOM_DEVICE.hc_num_in != 0x00)
	{
		USB_OTG_HC_Halt(pdev, USBH_CUSTOM_DEVICE.hc_num_in);
		USBH_Free_Channel(pdev, USBH_CUSTOM_DEVICE.hc_num_in);
		USBH_CUSTOM_DEVICE.hc_num_in = 0; //Reset the Channel as Free
	}

	if(USBH_CUSTOM_DEVICE.hc_num_out != 0x00)
	{
		USB_OTG_HC_Halt(pdev, USBH_CUSTOM_DEVICE.hc_num_out);
		USBH_Free_Channel(pdev, USBH_CUSTOM_DEVICE.hc_num_out);
		USBH_CUSTOM_DEVICE.hc_num_out = 0; //Reset the Channel as Free
	}
}


//
// 设置控制端口
//
static USBH_Status USBH_CUSTOM_DEVICE_ClassRequest (USB_OTG_CORE_HANDLE *pdev, void *phost)
{   
//	USBH_HOST *pphost = phost;

	USBH_Status status = USBH_BUSY;

	status = USBH_OK;
	return status; 
}


//
// 管理数据传输的状态机
//
static USBH_Status USBH_CUSTOM_DEVICE_Handle (USB_OTG_CORE_HANDLE *pdev, void *phost)
{
	USBH_Status status = USBH_OK;	
	USBH_HOST *pphost = phost;
//	URB_STATE URB_StatusTx = URB_IDLE; 
//	static uint32_t len;
	
	/* Call Application process */
	pphost->usr_cb->UserApplication();  // 启动用户程序

	
	/*Handle the transmission */
	CDC_ProcessTransmission(pdev, pphost);

	/*Always send in packet to device*/    
	CDC_ProcessReception(pdev, pphost);
/*	
	URB_StatusTx = HCD_GetURB_State(pdev , USBH_CUSTOM_DEVICE.hc_num_out);

	switch (USBH_CUSTOM_DEVICE.state)
	{
		case CUSTOM_DEVICE_IDLE:
			break;
		
		case CUSTOM_DEVICE_SEND_DATA:
			if((URB_StatusTx == URB_DONE) || (URB_StatusTx == URB_IDLE))
			{
				// 检查数据长度是否过长
				if(USBH_CUSTOM_DEVICE.lengthout > USBH_CUSTOM_DEVICE.BulkOutEpSize)
				{
					len = USBH_CUSTOM_DEVICE.BulkOutEpSize;
					// 发送数据
					USBH_BulkSendData(pdev, USBH_CUSTOM_DEVICE.buffout, len, USBH_CUSTOM_DEVICE.hc_num_out);
				}
				else
				{
					len = USBH_CUSTOM_DEVICE.lengthout;
					// 发送剩余数据
					USBH_BulkSendData(pdev, USBH_CUSTOM_DEVICE.buffout, len, USBH_CUSTOM_DEVICE.hc_num_out);					
				}
				USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_DATA_SENT;
			}
			break;
			
		case CUSTOM_DEVICE_DATA_SENT:
			// 检查发送状态
			if(URB_StatusTx == URB_DONE)
			{
				USBH_CUSTOM_DEVICE.buffout += len;		// 指向下一包数据
				USBH_CUSTOM_DEVICE.lengthout -= len;	// 减少数据长度
				
				if(USBH_CUSTOM_DEVICE.lengthout == 0)
				{
					USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_IDLE;
				}
				else
				{
					USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_SEND_DATA; 
				}
			}
			
			break;
		
//		case CUSTOM_DEVICE_Send6:
//			TxBuffer[0] = 0x06;
//			USBH_BulkSendData(pdev, (uint8_t *)TxBuffer, 1, USBH_CUSTOM_DEVICE.hc_num_out);
//			USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_Send_done;
//		break;
//		
//		case CUSTOM_DEVICE_Send4:
//			TxBuffer[0] = 0x04;
//			USBH_BulkSendData(pdev, TxBuffer, 1, USBH_CUSTOM_DEVICE.hc_num_out);
//			USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_Send_done;
//		break;
//		
//		case CUSTOM_DEVICE_SendCFG:
//			USBH_BulkSendData(pdev, TxBuffer, 1, USBH_CUSTOM_DEVICE.hc_num_out);
//			USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_Send_done;
//		break;
		
		case CUSTOM_DEVICE_Send_done:
			if(HCD_GetURB_State(pdev, USBH_CUSTOM_DEVICE.hc_num_out) == URB_DONE)
			{
				USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_Receive;
			}
		break;

		case CUSTOM_DEVICE_Receive:
			USBH_BulkReceiveData(pdev, USBH_CUSTOM_DEVICE.buffin, 1, USBH_CUSTOM_DEVICE.hc_num_in);
			USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_Receive_done;
		break;
		
		case CUSTOM_DEVICE_Receive_done:
			if(HCD_GetURB_State(pdev, USBH_CUSTOM_DEVICE.hc_num_in) == URB_DONE)
			{
				printf("Received date = %d\r\n",USBH_CUSTOM_DEVICE.buffin[0]);
				USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_IDLE;
			}
		break;
	
		default:
		break;
	}*/
	return status;
}

/**
  * @brief  The function is responsible for sending data to the device
  * @param  pdev: Selected device
  * @retval None
  */
void CDC_ProcessTransmission(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
	static uint32_t len ;
	URB_STATE URB_StatusTx = URB_IDLE;

	URB_StatusTx =   HCD_GetURB_State(pdev , USBH_CUSTOM_DEVICE.hc_num_out);

	switch(CDC_TxParam.CDCState)
	{
		case CDC_IDLE:
			break;

		case CDC_SEND_DATA:
			if(( URB_StatusTx == URB_DONE ) || (URB_StatusTx == URB_IDLE))
			{
				// 检查数据长度是否超过端点定义，超过，则先发送端点定义的长度
				if(CDC_TxParam.DataLength > USBH_CUSTOM_DEVICE.BulkOutEpSize)
				{
					len = USBH_CUSTOM_DEVICE.BulkOutEpSize;
					// 发送一包数据
					USBH_BulkSendData (pdev,CDC_TxParam.pRxTxBuff, len, USBH_CUSTOM_DEVICE.hc_num_out);    
				}
				else
				{
					len = CDC_TxParam.DataLength ;
					// 发送剩余数据
					USBH_BulkSendData (pdev,CDC_TxParam.pRxTxBuff, len, USBH_CUSTOM_DEVICE.hc_num_out);    
				}
				
				CDC_TxParam.CDCState = CDC_DATA_SENT;
			}
			break;

		case CDC_DATA_SENT:
			// 检查数据传输状态
			if(URB_StatusTx == URB_DONE )
			{         
				// 数据发送缓冲区指针移向下一包
				CDC_TxParam.pRxTxBuff += len ;

				// 减少数据长度
				CDC_TxParam.DataLength -= len;    

				if(CDC_TxParam.DataLength == 0)	// 数据已经发送完成
				{
					CDC_TxParam.CDCState = CDC_IDLE;
				}
				else	// 数据未发送完成，继续 CDC_SEND_DATA 状态机
				{
					CDC_TxParam.CDCState = CDC_SEND_DATA; 
				}
			}
			else if( URB_StatusTx == URB_NOTREADY )
			{
				// 重新发送相同的数据
				USBH_BulkSendData (pdev,CDC_TxParam.pRxTxBuff, len, USBH_CUSTOM_DEVICE.hc_num_out);    
			}
			break;
	}
}

/**
  * @brief  This function responsible for reception of data from the device
  * @param  pdev: Selected device
  * @retval None
  */
static void CDC_ProcessReception(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  
	if(RX_Enabled == 1)
	{
		URB_STATE URB_StatusRx = HCD_GetURB_State(pdev, USBH_CUSTOM_DEVICE.hc_num_in);  

		switch(CDC_RxParam.CDCState)
		{
			case CDC_IDLE:
				// 检查接收数据长度是否小于接收缓冲区大小
				if(CDC_RxParam.DataLength < (CDC_RxParam.BufferLen - USBH_CUSTOM_DEVICE.BulkInEpSize))
				{
					// 接收数据，长度为实际IN端点长度
					USBH_BulkReceiveData(pdev, CDC_RxParam.pFillBuff, USBH_CUSTOM_DEVICE.BulkInEpSize, USBH_CUSTOM_DEVICE.hc_num_in);

					// 状态机变为 USBH_CDC_GET_DATA 接收数据
					CDC_RxParam.CDCState = CDC_GET_DATA;
				}
				break;

			case CDC_GET_DATA:
				// 检查上一次设备状态是否完成
				if(URB_StatusRx == URB_DONE)
				{
					// 移动接收缓冲区数据指针 和数据长度
					CDC_RxParam.DataLength += pdev->host.hc[USBH_CUSTOM_DEVICE.hc_num_in].xfer_count;
					CDC_RxParam.pFillBuff += pdev->host.hc[USBH_CUSTOM_DEVICE.hc_num_in].xfer_count;

					if(RxDataLength == CDC_RxParam.DataLength)
					{
						RxDataComplete = 1;	// 接收完成标志
						CDC_RxParam.DataLength = 0;	// 数据长度清零
						CDC_RxParam.pFillBuff  = CDC_RxParam.pEmptyBuff ; // 缓冲区指针指向开头
						
						RX_Enabled = 0;
					}
//					/* Process the recived data */
//					CDC_ReceiveData(&CDC_RxParam);

					/*change the state od the CDC state*/
					CDC_RxParam.CDCState = CDC_IDLE;
				}
				break;
		}
	}
}


///**
//  * @brief  This is a call back function from cdc core layer to redirect the 
//  *         received data on the user out put system
//  * @param  cdc_Data: type of USBH_CDCXfer_TypeDef
//  * @retval None
//  */
//static void CDC_ReceiveData(CDC_Xfer_TypeDef *cdc_Data)
//{
//	uint8_t *ptr; 

//	if(cdc_Data->pEmptyBuff < cdc_Data->pFillBuff)
//	{
//		ptr = cdc_Data->pFillBuff;
//		*ptr = 0x00;

//		/* redirect the received data on the user out put system */
//		UserCb.Receive(cdc_Data->pEmptyBuff);

//		cdc_Data->pFillBuff  = cdc_Data->pEmptyBuff ; 
//		cdc_Data->DataLength = 0;    /*Reset the data length to zero*/
//	}
//}

/**
  * @brief  This function send data to the device.
  * @param  fileName : name of the file 
  * @retval the filestate will be returned 
  * FS_SUCCESS : returned to the parent function when the file length become to zero
  */
void  CDC_SendData(uint8_t *data, uint16_t length)
{
	if(CDC_TxParam.CDCState == CDC_IDLE)
	{
		CDC_TxParam.pRxTxBuff = data; 
		CDC_TxParam.DataLength = length;
		CDC_TxParam.CDCState = CDC_SEND_DATA;  
	}    
  
//	if(USBH_CUSTOM_DEVICE.state == CUSTOM_DEVICE_IDLE)
//	{
//		USBH_CUSTOM_DEVICE.buffout = data;
//		USBH_CUSTOM_DEVICE.lengthout = length;
//		USBH_CUSTOM_DEVICE.state = CUSTOM_DEVICE_SEND_DATA;  
//	}    
}

/**
  * @brief  This function send data to the device.
  * @param  fileName : name of the file 
  * @retval the filestate will be returned 
  * FS_SUCCESS : returned to the parent function when the file length become to zero
  */
void  CDC_StartReception(void)
{
	RX_Enabled = 1;
}

/**
  * @brief  This function send data to the device.
  * @param  fileName : name of the file 
  * @retval the filestate will be returned 
  * FS_SUCCESS : returned to the parent function when the file length become to zero
  */
void  CDC_StopReception(void)// USB_OTG_CORE_HANDLE *pdev)
{
	RX_Enabled = 0; 
//	USB_OTG_HC_Halt(pdev, CDC_Machine.CDC_DataItf.hc_num_in);
//	USBH_Free_Channel(pdev,CDC_Machine.CDC_DataItf.hc_num_in);
}

/*********************************** (C) COPYRIGHT yaoohui ***************************************/
/*************************************** END OF FILE *********************************************/
