/*
*********************************************************************************************************
*
*	ģ������ : SD���ļ�ϵͳӦ�ú���
*	�ļ����� : fatfs_sd_demo.c
*	��    �� : V1.0������FatFs R0.10b (C)ChaN, 2014��
*	˵    �� : ��װһЩff.c�еĺ���������ʹ��.
*			   ֧��BMPͼ����ʾ
*	�޸ļ�¼ :
*		�汾��	����       	����	˵��
*		V1.0	2014-10-21	JOY		
*
*********************************************************************************************************
*/
#include "fatfs_sd_demo.h"
#include <stdio.h>
#include <string.h>
#include "delay.h"
#include <stdlib.h>	// calloc

//FATFS fs;   /* ��Ҫ���ó�ȫ�ֱ��� */
FIL file;
FRESULT res;

char pathname[_MAX_LFN+1];


/*
*********************************************************************************************************
*	�� �� ��: FATFS_DriveSize
*	����˵��: ��õ�ǰ�����ܴ�С�Ϳ���ռ��С
*	��    ��: uint32_t* total���ܿռ��С����λKiB
*			  uint32_t* free������ռ��С����λKiB
*	�� �� ֵ: FRESULT���ɹ�����FR_OK
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
*	�� �� ��: ViewRootDir
*	����˵��: ɨ��SD��ָ��Ŀ¼�µ��ļ�����ʾͼƬ�ļ���bmp��gif��ʽ��
*	��    �Σ�char *path��ɨ��ָ��·��
*	�� �� ֵ: ��
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
				res = scan_files(path);       //�����ݹ����
                if (res != FR_OK) break;
				path[i] = 0;	// ��β�ӽ�����
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
*	�� �� ��: Timer_Init
*	����˵��: ��ʼ����ʱ��3, 1ms��ʱ
*	��    �Σ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void Timer3_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef		NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	TIM_DeInit(TIM3);
	
	TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/100000 - 1;//720-1;				// Ԥ��Ƶ72M/720 = 100kHz
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	// ���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period = 100 - 1;					// װ��ֵ 100k/100 = 1kHz(Ƶ��), 0.5s
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		// ���ָ�               
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
	// ���ò�ʹ��1ms��ʱ���ж�
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);		// �������жϱ�־
}
/*
*********************************************************************************************************
*	�� �� ��: FATFS_SpeedTest
*	����˵��: �����ļ���д�ٶ�
*	��    �Σ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
/* ���ڲ��Զ�д�ٶ� */
#define TEST_FILE_LEN			(2*1024*1024)	/* ���ڲ��Ե��ļ����� */
#define BUF_SIZE				512				/* ÿ�ζ�дSD����������ݳ��� */
uint8_t g_TestBuf[BUF_SIZE];
uint8_t g_ReadTestBuf[BUF_SIZE];
extern __IO int32_t g_iRunTime;	/* ȫ������ʱ�䣬��λ1ms�����ڼ���ϵͳ����ʱ�� */
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

	/* �򿪸��ļ��� */
	res = f_opendir(&DirInf, "/"); /* ���������������ӵ�ǰĿ¼��ʼ */
	if (res != FR_OK) 
	{
		printf("�򿪸�Ŀ¼ʧ�� (%d)\r\n", res);
		return;
	}

	Timer3_Init();		// ��ʼ��1ms��ʱ��
	
	/* ���ļ� */
	res = f_open(&file, "Speed1.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
		printf("���ļ�ʧ�� %d\r\n", res);
		return;
	}
	/* дһ������ */
	printf("��ʼд�ļ� %dKB ...\r\n", TEST_FILE_LEN / 1024);
	sprintf(buf, "Test Write %dKB...", TEST_FILE_LEN / 1024);
	
	/* ��ȡϵͳ����ʱ�� */
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);		// �������жϱ�־
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);		// ʹ������ж�
 	TIM_Cmd(TIM3, ENABLE);							// ʹ��1ms��ʱ��	
	runtime1 = g_iRunTime;
	for (i = 0; i < TEST_FILE_LEN / BUF_SIZE; i++)
	{
		res = f_write(&file, g_TestBuf, sizeof(g_TestBuf), &bw);	
		if (res == FR_OK)
		{
//			if ((i % 100) == 0)
//			{
//				/* ��˸LED */
//			}
		}
		else
		{
			err = 1;
			printf("Speed1.txt �ļ�дʧ��\r\n");
			break;
		}
	}
	runtime2 = g_iRunTime;	/* ��ȡϵͳ����ʱ�� */
	
	if (err == 0)
	{
		timelen = (runtime2 - runtime1);
		printf("  д�ĳ� : %dms   ƽ��д�ٶ� : %dB/S (%dKB/S)\r\n", timelen, 
			(TEST_FILE_LEN * 1000) / timelen, ((TEST_FILE_LEN / 1024) * 1000) / timelen);
		sprintf(buf, "WriteSpeed: %dKB/s", TEST_FILE_LEN/1024 * 1000/ timelen);
	}
	
	f_close(&file);		/* �ر��ļ�*/
	if(err)
		return;
		
	/* ��ʼ���ļ����� */
	memset(g_ReadTestBuf, 0, sizeof(g_ReadTestBuf));
	res = f_open(&file, "Speed1.txt", FA_OPEN_EXISTING | FA_READ);
	if (res !=  FR_OK)
	{
		printf("û���ҵ��ļ�: Speed1.txt\r\n");
		return;		
	}

	printf("��ʼ���ļ� %dKB ...\r\n", TEST_FILE_LEN / 1024);
	sprintf(buf, "Test Read %dKB...", TEST_FILE_LEN / 1024);
	runtime1 = g_iRunTime;	/* ��ȡϵͳ����ʱ�� */
	for (i = 0; i < TEST_FILE_LEN / BUF_SIZE; i++)
	{
		res = f_read(&file, g_ReadTestBuf, sizeof(g_ReadTestBuf), &bw);	
		if (res == FR_OK)
		{
//			if ((i % 100) == 0)
//			{
//				/* ��˸LED */
//			}
		}
		else
		{
			err = 1;
			printf("Speed1.txt �ļ���ʧ��\r\n");
			break;
		}
	}
	runtime2 = g_iRunTime;	/* ��ȡϵͳ����ʱ�� */
	
	if (err == 0)
	{
		timelen = (runtime2 - runtime1);
		printf("  ����ʱ : %dms   ƽ�����ٶ� : %dB/S (%dKB/S)\r\n", timelen, 
			(TEST_FILE_LEN * 1000) / timelen, ((TEST_FILE_LEN / 1024) * 1000) / timelen);
		sprintf(buf, "ReadSpeed: %dKB/s", TEST_FILE_LEN/1024 * 1000/ timelen);
	}
	
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);		// �������жϱ�־
	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);		// ��ֹ����ж�
 	TIM_Cmd(TIM3, DISABLE);							// ��ֹ1ms��ʱ��	
	g_iRunTime=0;
	/* �ر��ļ�*/
	f_close(&file);
	
	
	// �ж�д���Ƿ���ȷ
	printf("��ʼ��֤ ...\r\n");
	err = 0;
	memset(g_ReadTestBuf, 0, sizeof(g_ReadTestBuf));
	res = f_open(&file, "Speed1.txt", FA_OPEN_EXISTING | FA_READ);
	if (res !=  FR_OK)
	{
		printf("û���ҵ��ļ�: Speed1.txt\r\n");
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
					printf("��֤���󣡵�%d��[%d]\r\n(r:%d, w:%d)\r\n", i,j,g_ReadTestBuf[j] , g_TestBuf[j]);
					err = 1;
					break;
				}
			}
