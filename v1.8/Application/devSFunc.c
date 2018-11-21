//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*================================================== 
Devsfunc.c: routines to handle and update device 
record based on requests from host
//================================================*/
#define _DEVICE_
#include "..\ROM\types.h"
#include <Reg52.h>
#include "..\ROM\reg_stc1.h"
#include "..\ROM\RomBoot.h"
#include "..\ROM\Mmap.h"
#include "..\ROM\UsbEng.h"
#include "..\ROM\usb.h"
#include "..\ROM\usbAudio.h"
#include "..\ROM\usbHid.h"
#include "..\ROM\usbDfu.h"
#include "..\ROM\xdata.h"
#include "..\ROM\devref.h"
#include "..\ROM\RomFunc.h"
#include "..\ROM\hwMacro.h"
#include "devmap.h"
#include "device.h"
#include "delay.h"
#include "device.h"
#include "devRCode.h"
#include "Codec.h"
#include "DevFuncMacro.h"
#include "Softpll.h"

extern void devSoftPll();


#ifdef _ADD_DFU_INTERFACE
// For DFU 
DFU_STATUS_STRUCT DfuStatusData;
#endif

// proto
void devUserIntHandler();
void devGetStrDesc();
void devSetIf();
void devGetEpClass();
void devGetIfClass();
void devGetEpStatus();
void devGetIf();
void devSetEpFeature();
void devSetConfig();
void devClearEpFeature();

void devSetMute();
void devSetVol();
void devSetFreq();

/*-------------------------------------------------------------------
DevFunctionEntryParser(): Device function entry parser
-------------------------------------------------------------------*/
void DevFunctionEntryParser(byte Cmd, void xdata *Data)
{

#ifdef _ADD_DFU_INTERFACE  
    word temp;
#endif
    byte cmd;

    Data = Data;
    
#ifdef _USING_INDIRECT_POINTER_
    AppDevXternaldata = (XDATA_STRUCT xdata *)Data;
#endif

    cmd = Cmd;    
    RET_DATA_BYTE = EVENT_OK;
    RET_DATA_PTR = 0;
    switch(cmd)
    {
    ////////////////// USED BY USBENG.C /////////////////////////
    case DEV_USERINTHANDLER:
        devUserIntHandler();
        break;
        
    ////////////////// USED BY USB.C /////////////////////////
    case DEV_GETIFCLASS:
        devGetIfClass();
        break;
        
    case DEV_GETEPCLASS:
        devGetEpClass();
        break;
        
    //	case DEV_USBVENDORHANDLER:
    //	  break;
        
    case DEV_GETCONFIG:
        RET_DATA_PTR = (byte *)&AppDevice.configSetting;
        break;
        
    case DEV_GETDEVICESTATUS:
        RET_DATA_PTR = (byte *)&AppDevice.devStatus;
        break;
        
    case DEV_GETIFSTATUS:
        RET_DATA_PTR = (byte *)&AppDevice.dummy;
        break;
        
    case DEV_GETEPSTATUS:
        devGetEpStatus();
        break;
        
    case DEV_GETDEVDESC:
        // Save 
        UsbAddress = USBFADR;

        PARAMS_GETDEVDESC_SIZE = SIZEOF_DEVICE_DESC;
        RET_DATA_PTR = AppDevDesc;	

        break;
        
    case DEV_GETCONFIGDESC:
        // Application is running in DFU mode
        RET_DATA_PTR = AppConfigDesc;
        PARAMS_GETCONFIGDESC_SIZE = DEV_CONFIG_BLOCK_SIZE;	  

        break;
        
    case DEV_GETHIDDESC:
        if (PARAMS_GETHIDDESC_IFID == DEV_HID_IF_ID)
        {
            PARAMS_GETHIDDESC_SIZE = DEV_SIZEOF_HID_DESC;
            RET_DATA_PTR = AppDevHidDesc;
        }
        break;
        
    case DEV_GETHIDREPORTDESC:
        if (PARAMS_GETHIDDESC_IFID == DEV_HID_IF_ID)
        {
            PARAMS_GETHIDREPORTDESC_SIZE = DEV_SIZEOF_HID_REPORT_DESC;
            RET_DATA_PTR = AppDevHidReportDesc;
        }
        break;
        
    case DEV_GETSTRDESC:
        devGetStrDesc();
        break;
        
    case DEV_GETIF:
        devGetIf();
        break;
        
    // Set requests
    case DEV_SETREMOTEWAKEUP:
        AppDevice.devStatus |= DEV_STATUS_REMOTE_WAKEUP;
        break;
        
    case DEV_SETEPFEATURE:
        devSetEpFeature();
        break;
        
    case DEV_SETCONFIG:
        devSetConfig();
        break;
        
    case DEV_SETIF:
        devSetIf();
        break;
        
    case DEV_CLRREMOTEWAKEUP:
        AppDevice.devStatus &= ~DEV_STATUS_REMOTE_WAKEUP;
        break;
        
    case DEV_CLEAREPFEATURE:
        devClearEpFeature();
        break;
        
    ////////////////// USED BY USBAUDIO.C /////////////////////////
    // SET REQUESTS
        
    // As there is only master mute and no specific channel
    // So set mute and set mute all are the same
    case DEV_SETMUTEALL:
    case DEV_SETMUTE:
        devSetMute();
        break;
        
    case DEV_SETVOL:
        devSetVol();
        break;

    case DEV_SETVOLALL:           
        break;

    case DEV_SETFREQ:
        devSetFreq();
        break;
        
    // GET REQUESTS
    case DEV_GETUNITIDTYPE:
        switch (PARAMS_GETUNITIDTYPE_UNITID)
        {																		
        case DEV_SPK_FU_ID: 			
            RET_DATA_BYTE = AUD_SUBTYPE_FU;					
            break;															
			
        default:															
            RET_DATA_BYTE = AUD_SUBTYPE_UNDEF; 			
            break;															
        }	
        break;
        
        
    case DEV_GETMUTE:
        switch (PARAMS_GETMUTE_UNITID)
        {
        case DEV_SPK_FU_ID:
            AppDevice.dummy = AppDeviceSpkcurMute;
            break;
        default:
            RET_DATA_BYTE = EVENT_ERROR;
            break;
        }
        if (RET_DATA_BYTE == EVENT_OK)
        {
            AppDevice.dummy = AppDevice.dummy << 8;
            RET_DATA_PTR = (byte *)&AppDevice.dummy;
        }
        break;
        
    case DEV_GETVOL:
		break;
 
   case DEV_GETFREQ:
        break;
                                    
    ////////////////// USED BY USBHID.C /////////////////////////
    case DEV_HIDGETREPORT:
        switch(PARAMS_HIDGETREPORT_IFID)
        {
        case DEV_HID_IF_ID:
            AppDevice.dummy = AppDevice.hidParms.reportItem << 8;
            RET_DATA_PTR = (byte *)&AppDevice.dummy;
            break;
        default:
            RET_DATA_BYTE = EVENT_ERROR;
            break;
        }
        break;
    
    case DEV_HIDSETREPORT:
        AppDevice.hidParms.reportItem = USB_EP0_XFERDATA[0];
        break;
        
    case DEV_HIDGETIDLE:
    case DEV_HIDGETPROTO:
        break;
        
    case DEV_HIDSETIDLE:
    case DEV_HIDSETPROTO:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
       
        // These need to redefine
    case DEV_USERSUSPENSE:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
        
    default:
        break;
    }
}

