//=============================================================================
// RomFunc.c
//
// This module contains the common entry point for ROM functions that are
// exported to the application FW.
//
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//============================================================================= 

#include "types.h"
#include "xdata.h"
#include "i2c.h"
#include "romfunc.h"
#include "Utils.h"
#include "UsbEng.h"
#include "UsbDfu.h"
#include "devRef.h"

#define ROM_COMMON_RAM	0
#define ROM_I2C_WRITE	1




//
// Function:    Provide an common entry point to all ROM functions which are 
//              available to the application code.
//
// Input:       nFunction - Rom function being called.
//              ROM_FUNC_PARAMS - Global xdata memory location used to pass 
//                  function parameters.
//
// Returns 1 if function returns successfully. 0 o.w.
//
word RomFunction(byte Function)
{
  //
  // Determine what rom function is being called.
  //
  switch (Function)
  {
  case ROM_GET_PARAM_ADDR:
    return (word)&Externaldata.RomSharedData;
    
  case ROM_I2C_ACCESS:
    return I2CAccess();
    
  case ROM_BOOT_RESET_CPU:
    UtilResetBootCPU();
    return 1;
    
  case ROM_RESET_CPU:
    UtilResetCPU();
    return 1;
    
  case ROM_ENG_USB_INIT:
    engUsbInit();
    return 1;
    
  case ROM_INIT_DFU_STATE:
//    dfuWritePattern((byte *)&DfuStateMachine, sizeof(DfuStateMachine), 0);
    dfuInitStateMachine();
    return 1;
    
  case ROM_RUN_DFU_MODE:
    dfuSetup(DEV_ROMFUNC_DATA.DevDfuSetup.Target, DEV_ROMFUNC_DATA.DevDfuSetup.PollTablePtr);
    return 1;
    
  default:
    return 0;
    
  }
}

