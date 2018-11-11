//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*================================================== 
Codec.c: routines for prgramming the AC 97 Codec
Note: Data values are provided by the device record
//================================================*/
#include <Reg52.h>
#include "..\ROM\reg_stc1.h"
#include "..\ROM\hwMacro.h"
#include "delay.h"
#include "device.h"
#include "Codec.h"
#include "codec97.h"
#include "Softpll.h"

// flag set when Codec is initialized
bit CodecInited;
//byte CodecResetRegL;
//byte CodecResetRegH;

/*===================================================================
coInitCodec(): initialize the codec
===================================================================*/
void coInitCodec(bit bInitCport)
{
    // return;
    // Configure the C-port   
    if (!CodecInited) 
    {
        if (bInitCport)
        {
//	    CPTCTL = 0x00;	//控制外部CRESET引脚=0

		softPllInit();
//        coSetSpkFreq();
//        coSetMicFreq();
        
	    GLOBCTL &= 0xFE;    // C-port is disabled 
	
		CPTCNF1 = 0x0C;     // I2S mode4
	    CPTCNF2 = 0xE5;     // 32-bit per time slot //24bit data  
	    CPTCNF3 = 0xAC;	    // byte inverse & 1clk delay
	    CPTCNF4 = 0x03;        
	
		// configure DMA 
		DMACTL0 = 0x01;     // Disable Out EP1 to C-port 
		DMATSH0  = 0x80;     // 3 BYTEs per time slot //24bit 
		DMATSL0  = 0x03;     // Slots 0&1
		DMACTL0  = 0x01;     // Out EP1 to C-port 
		DMACTL0 |= 0x80;     // enable OEP1
             
        GLOBCTL |= 0x01;    // C-port is enabled

//	    CPTCTL = 0x01;	//控制外部CRESET引脚=1

        }                
	CodecInited = 1;
    }
}

/*===================================================================
coSetSpkFreq(): Change sampling frequency for the speaker
===================================================================*/
void coSetSpkFreq()
{
    // Write to all DAC regsiter sample rate
//    CPTDATH = HIGH(AppDevice.spk.freq); 	
//    CPTDATL = LOW(AppDevice.spk.freq);
//    CPTADR = CO97_XAUDREG_PCMFRONTDAC_RATE;  while ((CPTSTA & 0x20) == 0);
    
}

/*===================================================================
coPowerDown(): Power down Codec
===================================================================*/
void coPowerDown()
{
    // PR5, PR1, PR2, PR3 = 1

}

/*===================================================================
coColdReset(): Cold reset Codec
===================================================================*/
void coColdReset()
{   
    CodecInited = 0;
    coInitCodec(FALSE);
}
