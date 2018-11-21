//TAS1020A USB声卡
//By Rush,数码之家
//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*================================================== 
Softpll.c: 时钟与异步控制
//================================================*/
#include <Reg52.h>
#include "..\rom\types.h"
#include "..\rom\Reg_stc1.h"
#include "..\rom\usbdfu.h"
#include "devDesc.h"
#include "device.h"
#include "Softpll.h"

word EngAcgCap1, EngAcgCap2;

/*====================================================================
softPllInit(): DAC时钟初始化

=====================================================================*/
void softPllInit()   
{
	EngAcgCap1 = 0;
	EngAcgCap2 = 0;


	//使用外部11.2896MHZ输入
	ACGDCTL   = 0x00;//不分频
	ACGCTL  = 0x4d;//使能分频器,MCLK 1 2选择外部MCLKIN输入


}

/*====================================================================
softPll(): SOF中断程序，计算异步反馈量

=====================================================================*/
#include "..\ROM\Mmap.h"
#include "Devmap.h"
#include "..\ROM\hwMacro.h"


void softPll()
{ 
    unsigned int BufSize; 
		
	BufSize = DMABCNT0L | (DMABCNT0H << 8);//FIFO中留存的数据量
	
	if (BufSize > (OUTEP1_RSIZE /4 ) ) 
	{
			INEP2_X[0]=0x33;	INEP2_X[1]=0x03;	INEP2_X[2]=0x0b;//吃饱了撑的
	}
	else if (BufSize > (OUTEP1_RSIZE /8 ) ) 
	{
			INEP2_X[0]=0x66;	INEP2_X[1]=0x06;	INEP2_X[2]=0x0b;//44.100	
	}
	else
	{
			INEP2_X[0]=0x99;	INEP2_X[1]=0x09;	INEP2_X[2]=0x0b;//喂多点
	}
//修改上方的数据可以更激进的要求垃圾PC提供更多的数据填充缓存

	IEPDCNTX2 = 3;IEPDCNTY2 = 3; 	// let's send it to PC.	


	// Check if host drops part of a sample
    if (AppDevice.spkIf.curSetting != DEV_SPK_SETTING_0)
    {
        EngAcgCap2 = DMABCNT0L | (DMABCNT0H << 8);
        // turn on amp if there is ISO data stream out        
        if (EngAcgCap2 % DEV_NUM_BYTE_PER_SAMPLE )
    	{
    		// Alligning the UBM and DMA pointers
    		DMACTL0 = 0; // non-integral, so reset pointers
    		DMACTL0 = 0x81;
    	}

    }

}

/*
大多数情况下，以下程序即可稳定运行：

	if (BufSize > (OUTEP1_RSIZE /4 ) ) 
	{
			INEP2_X[0]=0x33;	INEP2_X[1]=0x03;	INEP2_X[2]=0x0b;//吃饱了撑的
	}
	else
	{
			INEP2_X[0]=0x66;	INEP2_X[1]=0x06;	INEP2_X[2]=0x0b;//44.100	
	}

Rush注释。

*/


