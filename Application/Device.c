//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*================================================== 
Device.c: routines to handle device record
//================================================*/
#define _DEVICE_
#include "..\ROM\types.h"
#include <Reg52.h>
#include "..\ROM\reg_stc1.h"
#include "..\ROM\Mmap.h"
#include "..\ROM\UsbEng.h"
#include "..\ROM\usb.h"
#include "..\ROM\RomBoot.h"
#include "..\ROM\usbAudio.h"
#include "..\ROM\usbHid.h"
#include "..\ROM\usbDfu.h"

#include "..\ROM\xdata.h"
#include "..\ROM\devref.h"
#include "..\ROM\ROmFunc.h"
#include "..\ROM\hwMacro.h"
#include "devRCode.h"
#include "Devmap.h"
#include "delay.h"
#include "device.h"
#include "Codec.h"
#include "Buttons.h"
#include "DevFuncMacro.h"
#include "Softpll.h"

// Device record
DEV_DEVICE_STRUCT AppDevice;
byte UsbAddress;
word DevAmpCounter;

// this is defined to save RAM space
bit AppDeviceSpkcurMute;
bit AppDeviceSpkpreMute;
bit volatile AppSleepModeOn;

// Reset Bit flag for App in App running
bit volatile AppResetFlag;
bit volatile AppResetFlag1;

// Suspense Bit flag for App
bit volatile AppSuspendFlag;

// for testing DFU DnLoad/Upload when target is DFU_TARGET_OTHER
#ifdef _ADD_DFU_TARGET_OTHER_
byte *DevTestDfuDataPtr;
#endif

// For hardware guys to debug
#define RAM(ram_addr) (*((unsigned char xdata *) ram_addr))

// xdata byte TestEp0XferData _at_ INPACK_ADDR;

// time waiting before turn on the AMP after turn on the DMA out
// This value equals the number of SOF interrupts or milisecs
#define DEV_DMA_AMP_WAIT_TIME   2000    

////////////////// ///////// DEVICE RECORD	/////////////////////////

/*-------------------------------------------------------------------
main(): This main is for device including the audio/hid/dfu
-------------------------------------------------------------------*/
void main()
{
unsigned char jj;
    IE = 0;
	CoAc97ModePin0 = 0;
 
    // Give the XRAM time to powered up as the MCU of the emulator is faster
    delay(80);	
	GLOBCTL |= 0x04;
	delay(10);


	DEV_TURN_OFF_AMP;

    // ROM code using PDATA starting at 0xFA00
	P2 = 0xFA;
	RomRecord.state  = ROM_APP_RUNNING;	
	// Just a dummy code to force the compiler to keep 
	// DevFunctionEntryParser() in the final codes
	AppDevice.dummy = 0;
	if(AppDevice.dummy)
		DevFunctionEntryParser(0, 0);
	
	devInit();
	softPllInit();	

	// using ROM DFU handler
	devRomFunction(ROM_INIT_DFU_STATE);
		
	// Take Codec out of reset
	CPTCTL = 0x01;
 
    // Turn external interrupt off
    devTurnOffRemoteWakeUp();

    // Reinitialize flags
    AppResetFlag1 = AppSleepModeOn = 
	AppSuspendFlag = AppResetFlag = FALSE;

    // Setup USB engine for connection
	USBENGINE_EP0_SIZE = DEV_MAX_EP0_PKT;	
	devRomFunction(ROM_ENG_USB_INIT);

    // Turn on SOF ISR
	USBIMSK |= 0x10;
	
	// wait till codec inited
    DevAmpCounter = 1500;
	while (AppDevice.configSetting == 0)
       devCheckReset();
    DevAmpCounter = 0;

    AppResetFlag1 = AppSleepModeOn = 
	AppSuspendFlag = AppResetFlag = FALSE;
	
    // wait for the host to settle down if needed
	while(1)
	{
	jj++;
	if (jj==0) 	CoAc97ModePin1 = !CoAc97ModePin1;

        // Check for USB Reset
        devCheckReset();
//		butChkUpdate();//与反馈端点冲突了
		devUpdate();

#ifdef _ADD_DFU_INTERFACE
		// For DFU mode
		if (RomRecord.state == ROM_DFU_MODE)
		{
            // Setup for reenumeration
            // as device in DFU mode
			IE = 0;
			USBCTL = 0;
			USBFADR = 0;
			PARAMS_DFU_SETUP(DFU_TARGET_EEPROM, 0);
			// PARAMS_DFU_SETUP(DFU_TARGET_RAM, 0);
			devRomFunction(ROM_RUN_DFU_MODE);
		}
#endif
		
        // Handle SUSPENSE/RESUME

        if ((AppSuspendFlag == TRUE) && (AppResetFlag == FALSE) && (USBFADR != 0))
        {         
            devSleepModeOn();
            AppSleepModeOn = TRUE;

            // For fremote wake up
            devSetRemoteWakeUp();
    
            PCON = 0x09;

        while (AppSleepModeOn ==  TRUE);
        devCheckReset();
        AppSuspendFlag = FALSE;

		// Delay a bit (approx. 5 ms) to account for button debounce if it was used to 
		// do a remote wake up. 

        delay(20);

        }
	};
}


