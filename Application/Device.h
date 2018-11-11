//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
#ifndef DEVICE_H
#define DEVICE_H
#include "devDesc.h"

#define CONTENTH    stc_sfr(0xFFEC)
#define CONTENTL    stc_sfr(0xFFEB)

// Misc
#define STALLSpkEp	  STALLOutEp1;STALLInEp2;
#define STALLHidEp	  STALLInEp3


#define STALLClrSpkEp STALLClrOutEp1;STALLClrInEp2;
#define STALLClrHidEp STALLClrInEp3


#define DEV_SPK_LEFT_CN		    0x00
#define DEV_SPK_RIGHT_CN		0x01
#define DEV_LINEIN_LEFT_CN		0x00
#define DEV_LINEIN_RIGHT_CN		0x01

#define DEV_MIN_INDEX			0x00
#define DEV_MAX_INDEX			0x01

#define	DEV_STATUS_SELFPOWER		0x0100
#define DEV_STATUS_REMOTE_WAKEUP	0x0200

#define DEV_SPK_HID_IDLE_RATE	0x20
#define DEV_MIC_HID_IDLE_RATE	0x20
#define DEV_BOOT_PROTOCOL		0x00
#define DEV_REPORT_PROTOCOL		0x01

// Enpoint state, bit sensitive.
#define	DEV_EP_STATE_HALT		0x0100
#define	DEV_EP_STATE_ENABLE		0x0200
#define DEV_EP_STATE_DISABLE	0x0400

// Speaker constant paramters
#define DEV_NUM_SPK_CHANNELS	0x02
/*
#define DEV_SPK_MSK_VOL	    0x1F
#define DEV_SPK_MAX_VOL		0x1F
#define DEV_SPK_MIN_VOL		0x00
#define DEV_SPK_RES_VOL		0x01
#define DEV_SPK_CUR_VOL		0x10
*/
// Speaker constant paramters
// 0 DB
#define DEV_SPK_MAX_VOL_BOUND   0x0000
#define DEV_SPK_MAX_VOL         0x0000
// - 46.5 DB
#define DEV_SPK_MIN_VOL         0xD180
// -22.5 DB
#define DEV_SPK_CUR_VOL         0xE980
// 1.5 DB resolution
#define DEV_SPK_RES_VOL         0x0180

#define DEV_SPK_MSK_MUTE	0x01
#define DEV_SPK_MUTE_ON		0x01
#define DEV_SPK_MUTE_OFF	0x00

#define DEV_SPK_MSK_BASS	0x7F
#define DEV_SPK_MAX_BASS	0x60
#define DEV_SPK_MIN_BASS	0x00
#define DEV_SPK_RES_BASS	0x01
#define DEV_SPK_CUR_BASS	0x30

#define DEV_SPK_MSK_TREBLE	0x7F
#define DEV_SPK_MAX_TREBLE	0x60
#define DEV_SPK_MIN_TREBLE	0x00
#define DEV_SPK_RES_TREBLE	0x01
#define DEV_SPK_CUR_TREBLE	0x30



// structures

// Speaker setup
typedef struct _dev_spk_struct {
	// Current left/right volumes
	int curVol[DEV_NUM_SPK_CHANNELS];

	// Previous left/right volumes
	int preVol[DEV_NUM_SPK_CHANNELS];

	// Current Bass for master channel
	byte curBass;

	// Previous Bass for master channel
	byte preBass;

	// Current Treble for master channel
	byte curTreble;

	// Previous Treble for master channel
	byte preTreble;

  // Sampling frequency
  word  freq;

} DEV_SPK_STRUCT;


typedef struct _dev_hid_parms {
	byte reportItem;
  byte pad;
} DEV_HID_PARMS;



// Basic interface structures
typedef struct _dev_interface_struct {
	// Interface current setting
	byte curSetting;

	// End point state
	word epState;
} DEV_INTERFACE_STRUCT;

// Device record
typedef struct _dev_device_struct {
	// Device selfpowered/remote wakeup
	word devStatus;

	// Configuration current setting
	byte configSetting;

	// Interfaces		
	// Audio control
	struct _dev_interface_struct audCtlrIf;

	// Speaker set setup parameters
	struct _dev_spk_struct spk;

	// microphone Record setup paramters
//	struct _dev_micRec_struct micRec;

	// Line In setup paramters
//	struct _dev_lineIn_struct lineIn;	

	// microphone Feed back setup paramters
//	struct _dev_micIn_struct micIn;

	// Speaker set interface 
	struct _dev_interface_struct spkIf;

	// Microphone Record interface
//	struct _dev_interface_struct micRecIf;

	// HID interface for the speaker set
	struct _dev_interface_struct hidIf;

	// HID parameters for the speaker set
	struct _dev_hid_parms hidParms;

	// DFU interface for the speaker set
	struct _dev_interface_struct dfuIf;

    // Dummy values
    word dummy;

    // Mute temporary storage
    byte mute;

} DEV_DEVICE_STRUCT;

// References
extern DEV_DEVICE_STRUCT AppDevice;
extern bit AppDeviceSpkcurMute;
extern bit AppDeviceSpkpreMute;
extern bit AppResetFlag;
extern bit AppResetFlag1;
extern bit AppSuspendFlag;
extern bit AppResumeFlag;
extern bit AppSleepModeOn;
extern byte UsbAddress;
extern word DevAmpCounter;
extern DFU_STATUS_STRUCT DfuStatusData;

// for testing DFU DnLoad/Upload when target is DFU_TARGET_OTHER
#ifdef _ADD_DFU_TARGET_OTHER_
extern byte *DevTestDfuDataPtr;
#endif

// Turn on/off Amp for speaker
sbit DevTurnOffAmp = P1^6;
#define DEV_TURN_OFF_AMP    DevTurnOffAmp = TRUE
#define DEV_TURN_ON_AMP     DevTurnOffAmp = FALSE 
// Prototypes
extern void devUserIntHandler();
extern void devGetStrDesc();
extern void devSoftPll();
extern void devInit();
extern void devInitRec();
extern void devInitSTC();
extern void devUpdate();
extern void DevFunctionEntryParser(byte ,void xdata *);
void devSleepModeOff();
extern void devSleepModeOn();
extern void devCheckReset();
extern void devSetRemoteWakeUp();
extern void devTurnOffRemoteWakeUp();
void devCtlAmpPower(bit );

// GPIO pins
sbit CoAc97ModePin0 = P1^0;
sbit CoAc97ModePin1 = P1^1;
sbit P33WakeButton = P3^3;
sbit P34WakeButton = P3^4;
sbit P35WakeButton = P3^5;
#endif
