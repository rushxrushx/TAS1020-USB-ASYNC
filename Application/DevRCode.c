//=============================================================================
// ROMCODE.C
//
// This module contains stub functions used to access functionality provided in
// the ROM.
//
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//============================================================================= 
//#pragma SRC
#pragma REGPARMS
#include "..\ROM\types.h"
#include "..\ROM\Reg_stc1.h"
//
// Public rom function entry point.
//
#define PUBLIC_ROM_FUNCTION_ENTRY   0x9FFD

//
// External int 0 in ROM
//
#define ROM_EXTINT0_ADDR            8003h

//
// This is a stub function for the external interrupt 0
// ISR in the ROM. The constant USE_ROM_EXT_INT0 should be
// defined if the user wants to use the ROM based USB engine.
//
void UseRomExtInt0() interrupt 0
{	
#pragma asm
    ljmp        ROM_EXTINT0_ADDR
#pragma endasm
}

// Jump to ROM function entry point
word devRomFunction(byte Function)
{
    Function = Function;   
#pragma asm
    ljmp        PUBLIC_ROM_FUNCTION_ENTRY
#pragma endasm
    return 1;
}
