//====================================================================
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//====================================================================
/*====================================================================
Usb.c: This file contains main entry point for 
parsing requests, usbProtocolHandler(), and USB 
standard handler for chapter 9, usbStandardHandler()
====================================================================*/
#include <Reg52.h>
#include "types.h"
#include "reg_stc1.h" 
#include "usb.h"
#include "UsbEng.h"
#include "usbaudio.h"
#include "UsbHid.h"
#include "Mmap.h"
#include "dfuMmap.h"
#include "UsbDfu.h"
#include "RomBoot.h"
#include "DevRef.h"
#include "xdata.h"

#ifdef	_TEST_RX_UNEXPECTED_
// ???
bit StringDescGetOn;
#endif

/*====================================================================
usbProtocolHandler() : Main entry for handling the USB protocol
Input:	none
Output: none
Globals used:
UsbRequest
EngParms
Return: none
Note:
- For a USB Read request, the request error is based on the 
UsbRequest.dataPtr: If the pointer is null, the request is failed.
It is the responsibility of the related called functions to set the
UsbRequest.dataPtr.
- For a USB Write request, the related called functions set directly
the UsbRequest.status which is either EVENT_OK or EVENT_ERROR.
- The initial values for UsbRequest.status is EVENT_ERROR and 
UsbRequest.dataPtr is NULL.
=====================================================================*/
void usbProtocolHandler(void)
{
    
    // Initialize status to error first and
    // data pointer, in case of READ requests, to null
    UsbRequest.status = EVENT_OK;
    UsbRequest.dataPtr = 0;
    UsbRequest.xferLength = UsbRequest.wLength;
    
    // Check request type
    switch (UsbRequest.bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_STANDARD:
        usbStandardHandler();
        break;
        
    case USB_REQ_TYPE_VENDOR:
        PARAMS_USBVENDORHANDLER((byte *)&UsbRequest);
        DEV_FUNCTION(DEV_USBVENDORHANDLER, &DEV_SHARED_DATA);
        UsbRequest.status = DEV_DATA_BYTE;
        UsbRequest.dataPtr = DEV_DATA_PTR;
        break;
        
    case USB_REQ_TYPE_CLASS:
        // Check class by checking the endpoint or the interface
        switch(UsbRequest.bmRequest & USB_REQ_TYPE_RECIP_MASK)
        {
        case USB_REQ_TYPE_INTERFACE:
            if (RomRecord.state == ROM_DFU_MODE)
            {
                if(UsbRequest.lowIndex == 0)
                    UsbRequest.temp = USB_DFU_CLASS;
                else
                    UsbRequest.temp = USB_UNKNOWN_CLASS;
            }
            else
            {
                PARAMS_GETIFCLASS(UsbRequest.lowIndex);
                DEV_FUNCTION(DEV_GETIFCLASS, &DEV_SHARED_DATA);
                UsbRequest.temp = DEV_DATA_WORD;
                
            }
            
            break;
            
        case USB_REQ_TYPE_ENDPOINT:
            if (RomRecord.state == ROM_DFU_MODE)
            {
                if(UsbRequest.lowIndex == 0)
                    UsbRequest.temp = USB_DFU_CLASS;
                else
                    UsbRequest.temp = USB_UNKNOWN_CLASS;
            }
            else 
            {
                PARAMS_GETEPCLASS(UsbRequest.lowIndex);
                DEV_FUNCTION(DEV_GETEPCLASS, &DEV_SHARED_DATA);
                UsbRequest.temp = DEV_DATA_WORD;
            }
            
            break;
            
        default:
            UsbRequest.temp = USB_UNKNOWN_CLASS;
            break;
        }
        
        // Handle the classes
        switch(UsbRequest.temp)
        {
        case USB_AUD_CLASS:
            audHandler();
            break;
        
        case USB_HID_CLASS:
            hidHandler();
            break;
        
        case USB_DFU_CLASS:
            dfuHandler();
            break;
        
        case USB_UNKNOWN_CLASS:
            PARAMS_CLASSHANDLER(&UsbRequest);
            DEV_FUNCTION(DEV_CLASSHANDLER, &DEV_SHARED_DATA);
            UsbRequest.dataPtr = DEV_DATA_PTR;
            UsbRequest.status = DEV_DATA_BYTE;
            break;
        
        default:
            break;
        }
        break;
    
    default:
        // should never get here
        break;
    }
    
    // ROM handle Classes
    // If READ EVENT
    if (UsbRequest.bmRequest & USB_REQ_TYPE_READ)
        // Check if there is any data to be sent
        if (UsbRequest.dataPtr == 0)
            // So we got somw data for this READ EVENT
            UsbRequest.status = EVENT_ERROR;
}