////////////////// ///////// DEVICE RECORD	/////////////////////////
/*-------------------------------------------------------------------
devInit(): initialize device
-------------------------------------------------------------------*/
void devInit()
{
//	DEV_TURN_OFF_AMP;
	CodecInited = 0; 
	devInitRec();
}

/*-------------------------------------------------------------------
devInitRec(): initialize device record
-------------------------------------------------------------------*/
void devInitRec()
{ 
	// Init device
#ifdef SELFPOWER_REMOTE
	AppDevice.devStatus = DEV_STATUS_SELFPOWER;
#else
#ifdef SELFPOWER_NOREMOTE
	AppDevice.devStatus = DEV_STATUS_SELFPOWER;
#else
	AppDevice.devStatus = 0;
#endif
#endif
	AppResetFlag = AppSuspendFlag = FALSE;
    AppDevice.dummy = 0;

    // Init configuration
    AppDevice.configSetting = 0;

    // Init audio control interface
    AppDevice.audCtlrIf.curSetting = 0;
    AppDevice.audCtlrIf.epState = DEV_EP_STATE_HALT;

    // Init speaker parameters
    AppDeviceSpkcurMute = DEV_SPK_MUTE_OFF;
    AppDeviceSpkpreMute = DEV_SPK_MUTE_ON; 
    AppDevice.spk.curVol[DEV_SPK_LEFT_CN] = 
    AppDevice.spk.curVol[DEV_SPK_RIGHT_CN] = DEV_SPK_CUR_VOL;
    AppDevice.spk.preVol[DEV_SPK_LEFT_CN] =
    AppDevice.spk.preVol[DEV_SPK_RIGHT_CN] = DEV_SPK_MIN_VOL;
    AppDevice.spk.curBass = DEV_SPK_CUR_BASS;
    AppDevice.spk.preBass = DEV_SPK_MIN_BASS;
    AppDevice.spk.curTreble = DEV_SPK_CUR_TREBLE;
    AppDevice.spk.preTreble = DEV_SPK_MIN_TREBLE;


    // Set current interface alternate setting
    AppDevice.spkIf.curSetting =
//    AppDevice.micRecIf.curSetting =
    AppDevice.hidIf.curSetting =
    AppDevice.dfuIf.curSetting = 0;

    // Initialize endpoint states
    AppDevice.spkIf.epState = 
//    AppDevice.micRecIf.epState = 
    AppDevice.hidIf.epState = DEV_EP_STATE_HALT;

    AppDevice.hidParms.reportItem	= 0x00;

    // DFU interface use control endpoint

    // Sampling frequency
    AppDevice.spk.freq = 0xAC44; 	
//    AppDevice.micRec.freq = 0xAC44; 
}

