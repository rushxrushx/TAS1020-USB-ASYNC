//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//==================================================
/*================================================== 
Button.c: routines to handle the buttons (HID)
//================================================*/  
#include "..\ROM\types.h"
#include <Reg52.h>
#include "..\ROM\Reg_stc1.h"
#include "..\ROM\UsbEng.h"
#include "..\ROM\UsbDfu.h"
#include "..\ROM\Mmap.h"
#include "delay.h"
#include "Devmap.h"
#include "Codec.h"
#include "buttons.h"
#include "device.h"

bit MuteButtonDown = 0;         // Indicates if mute button is down.

/*====================================================================
butChkUpdate() : Check buttons and update device record
return : TRUE id button pressed, else FALSE
=====================================================================*/
void butChkUpdate()
{
    byte ButValue;

    // 
    //  We will only indicate a mute toggle when the mute button
    //  transitions from UP to DOWN.
    //  NOTE: 1 => not pressed.

    ButValue = ButMuteSel;                       
                                                
    if (MuteButtonDown)   
    {
        if (ButValue)
          MuteButtonDown = 0;

        ButValue = 1;
    }
    else
        MuteButtonDown = (bit)(ButValue ? 0: 1);                


    ButValue = (ButValue << 1) | ButDown;
    ButValue = (ButValue << 1) | ButUp;
    ButValue = ~ButValue & 0x07;
    
    switch (ButValue)
    {
    case BUT_MUTE_TOGGLE:
    case BUT_VOL_UP:
    case BUT_VOL_DOWN:
        break;
    default:
        ButValue = 0;
        break;
    }
   
	if (ButValue)
    {
        while (!(IEPDCNTX3 & 0x80) && !AppSuspendFlag);

		if (AppSuspendFlag)
			return;

        HID_IO_BUFFER[0] = ButValue;
        IEPDCNTX3 = 1;
        while (!(IEPDCNTX3 & 0x80) && !AppSuspendFlag);

		if (AppSuspendFlag)
			return;

        HID_IO_BUFFER[0] = 0;
        IEPDCNTX3 = 1;
        delay(380);
    }
}

