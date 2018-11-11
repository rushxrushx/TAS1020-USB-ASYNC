//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*================================================== 
Softpll.c: routines to handle ACG soft PLL
//================================================*/
#include <Reg52.h>
#include "..\rom\types.h"
#include "..\rom\Reg_stc1.h"
#include "..\rom\usbdfu.h"
#include "devDesc.h"
#include "device.h"
#include "Softpll.h"

word EngAcgCap1, EngAcgCap2;
unsigned char fbCount;

/*====================================================================
softPllInit(): USB ACG initialization
=====================================================================*/
void softPllInit()   
{
	fbCount=3;
	EngAcgCap1 = 0;
	EngAcgCap2 = 0;

	// PLL1 设置22.5792MHZ 
	ACGFRQ2 = 0x6A;
	ACGFRQ1 = 0x4B;
	ACGFRQ0 = 0x20;
//#define _USE_PLL
#ifdef _USE_PLL
    //穷逼买不起晶振，使用片内PLL时钟                             
	ACGDCTL   = 0x10;//PLL1时钟除以2   (22.5792/2==11.2896MHZ)
	ACGCTL  = 0x44;//MCK1选择使用PLL1 开启分频器
#else
	//使用外部11.2896MHZ输入
	ACGDCTL   = 0x10;//不分频
	ACGCTL  = 0x4C;//开启分频器,MCLK1选择外部MCLKIN输入
#endif

}

/*====================================================================
softPll(): ISR for Start of Frame which is used for 
the synchronous mode
=====================================================================*/


#include "..\ROM\Mmap.h"
#include "Devmap.h"
#include "..\ROM\hwMacro.h"

unsigned long MlckSum;
	unsigned char nInt;
	unsigned long nFrac;
	unsigned long fbvalue; 

void softPll()
{  
    volatile unsigned int MclkPerMs;  //用1ms时钟usb SOF测量DA主时钟
	
	EngAcgCap2=EngAcgCap1;	//上次时钟捕获值
	LOW(EngAcgCap1)  = ACGCAPL;	  //这次新的数值
	HIGH(EngAcgCap1) = ACGCAPH;
	
    MclkPerMs = EngAcgCap1 - EngAcgCap2; //相减就是测量到的MCLK时钟频率(1ms单位)，例如：11289

//debug test for capture counter malfunction    	
MclkPerMs = 11290;

if (fbCount==0) {  MlckSum=MlckSum+MclkPerMs;	MclkPerMs=MlckSum/4;
			
	// 11289*1000/256 ~= 44100  (44.100Khz)  分成整数部分44 小数部分100
	// 反馈量10.10格式=  44 << 14 + 100 << 4
	
	nInt=MclkPerMs/256;//整数部分
	
	nFrac=(MclkPerMs - nInt*256)*1000/256;//小数部分
	
	//fbvalue=(nInt <<14) + (nFrac<<4);	
	fbvalue= nInt;
	fbvalue= fbvalue << 14;

	nFrac=nFrac << 4;
	fbvalue= nFrac | fbvalue;
	
	INEP2_X[0]=fbvalue & 0xff;
	INEP2_X[1]=(fbvalue>>8) & 0xff;
	INEP2_X[2]=(fbvalue>>16) & 0xff;


}
else if (fbCount==3) MlckSum=MclkPerMs;
else if (fbCount==2) MlckSum=MlckSum+MclkPerMs;
else if (fbCount==1) MlckSum=MlckSum+MclkPerMs;

//debug test for IN-EP 	
//		INEP2_X[0]=0x66;	INEP2_X[1]=0x06;	INEP2_X[2]=0x0b;
 
//	IEPDCNTX2 = 3;IEPDCNTY2 = 3;	

	if (fbCount==0)
	{
		fbCount=3;  
		IEPDCNTX2 = 3;IEPDCNTY2 = 3; 	// X data count =3	
	}
	else
	{
		fbCount--;

	} 


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