/*-------------------------------------------------------------------
devInitSTC(): Initilize STC for the specific device operations
-------------------------------------------------------------------*/
void devInitSTC()//端点初始化 
{
	// Disable all the Endpoints 
	IEPCNF1 = 0;
	IEPCNF4 = 0;
	IEPCNF5 = 0;
	IEPCNF6 = 0;
	IEPCNF7 = 0;
	OEPCNF2 = 0;
	OEPCNF3 = 0;
	OEPCNF4 = 0;
	OEPCNF5 = 0;
	OEPCNF6 = 0;
	OEPCNF7 = 0;

	
	// ISO OUT EP # 1 
#ifdef  _APP_24_BIT_
	OEPCNF1 = 0xC5; 		// Enable the ISO OUT EP, 6 BYTEs/sample
#else
    OEPCNF1 = 0xC3; 		// Enable the ISO OUT EP, 4 BYTEs/sample 
#endif
	OEPBBAX1 = OUTEP1_XOFFSET;
	OEPBSIZ1 = OUTEP1_SIZE;
	OEPDCNTX1 = 0x00; 	// Clear X data count
	OEPBBAY1  = 0;		// Y buffer is not used
	OEPDCNTY1 = 0x00;   // Clear Y data count

	// ISO IN EP # 0x82 ,Async Feedback 异步反馈端点
	IEPCNF2 = 0xC0; 	// Enable, ISO-IN EP, 1 BYTEs/sample
	IEPBBAX2 = INEP2_XOFFSET;
	IEPBSIZ2 = INEP2_SIZE; //1*8bytes
	IEPDCNTX2 = 0x00; 	// Clear X data count
	IEPBBAY2 =  INEP2_XOFFSET;		// Y buffer
	IEPDCNTY2 = 0x00;   // Clear Y data count
	
	// Interrupt Endpoint for HID
	IEPCNF3 = 0x80; 	// No Interrupt for MCU
	IEPDCNTX3 = 0x80;
	IEPBBAX3 = INEP3_XOFFSET ;
	IEPBSIZ3 = INEP3_SIZE;	// buffer size 8 BYTEs 
}

