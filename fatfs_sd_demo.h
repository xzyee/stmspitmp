/*
*********************************************************************************************************
*
*	ģ������ : SD���ļ�ϵͳӦ�ú���
*	�ļ����� : fatfs_sd_demo.h
*	��    �� : V1.0������FatFs R0.10b (C)ChaN, 2014��
*	˵    �� : ��װһЩff.c�еĺ���������ʹ��
*	�޸ļ�¼ :
*		�汾��	����       	����	˵��
*		V1.0	2014-10-21	JOY		
*
*********************************************************************************************************
*/
#ifndef __FATFS_SD_DEMO_H
#define __FATFS_SD_DEMO_H

#include "stm32f10x.h"
#include "ff.h"


// FatFs�ļ�ϵͳ��ȫ�ֱ���������������ϲ��ļ�����
//extern FATFS fs;
extern FIL file;
extern FRESULT res;


FRESULT FATFS_DriveSize(uint32_t* total, uint32_t* free);
void FATFS_SpeedTest(void);
FRESULT scan_files(char *path);/* Start node to be scanned (also used as work area) */
void FATFS_WriteTest(uint16_t _times);

#endif
