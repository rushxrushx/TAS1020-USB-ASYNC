//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*================================================== 
  Softpll.h
//================================================*/
#ifndef	_SOFTPLL_H_
#define _SOFTPLL_H_

#include "..\rom\types.h"

// For constant values
typedef struct _softpll_const_struct_ {
  word remainder;
  word refQuotient;
  byte defAcgfrq0;
  byte defAcgfrq1;
  byte defAcgfrq2;
  byte defAcgdctl;
  byte defAcgctl;
} SOFTPLL_CONST;

// Table of initial constants values for setting up ACG PLL
#define SOFTPLL_CONST_48KHZ     0
#define SOFTPLL_CONST_441KHZ    1
#define SOFTPLL_UPDATE_PERIOD   4

extern void softPllInit();
extern void softPll();

#endif