/*-------------------------------------------------------------------
devUpdate(): Check device audio paramters and update Codec
Return: TRUE if codec update, else FALSE 
-------------------------------------------------------------------*/
void devUpdate()
{
	bit status;
    bit changeStatus;
	
	status = changeStatus = FALSE;
    
    // check speaker
    if (AppDevice.spk.curVol[DEV_SPK_LEFT_CN] != AppDevice.spk.preVol[DEV_SPK_LEFT_CN])
    {
        status = TRUE;
        if (AppDevice.spk.curVol[DEV_SPK_LEFT_CN] < AppDevice.spk.preVol[DEV_SPK_LEFT_CN])
        {
            AppDevice.spk.preVol[DEV_SPK_LEFT_CN] -= DEV_SPK_RES_VOL;
            if ((AppDevice.spk.preVol[DEV_SPK_LEFT_CN] < DEV_SPK_MIN_VOL) ||
                (AppDevice.spk.curVol[DEV_SPK_LEFT_CN] > AppDevice.spk.preVol[DEV_SPK_LEFT_CN]))
                AppDevice.spk.preVol[DEV_SPK_LEFT_CN] = AppDevice.spk.curVol[DEV_SPK_LEFT_CN];             
        }
        else 
        {
            AppDevice.spk.preVol[DEV_SPK_LEFT_CN] += DEV_SPK_RES_VOL;
            if (AppDevice.spk.curVol[DEV_SPK_LEFT_CN] < AppDevice.spk.preVol[DEV_SPK_LEFT_CN])
                AppDevice.spk.preVol[DEV_SPK_LEFT_CN] = AppDevice.spk.curVol[DEV_SPK_LEFT_CN];
        }
    }
    
    if (AppDevice.spk.curVol[DEV_SPK_RIGHT_CN] != AppDevice.spk.preVol[DEV_SPK_RIGHT_CN])
    {
        status = TRUE;
        if (AppDevice.spk.curVol[DEV_SPK_RIGHT_CN] < AppDevice.spk.preVol[DEV_SPK_RIGHT_CN])
        {
            AppDevice.spk.preVol[DEV_SPK_RIGHT_CN] -= DEV_SPK_RES_VOL;
            if ((AppDevice.spk.preVol[DEV_SPK_RIGHT_CN] < DEV_SPK_MIN_VOL) ||
                (AppDevice.spk.curVol[DEV_SPK_RIGHT_CN] > AppDevice.spk.preVol[DEV_SPK_RIGHT_CN]))
                AppDevice.spk.preVol[DEV_SPK_RIGHT_CN] = AppDevice.spk.curVol[DEV_SPK_RIGHT_CN];
        }
        else 
        {
            AppDevice.spk.preVol[DEV_SPK_RIGHT_CN] += DEV_SPK_RES_VOL;
            if (AppDevice.spk.curVol[DEV_SPK_RIGHT_CN] < AppDevice.spk.preVol[DEV_SPK_RIGHT_CN])
                AppDevice.spk.preVol[DEV_SPK_RIGHT_CN] = AppDevice.spk.curVol[DEV_SPK_RIGHT_CN];
        }
    }
    
    if (AppDeviceSpkcurMute != AppDeviceSpkpreMute)
    {
        status = TRUE;
        AppDeviceSpkpreMute = AppDeviceSpkcurMute;
    }
    
    if (status)
    {
        changeStatus = TRUE;
        status = FALSE;
//        coSpkUpdate();
    }

    // check microphone recording
/*    if (AppDevice.micRec.curVol != AppDevice.micRec.preVol)
    {
        status = TRUE;
        if (AppDevice.micRec.curVol < AppDevice.micRec.preVol)
        {
            AppDevice.micRec.preVol -= DEV_MICREC_RES_VOL;
            if ((AppDevice.micRec.preVol < DEV_MICREC_MIN_VOL) ||
                (AppDevice.micRec.curVol > AppDevice.micRec.preVol))
                AppDevice.micRec.preVol = AppDevice.micRec.curVol;
        }
        else 
        {
			AppDevice.micRec.preVol += DEV_MICREC_RES_VOL;
            if (AppDevice.micRec.preVol > AppDevice.micRec.curVol)
                AppDevice.micRec.preVol = AppDevice.micRec.curVol;        
        }
    }
    
    if (AppDeviceMicReccurMute != AppDeviceMicRecpreMute)
    {
        status = TRUE;
        AppDeviceMicRecpreMute = AppDeviceMicReccurMute;
    }
    
    if (status)
    {
        changeStatus = TRUE;
        status = FALSE;
//        coMicRecUpdate();
    }*/

    // check microphone feedback
/*    if (AppDevice.micIn.curVol != AppDevice.micIn.preVol)
    {
        status = TRUE;
        if (AppDevice.micIn.curVol < AppDevice.micIn.preVol)
       {
            AppDevice.micIn.preVol -= DEV_MICIN_RES_VOL;
            if ((AppDevice.micIn.preVol < DEV_MICIN_MIN_VOL) ||
                (AppDevice.micIn.curVol > AppDevice.micIn.preVol))
                AppDevice.micIn.preVol = AppDevice.micIn.curVol;
        }
        else 
        {
			AppDevice.micIn.preVol += DEV_MICIN_RES_VOL;
            if (AppDevice.micIn.preVol > AppDevice.micIn.curVol)
                AppDevice.micIn.preVol = AppDevice.micIn.curVol;        
        }
    }
    
    if (AppDeviceMicIncurMute != AppDeviceMicInpreMute)
    {
        status = TRUE;
        AppDeviceMicInpreMute = AppDeviceMicIncurMute;
    }
    
    if (status)
    {
        changeStatus = TRUE;
        status = FALSE;
//        coMicInUpdate();
    }

    // Check Line in
    if (AppDevice.lineIn.curVol[DEV_LINEIN_RIGHT_CN] != AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN])
    {
        status = TRUE;
        if (AppDevice.lineIn.curVol[DEV_LINEIN_RIGHT_CN] < AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN])
        {
            AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN] -= DEV_LINEIN_RES_VOL;
            if ((AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN] < DEV_LINEIN_MIN_VOL) ||
                (AppDevice.lineIn.curVol[DEV_LINEIN_RIGHT_CN] > AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN]))
                AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN] = AppDevice.lineIn.curVol[DEV_LINEIN_RIGHT_CN];
        }
        else 
        {
            AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN] += DEV_LINEIN_RES_VOL;
            if (AppDevice.lineIn.curVol[DEV_LINEIN_RIGHT_CN] < AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN])
                AppDevice.lineIn.preVol[DEV_LINEIN_RIGHT_CN] = AppDevice.lineIn.curVol[DEV_LINEIN_RIGHT_CN];
        }
    }

    if (AppDevice.lineIn.curVol[DEV_LINEIN_LEFT_CN] != AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN])
    {
        status = TRUE;
        if (AppDevice.lineIn.curVol[DEV_LINEIN_LEFT_CN] < AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN])
        {
            AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN] -= DEV_LINEIN_RES_VOL;
            if ((AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN] < DEV_LINEIN_MIN_VOL) ||
                (AppDevice.lineIn.curVol[DEV_LINEIN_LEFT_CN] > AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN]))
                AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN] = AppDevice.lineIn.curVol[DEV_LINEIN_LEFT_CN];
        }
        else 
        {
            AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN] += DEV_LINEIN_RES_VOL;
            if (AppDevice.lineIn.curVol[DEV_LINEIN_LEFT_CN] < AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN])
                AppDevice.lineIn.preVol[DEV_LINEIN_LEFT_CN] = AppDevice.lineIn.curVol[DEV_LINEIN_LEFT_CN];
        }
    } 
   
    if (AppDeviceLineIncurMute != AppDeviceLineInpreMute)
    {
        status = TRUE;
        AppDeviceLineInpreMute = AppDeviceLineIncurMute;
    }
    
    if (status)
    {
        changeStatus = TRUE;
        status = FALSE;
//        coLineInUpdate();
    }*/
    
    // Any thing changed
    if (changeStatus == TRUE)
    {
        delay(5);
    }
}