/*====================================================================
usbStandardHandler() : Handle USB Standard chapter 9
Input:	none
Output: none
Globals used:
UsbRequest
Return: none
Note:
- For a USB Read request, the UsbRequest.dataPtr is set to buffer
pointer or NULL for bad or unsupported request.
- For a USB Write request, the UsbRequest.status is set either to
EVENT_OK or EVENT_ERROR.
=====================================================================*/
void usbStandardHandler(void)
{
    if (UsbRequest.bmRequest & USB_REQ_TYPE_READ) 
    { // READ EVENT
        switch (UsbRequest.bRequest) 
        { 	/* check bRequest */
        case USB_REQ_GET_CONFIGURATION:
            if (RomRecord.state == ROM_DFU_MODE)
                UsbRequest.dataPtr = &DfuDevice.configSetting;
            else
            {
                DEV_FUNCTION(DEV_GETCONFIG, &DEV_SHARED_DATA);
                UsbRequest.dataPtr =	DEV_DATA_PTR;
            }
            
            break;
            
        case USB_REQ_GET_STATUS:				
            // check recipients
            switch (UsbRequest.bmRequest & USB_REQ_TYPE_RECIP_MASK) 
            { 	
            case USB_REQ_TYPE_DEVICE:
                if (RomRecord.state == ROM_DFU_MODE)
                    UsbRequest.dataPtr = DfuDevice.devStatus;
                else
                {
                    DEV_FUNCTION(DEV_GETDEVICESTATUS, &DEV_SHARED_DATA);
                    UsbRequest.dataPtr = DEV_DATA_PTR;
                }
                
                break;
                
            case USB_REQ_TYPE_INTERFACE:
                if (RomRecord.state == ROM_DFU_MODE)
                    UsbRequest.dataPtr = DfuDevice.dummy;
                else
                {
                    PARAMS_GETIFSTATUS(UsbRequest.lowIndex);
                    DEV_FUNCTION(DEV_GETIFSTATUS, &DEV_SHARED_DATA);
                    UsbRequest.dataPtr = DEV_DATA_PTR;
                }
                
                break;
                
            case USB_REQ_TYPE_ENDPOINT:
                if (RomRecord.state == ROM_DFU_MODE)
                    UsbRequest.dataPtr = DfuDevice.epState;
                else
                {
                    PARAMS_GETEPSTATUS(UsbRequest.lowIndex);
                    DEV_FUNCTION(DEV_GETEPSTATUS, &DEV_SHARED_DATA);
                    UsbRequest.dataPtr = DEV_DATA_PTR;
                }
                
                break;
                
            default:
                break;
            }
            break;
            
        case USB_REQ_GET_DESCRIPTOR:
            // check descriptor type
            switch (UsbRequest.hiwValue) 
            { 
            case DESC_TYPE_DEVICE:
                if (RomRecord.state == ROM_DFU_MODE)
                {
                    UsbRequest.dataPtr = DfuData.deviceDesc;
                    UsbRequest.xferLength = SIZEOF_DEVICE_DESC;
                }
                else
                {
                    PARAMS_GETDEVDESC(&UsbRequest.xferLength);
                    DEV_FUNCTION(DEV_GETDEVDESC, &DEV_SHARED_DATA);
                    UsbRequest.dataPtr = DEV_DATA_PTR;
                }
                
                break;
                
            case DESC_TYPE_CONFIG:
                if (RomRecord.state == ROM_DFU_MODE)
                {
                    UsbRequest.dataPtr = DfuData.configDesc;
                    UsbRequest.xferLength = DFU_SIZEOF_CONFIG_BLOCK;
                }
                else
                {
                    PARAMS_GETCONFIGDESC(&UsbRequest.xferLength, UsbRequest.hiwValue);
                    DEV_FUNCTION(DEV_GETCONFIGDESC, &DEV_SHARED_DATA);
                    UsbRequest.dataPtr = DEV_DATA_PTR;
                }
                break;
                
            case DESC_TYPE_HID:
                PARAMS_GETHIDDESC(&UsbRequest.xferLength, UsbRequest.lowIndex);
                DEV_FUNCTION(DEV_GETHIDDESC, &DEV_SHARED_DATA);
                UsbRequest.dataPtr =	DEV_DATA_PTR;
                break;
                
            case DESC_TYPE_HID_REPORT:
                PARAMS_GETHIDREPORTDESC(&UsbRequest.xferLength, UsbRequest.lowIndex);
                DEV_FUNCTION(DEV_GETHIDREPORTDESC, &DEV_SHARED_DATA);
                UsbRequest.dataPtr =	DEV_DATA_PTR;
                break;
                
            case DESC_TYPE_STRING:
                if (RomRecord.state == ROM_DFU_MODE)
                {
                    dfuGetStrDesc();
                }
                else
                {
                    PARAMS_GETSTRDESC(&UsbRequest.xferLength, UsbRequest.lowValue);
                    DEV_FUNCTION(DEV_GETSTRDESC, &DEV_SHARED_DATA);
                    UsbRequest.dataPtr =	DEV_DATA_PTR;
                }
                
                break;
                
            default:
                break;
            }
            break;
            
        case USB_REQ_GET_INTERFACE:
            if (RomRecord.state == ROM_DFU_MODE)
                UsbRequest.dataPtr = &DfuDevice.ifCurSetting;
            else
            {
                PARAMS_GETIF(UsbRequest.lowIndex);
                DEV_FUNCTION(DEV_GETIF, &DEV_SHARED_DATA);
                UsbRequest.dataPtr =	DEV_DATA_PTR;
            }
            
            break;
            
        default:
            break;
        }
    } // end of READ_EVENT
  
    else // WRITE EVENT
    { 
        // check bRequest
        switch (UsbRequest.bRequest) 
        { 
        case USB_REQ_CLEAR_FEATURE:
            // type device
            if ((UsbRequest.bmRequest == USB_REQ_TYPE_DEVICE) &&
                (UsbRequest.lowValue == FEATURE_REMOTE_WAKEUP))
            {
                if (RomRecord.state == ROM_DFU_MODE)
                {
                    DfuDevice.devStatus[0] &= ~DFU_STATUS_REMOTEWAKEUP; 
                }
                else
                {
                    DEV_FUNCTION(DEV_CLRREMOTEWAKEUP, &DEV_SHARED_DATA);
                    UsbRequest.status = DEV_DATA_BYTE;
                }
            }
            else 
            {
                // type endpoint
                if ((UsbRequest.bmRequest == USB_REQ_TYPE_ENDPOINT) &&
                    (UsbRequest.lowValue == FEATURE_ENDPOINT_STALL))
                {
                    if (RomRecord.state == ROM_DFU_MODE)
                    {
                        DfuDevice.epState[0] &= ~DFU_STATUS_EP_HALT; 
                    }
                    else
                    {
                        PARAMS_CLEAREPFEATURE(UsbRequest.lowIndex);
                        DEV_FUNCTION(DEV_CLEAREPFEATURE, &DEV_SHARED_DATA);
                        UsbRequest.status = DEV_DATA_BYTE;
                    }
                    
                }
                else
                    UsbRequest.status = EVENT_ERROR;
            }
            break;
            
        case USB_REQ_SET_FEATURE:
            // type device
            if ((UsbRequest.bmRequest == USB_REQ_TYPE_DEVICE) &&
                (UsbRequest.lowValue == FEATURE_REMOTE_WAKEUP))
            {
                if (RomRecord.state == ROM_DFU_MODE)
                {
                    DfuDevice.devStatus[0] |= DFU_STATUS_REMOTEWAKEUP; 
                }
                else
                {
                    DEV_FUNCTION(DEV_SETREMOTEWAKEUP, &DEV_SHARED_DATA);
                    UsbRequest.status = DEV_DATA_BYTE;
                }
            }
            else 
            {
                // type endpoint
                if ((UsbRequest.bmRequest == USB_REQ_TYPE_ENDPOINT) &&
                    (UsbRequest.lowValue == FEATURE_ENDPOINT_STALL))
                {
                    if (RomRecord.state == ROM_DFU_MODE)
                    {
                        DfuDevice.epState[0] |= DFU_STATUS_EP_HALT; 
                    }
                    else
                    {
                        PARAMS_SETEPFEATURE(UsbRequest.lowIndex);
                        DEV_FUNCTION(DEV_SETEPFEATURE, &DEV_SHARED_DATA);
                        UsbRequest.status = DEV_DATA_BYTE;
                    }
                }
                else
                    UsbRequest.status = EVENT_ERROR;
            }
            break;
            
        case USB_REQ_SET_ADDRESS:
            if (UsbRequest.lowValue != 0xFF)
            {
                // send 0 byte data packet				
                EngParms.state = USB_WAIT_ADDR_ACK;
            }
            break;
            
        case USB_REQ_SET_CONFIGURATION:
            if (RomRecord.state == ROM_DFU_MODE)
            {
                DfuDevice.configSetting = UsbRequest.lowValue;
                DfuDevice.ifCurSetting = 0;
                DfuDevice.epState[0] = 0;
            }
            else
            {
                PARAMS_SETCONFIG(UsbRequest.lowValue);
                DEV_FUNCTION(DEV_SETCONFIG, &DEV_SHARED_DATA);
                UsbRequest.status = DEV_DATA_BYTE;
            }
            break;
            
        case USB_REQ_SET_INTERFACE:
            if (RomRecord.state == ROM_DFU_MODE)
            {
                DfuDevice.ifCurSetting = UsbRequest.lowValue;
            }
            else
            {
                PARAMS_SETIF(UsbRequest.lowIndex, UsbRequest.lowValue);
                DEV_FUNCTION(DEV_SETIF, &DEV_SHARED_DATA);
                UsbRequest.status = DEV_DATA_BYTE;
            }
            break;
            
        default:
            UsbRequest.status = EVENT_ERROR;
            break;
        } 
    } // end of WRITE_EVENT
}
///////////////////////////////
/* end of chapter 9 requests */

