/*
*********************************************************************************************************
*
*	模块名称 : SD卡文件系统应用函数
*	文件名称 : fatfs_sd_demo.h
*	版    本 : V1.0（基于FatFs R0.10b (C)ChaN, 2014）
*	说    明 : 封装一些ff.c中的函数，便于使用
*	修改记录 :
*		版本号	日期       	作者	说明
*		V1.0	2014-10-21	JOY		
*
*********************************************************************************************************
*/
#ifndef __FATFS_SD_DEMO_H
#define __FATFS_SD_DEMO_H

#include "stm32f10x.h"
#include "ff.h"


// FatFs文件系统用全局变量，声明后便于上层文件调用
//extern FATFS fs;
extern FIL file;
extern FRESULT res;


FRESULT FATFS_DriveSize(uint32_t* total, uint32_t* free);
void FATFS_SpeedTest(void);
FRESULT scan_files(char *path);/* Start node to be scanned (also used as work area) */
void FATFS_WriteTest(uint16_t _times);

#endif