/*===================================================================
devGetEpClass(): Get the class type based on the endpoint id
===================================================================*/
void devGetEpClass()
{
    switch(PARAMS_GETEPCLASS_EPID)
    {
    case DEV_HID_EP_ID:
        RET_DATA_WORD = USB_HID_CLASS;
        break;

    case DEV_SPK_EP_ID:
        RET_DATA_WORD = USB_AUD_CLASS;
        break;
    default:
        RET_DATA_WORD = USB_UNKNOWN_CLASS;
        break;
    }
}

/*===================================================================
devGetIfClass(): Get the class type based on the interface id
===================================================================*/
void devGetIfClass()
{
    // Check interface ID
    switch(PARAMS_GETIFCLASS_IFID)
    {
    case DEV_HID_IF_ID:
        RET_DATA_WORD = USB_HID_CLASS;
        break;
    case DEV_AUD_CTLR_IF_ID:
        RET_DATA_WORD = USB_AUD_CLASS;
        break;

    default:
        RET_DATA_WORD = USB_UNKNOWN_CLASS;
        break;
    }
}

/*===================================================================
devSetIf(): Set alternating Interface setting
===================================================================*/
void devSetIf()
{
    switch (PARAMS_SETIF_IFID)
    {
    case DEV_SPK_IF_ID: 
        if ((PARAMS_SETIF_SETTING == DEV_SPK_SETTING_0) ||
            (PARAMS_SETIF_SETTING == DEV_SPK_SETTING_1))
        {
            AppDevice.spkIf.curSetting = PARAMS_SETIF_SETTING;
            if (AppDevice.spkIf.curSetting == DEV_SPK_SETTING_0)//alt0:zero bandwidth 0带宽播放停止
            {
				DMACTL0 = 0x01;     // Disable Out EP1 to C-port 
            }
            else //alt1:Play 正常播放
            {
                DMACTL0 = 0x81;     // Out EP1 to C-port
                softPllInit();
                USBIMSK |= 0x10;    // SOF INT on 
            }
        }
        else
            RET_DATA_BYTE = EVENT_ERROR;
        break;


    case DEV_AUD_CTLR_IF_ID:
    case DEV_HID_IF_ID:	
        if (PARAMS_SETIF_SETTING != 0)
            RET_DATA_BYTE = EVENT_ERROR;
        break;
        
    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
    
    return;
}

/*===================================================================
devUserIntHandler(): User interrupt handler
===================================================================*/
void devUserIntHandler()
{
    switch(PARAMS_USERINTHANDLER_INTVECTOR)
    {
    // Start of frame
    case PSOF_INT:
        AppResetFlag1 = FALSE;
		break;

    case SOF_INT:
        AppResetFlag1 = FALSE;
        if (AppDevice.configSetting)
        {
            softPll();
            devCtlAmpPower(TRUE);
        }
        break;
 
    // Suspense       
    case SUSR_INT:
        AppSuspendFlag = TRUE;
        break;
        
    // Resume
    case RESR_INT:
		AppSuspendFlag = FALSE; 
        if (AppSleepModeOn == TRUE)
        {
            // turn external interrupt off
            // in case no remote wake up from device 
            devTurnOffRemoteWakeUp();  
            devSleepModeOff();
            AppSleepModeOn = FALSE;
        }

        // Check RESET Status bit.       
        // this must be the readl USB RESET
        if (USBSTA & 0x80)
        {
            USBFADR = UsbAddress = 0;
            AppDevice.configSetting = 0;
        }
        break;
     
    // Reset
    case RSTR_INT:
#ifdef _815E_FIX
        USBFADR = UsbAddress; 
        if (AppResetFlag == TRUE)
            AppResetFlag = FALSE;
        else
            AppResetFlag = TRUE;
#else

        AppResetFlag = TRUE;
#endif
        break;
        
    // External interrupt
    case XINT_INT:
        // Check if Remote set
        if (AppSuspendFlag && AppDevice.devStatus & DEV_STATUS_REMOTE_WAKEUP)
        {
            USBCTL |= 0x20;
            USBCTL &= ~0x20;
            // turn external interrupt off
            P3MSK = 0xFF;
            GLOBCTL &= ~0x40;
        }
        // Clocks already started..
        break; 
        
    default:
        break;
    }
}

/*===================================================================
devGetStrDesc(): Get string descriptor using string index
===================================================================*/
void devGetStrDesc()
{
    switch(PARAMS_GETSTRDESC_STRID)
    {
    case DEV_LANG_STR_IDX:
        PARAMS_GETSTRDESC_SIZE = DEV_LANGID_SIZE;
        RET_DATA_PTR = AppDevLanguageIDDesc;
        break;
        
    case DEV_COMPANY_STR_IDX:
        PARAMS_GETSTRDESC_SIZE = DEV_COMPANY_STR_SIZE;
        RET_DATA_PTR = AppDevCompanyStringDesc;
        break;
        
    case DEV_PROD_STR_IDX:
        PARAMS_GETSTRDESC_SIZE = DEV_PROD_STR_SIZE;
        RET_DATA_PTR = AppDevProductStringDesc;
        break;
        
        
        
    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
}

/*===================================================================
devGetEpStatus(): Get endpoint status
===================================================================*/
void devGetEpStatus()
{
    switch (PARAMS_GETEPSTATUS_EPID)
    {
    case DEV_AUD_CTLR_EP_ID:
        RET_DATA_PTR = (byte *)&AppDevice.audCtlrIf.epState;
        break;
    case DEV_SPK_EP_ID:
        RET_DATA_PTR = (byte *)&AppDevice.spkIf.epState;
        break;
    case DEV_FB_EP_ID://这里也要加上
        RET_DATA_PTR = (byte *)&AppDevice.spkIf.epState;
        break;
    case DEV_HID_EP_ID:
        RET_DATA_PTR = (byte *)&AppDevice.hidIf.epState;
        break;
    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
}

/*===================================================================
devGetIf(): Get interface setting
===================================================================*/
void devGetIf()
{
    switch(PARAMS_GETIF_IFID)
    {
    case DEV_AUD_CTLR_IF_ID:
        RET_DATA_PTR = (byte *)&AppDevice.audCtlrIf.curSetting;
        break;
    case DEV_SPK_IF_ID:
        RET_DATA_PTR = (byte *)&AppDevice.spkIf.curSetting;
        break;

    case DEV_HID_IF_ID:
        RET_DATA_PTR = (byte *)&AppDevice.hidIf.curSetting;
        break;
    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
}

/*===================================================================
devSetEpFeature(): Set endpoint feature
===================================================================*/
void devSetEpFeature()
{
    switch(PARAMS_SETEPFEATURE_EPID)
    {	
    case DEV_AUD_CTLR_EP_ID:
        AppDevice.audCtlrIf.epState |= DEV_EP_STATE_HALT;
        break;

    case DEV_FB_EP_ID://临时处理
    case DEV_SPK_EP_ID:
        AppDevice.spkIf.epState |= DEV_EP_STATE_HALT;
        STALLSpkEp;
        break;

    case DEV_HID_EP_ID:
        AppDevice.hidIf.epState |= DEV_EP_STATE_HALT;
        STALLHidEp;
        break;
    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
}

/*===================================================================
devSetConfig(): Set configuration
===================================================================*/
void devSetConfig()
{
    AppDevice.configSetting = PARAMS_SETCONFIG_CONFIGID;
 
    if (AppDevice.configSetting == 0)
    {
        AppDevice.spkIf.curSetting = DEV_SPK_SETTING_0;
        AppDevice.spkIf.epState |= DEV_EP_STATE_HALT;
        AppDevice.hidIf.epState |= DEV_EP_STATE_HALT;
        STALLHidEp;
        STALLSpkEp; 
    }
    else
    {		
        devInitSTC();
        coInitCodec(TRUE);	  
        AppDevice.spkIf.epState &= ~DEV_EP_STATE_HALT;
        AppDevice.hidIf.epState &= ~DEV_EP_STATE_HALT;
        STALLClrSpkEp;
        STALLClrHidEp;
    }

}

/*===================================================================
devClearEpFeature(): Clear endpoint feature
===================================================================*/
void devClearEpFeature()
{
    switch(PARAMS_CLEAREPFEATURE_EPID)
    {
    case DEV_AUD_CTLR_EP_ID:
        AppDevice.audCtlrIf.epState &= ~DEV_EP_STATE_HALT;
        break;

    case DEV_FB_EP_ID:///暂时先这么处理
    case DEV_SPK_EP_ID:
        AppDevice.spkIf.epState &= ~DEV_EP_STATE_HALT;
        STALLClrSpkEp;
        break;

    case DEV_HID_EP_ID:
        AppDevice.hidIf.epState &= ~DEV_EP_STATE_HALT;
        STALLClrHidEp;
        break;
    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
}


/*===================================================================
devSetMute(): Set Mute
===================================================================*/
void devSetMute()
{
    switch (PARAMS_SETMUTE_UNITID)
    {
    case DEV_SPK_FU_ID:
        AppDeviceSpkcurMute = USB_EP0_XFERDATA[0] & DEV_SPK_MSK_MUTE;
        break;

    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
}

/*===================================================================
devSetVol(): Set Volume
===================================================================*/
void devSetVol()
{
    switch (PARAMS_SETVOL_UNITID)														
    {
    case DEV_SPK_FU_ID:
        AppDevice.spk.curVol[PARAMS_SETVOL_CN - 1] = 
               MAKE_BIG_ENDIAN(*(word *)&USB_EP0_XFERDATA[0]);
        break;

    default:
        RET_DATA_BYTE = EVENT_ERROR;
        break;
    }
}

/*===================================================================
devSetFreq(): Set smapling frequency rate
===================================================================*/
void devSetFreq()
{
    if (
        ((USB_EP0_XFERDATA[1] == 0xAC) && (USB_EP0_XFERDATA[0] == 0x44)) ||
        ((USB_EP0_XFERDATA[1] == 0xBB) && (USB_EP0_XFERDATA[0] == 0x80))) 
    {
        switch (PARAMS_SETFREQ_EPID)
        {
        case DEV_SPK_EP_ID:
            AppDevice.spk.freq = (USB_EP0_XFERDATA[1] << 8) | 
                USB_EP0_XFERDATA[0];
//            delay(5);
//			coSetSpkFreq();
            break;

        default:
            RET_DATA_BYTE = EVENT_ERROR;
			break;
        }
    }
}
