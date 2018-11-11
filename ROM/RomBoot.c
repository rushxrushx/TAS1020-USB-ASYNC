//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 
/*====================================================================  
RomBoot.C
Main Entry for the firmware
====================================================================*/
#include "types.h"
#include <Reg52.h>
#include "reg_stc1.h"
#include "Mmap.h"
#include "RomBoot.h"
#include "usb.h"
#include "usbEng.h"
#include "eeprom.h"
#include "usbDfu.h"
#include "Utils.h"
#include "devRef.h"
#include "xdata.h"



/*====================================================================
main() : Main Entry for the ROM firmware, running in the idle mode
Input:  none
Output: none
Return: none
=====================================================================*/
void main()
{  
    // Check if Shadow RAM bit is cleared
    GLOBCTL = 0x04;				  // 12Mclk, Ext int off, LPWR on, CODEC is off   
    USBFADR = 0;            // cleared device address
    USBIMSK = 0;
    
    // Default ROM state
    RomRecord.state = ROM_BOOT_START; 
    RomRecord.attribute = 0;
    RomRecord.bootType = 0;   
    
    // Read Eeprom Header and decides which type of boot process
    eepromExist();
    
    switch(DfuData.eepromHeaderData.dataType)
    {
    case EEPROM_APPCODE_TYPE:
        if (eepromBoot() == TRUE)
        {
            RomRecord.state = ROM_APP_RUNNING;
            if (DfuData.eepromHeaderData.attribute & EEPROM_CPU_SPEED_24MHZ)
                GLOBCTL |= 0x80;            
        }
    default:
        if (RomRecord.state == ROM_BOOT_START)
        {
            if ((DfuData.eepromHeaderData.dataType == EEPROM_DEVICE_TYPE) && 
                (DfuData.eepromHeaderData.attribute & EEPROM_CPU_SPEED_24MHZ))
                GLOBCTL |= 0x80;
            
            if ((DfuData.eepromHeaderData.dataType == EEPROM_UNEXIST) ||
                (DfuData.eepromHeaderData.dataType == EEPROM_DEVICE_TYPE))
                dfuSetup(DFU_TARGET_RAM, 0);		
            else
                dfuSetup(DFU_TARGET_EEPROM, 0);
            
        }
        break;
    }
    
    // Flag for App ready
    RomRecord.state  = ROM_APP_RUNNING;
    
    // Now ready to detach start running application code
    UtilResetCPU();
    
    return;
}