/*====================================================================
devSleepModeOff(): Take device hardare out of sleep mode.
=====================================================================*/
void devSleepModeOff ()
{
    // Turn on codec
	CoAc97ModePin0 = 0;
	CoAc97ModePin1 = 0;
	coColdReset();
    DevAmpCounter = 0;

    // turn the power amp on only in normal operation
//    if (AppDevice.spkIf.curSetting == DEV_SPK_SETTING_1)
//    {
//    	DEV_TURN_ON_AMP;
//        DMACTL0 = 0x81;
//    }

    // Turn on miscellaneous hardware

}

/*====================================================================
devSleepModeOn(): Set device hardware to sleep mode, low power mode.
=====================================================================*/
void devSleepModeOn()
{

    // turn off power amp and reset counter
    DevAmpCounter = 0;
    devCtlAmpPower(FALSE);

    // Set codec to sleep mode
   	coPowerDown();
	CoAc97ModePin0 = 1;
	CoAc97ModePin1 = 1;

    // Set miscellaneous hardware for sleep mode
}

/*-------------------------------------------------------------------
devCtlAmpPower(): Check and set Power Amp appropriately to reduce the
pop when turning Amp on.
Method:
- Turning DMA on if it is off
- After the DMA is on, wait for 2 to 3 sec, turn on the Amp. This is 
  because the amp needs time to charge the capacitor
Note:
- If this routine is used, it should be called every 1 ms.
- The waiting time is changed base on the Amp and Codec parts.
-------------------------------------------------------------------*/
void devCtlAmpPower(bit Mode)
{
    if (DevAmpCounter == 0)
    {
        // turn on DMA
        if (Mode == TRUE)
        {
            DMACTL0 = 0x01;
            DMACTL0 = 0x81;
            DevAmpCounter = 1;         
        }
        else
        {
            DMACTL0 = 0x01;
            delay(100);
            DEV_TURN_OFF_AMP;
            DevAmpCounter = DEV_DMA_AMP_WAIT_TIME;        
        }
	}

    // Wait for a certain time before turn on the Amp
    if ((DevAmpCounter < DEV_DMA_AMP_WAIT_TIME) && (DevAmpCounter > 0))
    {
        DevAmpCounter++;
        // Turn on Amp and wait 50 ms before turn on DMA
        if (DevAmpCounter == DEV_DMA_AMP_WAIT_TIME)
            DEV_TURN_ON_AMP; 
    }
}

/*-------------------------------------------------------------------
devSetRemoteWakeUp(): Turn on remote wakeup using GPIO port 3 pins
3, 4, and 5.
-------------------------------------------------------------------*/
void devSetRemoteWakeUp()
{
    GLOBCTL |= 0x40;
    P33WakeButton = 1;
    P34WakeButton = 1;
    P35WakeButton = 1;
    P3MSK = 0x00;
}

/*-------------------------------------------------------------------
devTurnOffRemoteWakeUp(): Turn off remote wakeup for GPIO port 3 pins
3, 4, and 5.
-------------------------------------------------------------------*/
void devTurnOffRemoteWakeUp()
{
    P3MSK = 0xFF;
    GLOBCTL &= ~0x40;
}

/*-------------------------------------------------------------------
devCheckReset(): Check if the USB reset is spurious or real
-------------------------------------------------------------------*/
void devCheckReset()
{
#ifdef _815E_FIX
	if (AppResetFlag == TRUE)
	{ 
        AppResetFlag1 = TRUE;

        // make sure SOF INT on
        // as we monitor for SOF
        USBIMSK |= 0x10;    // SOF INT on
        delay(14);

        if (AppResetFlag1 == TRUE)
        {
            UsbAddress = USBFADR = 0;
            AppDevice.configSetting = 0;
        }
        AppResetFlag = AppResetFlag1 = FALSE;
	}
#else
	AppResetFlag = FALSE;
#endif	
}

