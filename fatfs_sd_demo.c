/*
*********************************************************************************************************
*
*	模块名称 : SD卡文件系统应用函数
*	文件名称 : fatfs_sd_demo.c
*	版    本 : V1.0（基于FatFs R0.10b (C)ChaN, 2014）
*	说    明 : 封装一些ff.c中的函数，便于使用.
*			   支持BMP图像显示
*	修改记录 :
*		版本号	日期       	作者	说明
*		V1.0	2014-10-21	JOY		
*
*********************************************************************************************************
*/
#include "fatfs_sd_demo.h"
#include <stdio.h>
#include <string.h>
#include "delay.h"
#include <stdlib.h>	// calloc

//FATFS fs;   /* 需要设置成全局变量 */
FIL file;
FRESULT res;

char pathname[_MAX_LFN+1];


/*
*********************************************************************************************************
*	函 数 名: FATFS_DriveSize
*	功能说明: 获得当前磁盘总大小和空余空间大小
*	形    参: uint32_t* total：总空间大小，单位KiB
*			  uint32_t* free：空余空间大小，单位KiB
*	返 回 值: FRESULT：成功返回FR_OK
*********************************************************************************************************
*/
FRESULT FATFS_DriveSize(uint32_t* total, uint32_t* free) 
{ 
	FATFS *fs;
    DWORD fre_clust; 

    /* Get volume information and free clusters of drive */ 
    res = f_getfree("0:", &fre_clust, &fs); 
    if (res != FR_OK) 
	{ 
		return res; 
	} 

    /* Get total sectors and free sectors */ 
    *total = (fs->n_fatent - 2) * fs->csize / 2; 
    *free = fre_clust * fs->csize / 2; 
	 
	/* Return OK */ 
	return FR_OK; 
} 


/*
*********************************************************************************************************
*	函 数 名: ViewRootDir
*	功能说明: 扫描SD卡指定目录下的文件，显示图片文件（bmp和gif格式）
*	形    参：char *path：扫描指定路径
*	返 回 值: 无
*********************************************************************************************************
*/
uint16_t FileCount=0;	
FRESULT scan_files (
    char *path        /* Start node to be scanned (also used as work area) */
)
{
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;   /* This function is assuming non-Unicode cfg. */
	
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) 
	{
        i = strlen(path);
		printf("Name\t\tSize\r\n");
        while(1)
		{
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
			
            if (fno.fattrib & AM_DIR) /* It is a directory */
			{      
				sprintf(&path[i], "/%s", fn);
				res = scan_files(path);       //函数递归调用
                if (res != FR_OK) break;
				path[i] = 0;	// 结尾加结束符
            } 
			else /* It is a file. */
			{             
				memset(pathname,0,_MAX_LFN+1);
				sprintf(pathname, "%s/%s",path,fn);
 				printf("%s\t",pathname);//	printf("%s/%s\t", path, fn);
				printf("%lu\r\n", fno.fsize);
				FileCount++;
            }

        }
        f_closedir(&dir);
		
    }
	
	if(FileCount) res = FR_OK;	
    return res;
}

