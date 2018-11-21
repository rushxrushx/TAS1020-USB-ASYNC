//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 
//==================================================================== 
// DfuMmap.h
// Mapping for the STC buffer RAM usage related to the DFU mode
// note: 
// - The buffer used starts after the buffers defined in Mmap.h
// - Check Mmap.h for any notes
// - When DFU mode is running, there will be no application running
//   and so the whole STC buffer can be used by DFU. On the other hand
//   when application is running, the DFU mode is not running, so
//   the DFU mode buffer is not used. That means the buffers for
//   application and DFU mode can be overlayed with no conflict.
//==================================================================== 
#ifndef _DFUMMAP_H
#define _DFUMMAP_H
#include "Mmap.h"

#endif