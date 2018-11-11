//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 
/*====================================================================  
Utils.c
Utilities functions
====================================================================*/
#pragma SRC

#include "types.h"
#include <Reg52.h>
#include "reg_stc1.h"
#include "Mmap.h"
#include "Utils.h"

/*====================================================================
UtilResetBootCPU: Reset 51 core and USB portion to hard reset values and
perfom long jump to address 0 (reset vector) and boot from ROM
Input:    None
Return:   None
Remark:   compile w #pragma SRC and link w code segment 8003h 
Note: This is a complete reset as in power up
====================================================================*/
void UtilResetBootCPU() {
  // STC portion - Does not reset DMA, C-PORT, D-PORT, ACG 
  IE=0;           // Disable 8051 interrupts 
  //    USBCTL=0x10;    // CONT is 0  - causing host to detect a device disconnect
  // FRSTE is set - for a real device reset including external
  // RAM
  // FEN = 0                                               
  //    GLOBCTL &= 0x80; // Keep the CPU speed the same at 12/24MHZ
  
  //    USBFADR=0;
  //    USBSTA=0;
  
  USBCTL |= 0x01;       // Set SDW bit confirm
  MEMCFG &= ~MEMCFG_SDW_BIT;    // turn off SDW bit to turn on shadow ROM
  USBCTL &= ~0x01;      // Reset SDW bit confirm

#pragma asm
  ljmp 08000h		// jump to reset vector
#pragma endasm
    
    return;
} /* ResetCPU */

/*====================================================================
UtilResetCPU: Reset 51 core and USB portion to hard reset values and
perfom long jump to address 0 (reset vector)and jump to Program RAM
Input:    None
Return:   None
Remark:   compile w #pragma SRC and link w code segment 8003h 
====================================================================*/
void UtilResetCPU() {
  // STC portion - Does not reset DMA, C-PORT, D-PORT, ACG 
  IE=0;           // Disable 8051 interrupts 
  //    USBCTL=0;     	// CONT is 0  - causing host to detect a device disconnect
  // FRSTE is 0 - prevent a real device reset
  // FEN = 0                                               
  //    GLOBCTL &= 0x80; // Keep the CPU speed the same at 12/24MHZ
  
  //    USBFADR=0;
  //    USBSTA=0;
  
  MEMCFG |= MEMCFG_SDW_BIT;    // turn on SDW bit to turn off shadow ROM
  
#pragma asm
  ljmp 00000h		// jump to reset vector
#pragma endasm
    
    return;
} /* ResetCPU */



