//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 
/*====================================================================  
USBHID.C
Handler for USB HID class
====================================================================*/
#include "types.h"
#include <Reg52.h>
#include "reg_stc1.h"
#include "Mmap.h"
#include "UsbEng.h"
#include "usb.h"
#include "usbhid.h"
#include "devRef.h"

/*====================================================================
hidHandler() : Handle USB HID Class requests
Input:  none
Output: none
Globals used:
UsbRequest
Return: none
=====================================================================*/
void hidHandler()
{
  switch (UsbRequest.bRequest)
  {
  case HID_GET_REPORT:
    PARAMS_HIDGETREPORT(UsbRequest.lowIndex, UsbRequest.hiwValue,UsbRequest.lowValue);
    DEV_FUNCTION(DEV_HIDGETREPORT, &DEV_SHARED_DATA);
    break;
    
  case HID_SET_REPORT:
    PARAMS_HIDSETREPORT(UsbRequest.lowIndex, UsbRequest.hiwValue,UsbRequest.lowValue);
    DEV_FUNCTION(DEV_HIDSETREPORT, &DEV_SHARED_DATA);
    break;
    
  case HID_GET_IDLE:
    PARAMS_HIDGETIDLE(UsbRequest.lowIndex);
    DEV_FUNCTION(DEV_HIDGETIDLE, &DEV_SHARED_DATA);
    break;
    
  case HID_SET_IDLE:
    PARAMS_HIDSETIDLE(UsbRequest.lowIndex);
    DEV_FUNCTION(DEV_HIDSETIDLE, &DEV_SHARED_DATA);
    break;
    
  case HID_GET_PROTOCOL:
    PARAMS_HIDGETPROTOCOL(UsbRequest.lowIndex);
    DEV_FUNCTION(DEV_HIDGETPROTO, &DEV_SHARED_DATA);
    break;
    
  case HID_SET_PROTOCOL:
    PARAMS_HIDSETPROTOCOL(UsbRequest.lowIndex);
    DEV_FUNCTION(DEV_HIDSETPROTO, &DEV_SHARED_DATA);
    break;
  
  default:
    break;
  }
    UsbRequest.dataPtr = DEV_DATA_PTR;
    UsbRequest.status = DEV_DATA_BYTE;
}

