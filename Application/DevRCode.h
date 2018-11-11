//=============================================================================
// RomCode.h
//
// This module contains defines for accessing code in the ROM.
//
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//============================================================================= 

//
// Rom function numbers.
//

//
// The following structure defines the structure of generic pointers.
// I.E. generic pointers are stored using 3 bytes. The first byte specifies
// the memory type (e.g. xdata, code), the second byte is the MSB of the
// the address and the third is the LSB of the address.
//
typedef struct _tag_GENERIC_PTR
{
    byte MemoryType;

    union
    {
    byte Addr[2];
    word wAddr;
    void xdata * pXdata;
    void code * pCode;
    void idata * pIdata;
    void data * pData;
    } ptr;

} GENERIC_PTR;

//
// Generic pointer memory types.
//
#define IDATA_PTR       ((byte) 0x00)
#define XDATA_PTR       ((byte) 0x01)
#define CODE_PTR        ((byte) 0xFF)


//
// Make a generic pointer.
//
#define MAKE_GENERIC_PTR(MemorySpace, Ptr, GenericPtr) \
                            GenericPtr.MemoryType = MemorySpace; \
                            GenericPtr.wAddr = (word)Ptr



//
// Entry point for RomFunction.
//
#pragma REGPARMS
word devRomFunction(byte);