/*
*********************************************************************************************************
*	函 数 名: Timer_Init
*	功能说明: 初始化定时器3, 1ms定时
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
static void Timer3_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef		NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	TIM_DeInit(TIM3);
	
	TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/100000 - 1;//720-1;				// 预分频72M/720 = 100kHz
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	// 向上计数模式
	TIM_TimeBaseStructure.TIM_Period = 100 - 1;					// 装载值 100k/100 = 1kHz(频率), 0.5s
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		// 不分割               
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
	// 配置并使能1ms定时器中断
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);		// 清除溢出中断标志
}
/*
*********************************************************************************************************
*	函 数 名: FATFS_SpeedTest
*	功能说明: 测试文件读写速度
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
/* 用于测试读写速度 */
#define TEST_FILE_LEN			(2*1024*1024)	/* 用于测试的文件长度 */
#define BUF_SIZE				512				/* 每次读写SD卡的最大数据长度 */
uint8_t g_TestBuf[BUF_SIZE];
uint8_t g_ReadTestBuf[BUF_SIZE];
extern __IO int32_t g_iRunTime;	/* 全局运行时间，单位1ms，用于计算系统运行时间 */
void FATFS_SpeedTest(void)
{
	DIR DirInf;  
	uint32_t bw;
	uint32_t i,j;
	uint32_t runtime1,runtime2,timelen;
	uint8_t err = 0;
	char buf[30];
	
	for (i = 0; i < sizeof(g_TestBuf); i++)
	{
		g_TestBuf[i] = (i % 10) + '0';
	}

	/* 打开根文件夹 */
	res = f_opendir(&DirInf, "/"); /* 如果不带参数，则从当前目录开始 */
	if (res != FR_OK) 
	{
		printf("打开根目录失败 (%d)\r\n", res);
		return;
	}

	Timer3_Init();		// 初始化1ms定时器
	
	/* 打开文件 */
	res = f_open(&file, "Speed1.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
		printf("打开文件失败 %d\r\n", res);
		return;
	}
	/* 写一串数据 */
	printf("开始写文件 %dKB ...\r\n", TEST_FILE_LEN / 1024);
	sprintf(buf, "Test Write %dKB...", TEST_FILE_LEN / 1024);
	
	/* 读取系统运行时间 */
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);		// 清除溢出中断标志
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);		// 使能溢出中断
 	TIM_Cmd(TIM3, ENABLE);							// 使能1ms定时器	
	runtime1 = g_iRunTime;
	for (i = 0; i < TEST_FILE_LEN / BUF_SIZE; i++)
	{
		res = f_write(&file, g_TestBuf, sizeof(g_TestBuf), &bw);	
		if (res == FR_OK)
		{
//			if ((i % 100) == 0)
//			{
//				/* 闪烁LED */
//			}
		}
		else
		{
			err = 1;
			printf("Speed1.txt 文件写失败\r\n");
			break;
		}
	}
	runtime2 = g_iRunTime;	/* 读取系统运行时间 */
	
	if (err == 0)
	{
		timelen = (runtime2 - runtime1);
		printf("  写耗长 : %dms   平均写速度 : %dB/S (%dKB/S)\r\n", timelen, 
			(TEST_FILE_LEN * 1000) / timelen, ((TEST_FILE_LEN / 1024) * 1000) / timelen);
		sprintf(buf, "WriteSpeed: %dKB/s", TEST_FILE_LEN/1024 * 1000/ timelen);
	}
	
	f_close(&file);		/* 关闭文件*/
	if(err)
		return;
		
	/* 开始读文件测试 */
	memset(g_ReadTestBuf, 0, sizeof(g_ReadTestBuf));
	res = f_open(&file, "Speed1.txt", FA_OPEN_EXISTING | FA_READ);
	if (res !=  FR_OK)
	{
		printf("没有找到文件: Speed1.txt\r\n");
		return;		
	}

	printf("开始读文件 %dKB ...\r\n", TEST_FILE_LEN / 1024);
	sprintf(buf, "Test Read %dKB...", TEST_FILE_LEN / 1024);
	runtime1 = g_iRunTime;	/* 读取系统运行时间 */
	for (i = 0; i < TEST_FILE_LEN / BUF_SIZE; i++)
	{
		res = f_read(&file, g_ReadTestBuf, sizeof(g_ReadTestBuf), &bw);	
		if (res == FR_OK)
		{
//			if ((i % 100) == 0)
//			{
//				/* 闪烁LED */
//			}
		}
		else
		{
			err = 1;
			printf("Speed1.txt 文件读失败\r\n");
			break;
		}
	}
	runtime2 = g_iRunTime;	/* 读取系统运行时间 */
	
	if (err == 0)
	{
		timelen = (runtime2 - runtime1);
		printf("  读耗时 : %dms   平均读速度 : %dB/S (%dKB/S)\r\n", timelen, 
			(TEST_FILE_LEN * 1000) / timelen, ((TEST_FILE_LEN / 1024) * 1000) / timelen);
		sprintf(buf, "ReadSpeed: %dKB/s", TEST_FILE_LEN/1024 * 1000/ timelen);
	}
	
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);		// 清除溢出中断标志
	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);		// 禁止溢出中断
 	TIM_Cmd(TIM3, DISABLE);							// 禁止1ms定时器	
	g_iRunTime=0;
	/* 关闭文件*/
	f_close(&file);
	
	
	// 判断写入是否正确
	printf("开始验证 ...\r\n");
	err = 0;
	memset(g_ReadTestBuf, 0, sizeof(g_ReadTestBuf));
	res = f_open(&file, "Speed1.txt", FA_OPEN_EXISTING | FA_READ);
	if (res !=  FR_OK)
	{
		printf("没有找到文件: Speed1.txt\r\n");
		return;		
	}
	for (i = 0; i < TEST_FILE_LEN / BUF_SIZE; i++)
	{
		res = f_read(&file, g_ReadTestBuf, sizeof(g_ReadTestBuf), &bw);	
		if (res == FR_OK)
		{
			for(j=0; j<bw; j++)
			{
				if(g_ReadTestBuf[j] != g_TestBuf[j])
				{
					printf("验证错误！第%d次[%d]\r\n(r:%d, w:%d)\r\n", i,j,g_ReadTestBuf[j] , g_TestBuf[j]);
					err = 1;
					break;
				}
			}
//			if ((i % 100) == 0)
//			{
//				/* 闪烁LED */
//			}
		}
		else
		{
			err = 1;
			printf("Speed1.txt 文件验证失败\r\n");
			break;
		}
	}
	if(err == 0)
		printf("验证成功！\r\n");
	
	/* 关闭文件*/
	f_close(&file);
 	delay_ms(2000);
}

void FATFS_WriteTest(uint16_t _times)
{
	DIR DirInf;  
	uint32_t bw;
	uint32_t i,j;
	uint8_t err = 0;
	char buf[30];
	
	for (i = 0; i < sizeof(g_TestBuf); i++)
	{
		g_TestBuf[i] = (i % 10) + '0';
	}

	/* 打开根文件夹 */
	res = f_opendir(&DirInf, "/"); /* 如果不带参数，则从当前目录开始 */
	if (res != FR_OK) 
	{
		printf("打开根目录失败 (%d)\r\n", res);
		return;
	}

	
	/* 打开文件 */
	sprintf(buf, "WTest%d.txt", _times);	
	res = f_open(&file, buf, FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
		printf("打开文件失败 %d\r\n", res);
		return;
	}
	/* 写一串数据 */
	printf("写文件 \"%s\" ...", buf);

	for (i = 0; i < TEST_FILE_LEN / BUF_SIZE; i++)
	{
		res = f_write(&file, g_TestBuf, sizeof(g_TestBuf), &bw);	
		if (res == FR_OK)
		{
//			if ((i % 100) == 0)
//			{
//				/* 闪烁LED */
//			}
		}
		else
		{
			err = 1;
			printf("  失败\r\n\r\n");
			break;
		}
	}
	
	if (err == 0)
	{
		printf("  成功\r\n\r\n");
	}
	
	f_close(&file);		/* 关闭文件*/

 	delay_ms(100);
}
