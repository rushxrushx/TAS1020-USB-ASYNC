//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 
/*====================================================================
USBAUDIO.C
Handler for USB audio class
====================================================================*/
#define _USB_AUD_
#include <Reg52.h>
#include "types.h"
#include "Mmap.h"
#include "UsbEng.h"
#include "reg_stc1.h"
#include "usbaudio.h"

// For debug compilation
#include "DevRef.h"

/*====================================================================
audHandler() : Handle USB Audio Class requests
Input:	none
Output: none
Globals used:
UsbRequest
Return: none
Note: Support for the following:
- Feature Unit Control for Volume, Mute, Treble, Bass
- Endpoint Frequency Sampling Rate for 8, 11,025, 16, 22.05
44.1, 48 KHZ.
- Mixer Unit: Fixed input/output gain.
=====================================================================*/
void audHandler()
{
    byte unHandleCase;
    
    unHandleCase = 0;
    
    PARAMS_GETUNITIDTYPE(UsbRequest.hiwIndex);
    DEV_FUNCTION(DEV_GETUNITIDTYPE, &DEV_SHARED_DATA);
    UsbRequest.temp = DEV_DATA_BYTE;
    
    switch (UsbRequest.bmRequest)
    {   
    case AUD_BMREQ_SET_EIDIF: 
        switch (UsbRequest.temp)
        {
        case AUD_SUBTYPE_MIXER:
            PARAMS_SETMIXER(UsbRequest.hiwIndex, UsbRequest.hiwValue, UsbRequest.lowValue, 
                UsbRequest.bRequest, UsbRequest.wLength);
            DEV_FUNCTION(DEV_SETMIXER, &DEV_SHARED_DATA);
            break;
            
            
        case AUD_SUBTYPE_FU:
            // first form
            if (UsbRequest.lowValue != 0xFF)
            {
                switch (UsbRequest.hiwValue)
                {
                case AUD_FU_MUTE_CNTL:
                    PARAMS_SETMUTE(UsbRequest.hiwIndex, UsbRequest.lowValue);
                    DEV_FUNCTION(DEV_SETMUTE, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_VOLUME_CNTL:
                    PARAMS_SETVOL(UsbRequest.hiwIndex, UsbRequest.lowValue);
                    DEV_FUNCTION(DEV_SETVOL, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_BASS_CNTL:
                    PARAMS_SETBASS(UsbRequest.hiwIndex, UsbRequest.lowValue);
                    DEV_FUNCTION(DEV_SETBASS, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_TREBLE_CNTL:
                    PARAMS_SETTREBLE(UsbRequest.hiwIndex, UsbRequest.lowValue);
                    DEV_FUNCTION(DEV_SETTREBLE, &DEV_SHARED_DATA);
                    break;
                    
                default:
                    // Check later and call application to
                    // handle this case
                    unHandleCase = 1;
                    break;
                }
            }
            // second format: set multiple values from all available channels or controls
            else
            {
                switch (UsbRequest.hiwValue)
                {
                case AUD_FU_MUTE_CNTL:
                    PARAMS_SETMUTEALL(UsbRequest.hiwIndex, UsbRequest.wLength);
                    DEV_FUNCTION(DEV_SETMUTEALL, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_VOLUME_CNTL:
                    PARAMS_SETVOLALL(UsbRequest.hiwIndex, UsbRequest.wLength >> 1);
                    DEV_FUNCTION(DEV_SETVOLALL, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_BASS_CNTL:
                    PARAMS_SETBASSALL(UsbRequest.hiwIndex, UsbRequest.wLength);
                    DEV_FUNCTION(DEV_SETBASSALL, &DEV_SHARED_DATA);																																
                    break;
                    
                case AUD_FU_TREBLE_CNTL:
                    PARAMS_SETTREBLEALL(UsbRequest.hiwIndex, UsbRequest.wLength);
                    DEV_FUNCTION(DEV_SETTREBLEALL, &DEV_SHARED_DATA);
                    break;
                    
                default:
                    // Check later and call application to
                    // handle this case
                    unHandleCase = 1;
                    break;
                }
            }
            
            break;
            
        default:
            // Check later and call application to
            // handle this case
            unHandleCase = 1;
            break;
        }
        break;
        
    case AUD_BMREQ_GET_EIDIF: 
        switch (UsbRequest.temp)
        {
        case AUD_SUBTYPE_MIXER:
            
            PARAMS_GETMIXER(UsbRequest.hiwIndex, UsbRequest.hiwValue, UsbRequest.lowValue, 
                UsbRequest.bRequest, UsbRequest.wLength);
            DEV_FUNCTION(DEV_GETMIXER, &DEV_SHARED_DATA);
            break;
            
        case AUD_SUBTYPE_FU:
            // first format: get one value from a specific channel or control
            if (UsbRequest.lowValue != 0xFF)
            {
                switch (UsbRequest.hiwValue)
                {
                case AUD_FU_MUTE_CNTL:
                    PARAMS_GETMUTE(UsbRequest.hiwIndex, UsbRequest.lowValue);
                    DEV_FUNCTION(DEV_GETMUTE, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_VOLUME_CNTL:
                    PARAMS_GETVOL(UsbRequest.hiwIndex, UsbRequest.lowValue, UsbRequest.bRequest);
                    DEV_FUNCTION(DEV_GETVOL, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_BASS_CNTL:
                    PARAMS_GETBASS(UsbRequest.hiwIndex, UsbRequest.lowValue, UsbRequest.bRequest);
                    DEV_FUNCTION(DEV_GETBASS, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_TREBLE_CNTL:
                    PARAMS_GETTREBLE(UsbRequest.hiwIndex, UsbRequest.lowValue, UsbRequest.bRequest);
                    DEV_FUNCTION(DEV_GETTREBLE, &DEV_SHARED_DATA);
                    break;
                    
                default:
                    // Check later and call application to
                    // handle this case
                    unHandleCase = 1;
                    break;
                }
            }
            // second format: get multiple values from all available channels or controls
            else
            {
                switch (UsbRequest.hiwValue)
                {
                case AUD_FU_MUTE_CNTL:
                    PARAMS_GETMUTEALL(UsbRequest.hiwIndex, UsbRequest.bRequest, UsbRequest.wLength);
                    DEV_FUNCTION(DEV_GETMUTEALL, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_VOLUME_CNTL:
                    PARAMS_GETVOLALL(UsbRequest.hiwIndex, UsbRequest.bRequest,UsbRequest.wLength >> 1);
                    DEV_FUNCTION(DEV_GETVOLALL, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_BASS_CNTL:
                    PARAMS_GETBASS(UsbRequest.hiwIndex, UsbRequest.bRequest, UsbRequest.wLength);
                    DEV_FUNCTION(DEV_GETBASSALL, &DEV_SHARED_DATA);
                    break;
                    
                case AUD_FU_TREBLE_CNTL:
                    PARAMS_GETTREBLE(UsbRequest.hiwIndex, UsbRequest.bRequest, UsbRequest.wLength);
                    DEV_FUNCTION(DEV_GETTREBLEALL, &DEV_SHARED_DATA);
                    break;
                    
                default:
                    // Check later and call application to
                    // handle this uncovered case
                    unHandleCase = 1;
                    break;
                }
            }
            break;
        default:	
            // Check later and call application to
            // handle this uncovered case
            unHandleCase = 1;
            break;
        }
        
        break;
        
    case AUD_BMREQ_SET_EP:
        if (UsbRequest.hiwValue == AUD_SAMP_FREQ_CTL)
        {
            PARAMS_SETFREQ(UsbRequest.lowIndex);
            DEV_FUNCTION(DEV_SETFREQ, &DEV_SHARED_DATA);
        }		
        break;
        
    case AUD_BMREQ_GET_EP:
        if (UsbRequest.hiwValue == AUD_SAMP_FREQ_CTL)
        {
            PARAMS_GETFREQ(UsbRequest.lowIndex);
            DEV_FUNCTION(DEV_GETFREQ, &DEV_SHARED_DATA);
        }		
        break;	
        
    default:
        // Check later and call application to
        // handle this uncovered case
        unHandleCase = 1;
        break;
    }
  
    // Check to see if the case is not handled by the ROM code
    if (unHandleCase)
    {
      PARAMS_AUDIOCLASSHANDLER(&UsbRequest);
      DEV_FUNCTION(DEV_AUDIOCLASSHANDLER, &DEV_SHARED_DATA);
    }

    UsbRequest.dataPtr = DEV_DATA_PTR;
    UsbRequest.status = DEV_DATA_BYTE;  
}