//			if ((i % 100) == 0)
//			{
//				/* ��˸LED */
//			}
		}
		else
		{
			err = 1;
			printf("Speed1.txt �ļ���֤ʧ��\r\n");
			break;
		}
	}
	if(err == 0)
		printf("��֤�ɹ���\r\n");
	
	/* �ر��ļ�*/
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

	/* �򿪸��ļ��� */
	res = f_opendir(&DirInf, "/"); /* ���������������ӵ�ǰĿ¼��ʼ */
	if (res != FR_OK) 
	{
		printf("�򿪸�Ŀ¼ʧ�� (%d)\r\n", res);
		return;
	}

	
	/* ���ļ� */
	sprintf(buf, "WTest%d.txt", _times);	
	res = f_open(&file, buf, FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
		printf("���ļ�ʧ�� %d\r\n", res);
		return;
	}
	/* дһ������ */
	printf("д�ļ� \"%s\" ...", buf);

	for (i = 0; i < TEST_FILE_LEN / BUF_SIZE; i++)
	{
		res = f_write(&file, g_TestBuf, sizeof(g_TestBuf), &bw);	
		if (res == FR_OK)
		{
//			if ((i % 100) == 0)
//			{
//				/* ��˸LED */
//			}
		}
		else
		{
			err = 1;
			printf("  ʧ��\r\n\r\n");
			break;
		}
	}
	
	if (err == 0)
	{
		printf("  �ɹ�\r\n\r\n");
	}
	
	f_close(&file);		/* �ر��ļ�*/

 	delay_ms(100);
}
