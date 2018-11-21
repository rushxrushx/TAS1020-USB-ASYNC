//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
// Bit definition for the buttons
// The Up/down are used for volume, treble, and bass
// The On/Off are used for mute.

#ifndef _BUTTONS_H
#define _BUTTONS_H

#define	BUT_VOL_UP					0x01
#define BUT_VOL_DOWN				0x02
#define	BUT_MUTE_TOGGLE				0x04

// GPIO definitions
// GPIO bins used for volume up/down/mute
sbit ButUp = P3^3;
sbit ButDown = P3^4;
sbit ButMuteSel = P3^5;

extern void butChkUpdate();
#endif
