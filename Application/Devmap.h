//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
//================================================== 
// Mmap.h
// Mapping for the Application STC buffer RAM usage
// note:
// - The offset values is from 0xF800. These are used
//   for programming the I/O endpoint buffer 
//  - All vakues are multiple of 8 bytes. Ex.: if 
//    value is 1, value is actually 8 bytes.
// - The Ep0 temporary save buffer size is defined
//   by the appication. This buffer is not the same
//   as the IN/OUT EP0 pipe buffers
//==================================================

// the size of EP0 temporary save buffer
#define DEV_USB_INEP0_SIZE		0x08
#define DEV_USB_OUTEP0_SIZE		0x08

#define USB_EP0_XFERDATA_SIZE 0x10
#define USB_DEV_EP0_XFERDATA  stc_sfr_array(INPACK_ADDR + USB_EP0_XFERDATA_SIZE)
#define USB_APP_ADDR_START    (INPACK_ADDR + DEV_USB_INEP0_SIZE + DEV_USB_OUTEP0_SIZE + USB_EP0_XFERDATA_SIZE)

// ISO OUT EP 1, speaker audio stream
#define OUTEP1_X_ADDR         USB_APP_ADDR_START

// use double buffers size in Tas1020 since there's only 1 ISO buffer
//#define OUTEP1_RSIZE          0x180 // 16 bit 48 KHZ stereo max
#ifdef _APP_24_BIT_
#define OUTEP1_RSIZE          0x240    // 24 bit 48 kHZ stereo max =576
#else
#define OUTEP1_RSIZE          0x180     // 16 bit 48 kHZ stereo max
#endif
#define	OUTEP1_XOFFSET	((OUTEP1_X_ADDR - STC_BUFFER_BASE_ADDR) >> 3)
#define OUTEP1_SIZE		(OUTEP1_RSIZE >> 3)


// ISO IN EP 2, iso fb

#define INEP2_RSIZE          0x8     // 3 BYTES feedback并且必须是8的倍数

#define INEP2_X_ADDR		(OUTEP1_X_ADDR + OUTEP1_RSIZE)
#define	INEP2_XOFFSET		((INEP2_X_ADDR - STC_BUFFER_BASE_ADDR) >> 3)
#define INEP2_SIZE			(INEP2_RSIZE >> 3)

#define INEP2_X							stc_sfr_array(INEP2_X_ADDR)

// IN Interrupt EP 3, HID
// X address 0xFB10, size = 8
#define INEP3_X_ADDR          			(INEP2_X_ADDR + INEP2_RSIZE)
#define INEP3_RSIZE						0x08
#define	INEP3_XOFFSET					((INEP3_X_ADDR - STC_BUFFER_BASE_ADDR) >> 3)
#define INEP3_SIZE						(INEP3_RSIZE >> 3)

#define INEP3_X							stc_sfr_array(INEP3_X_ADDR)
#define HID_IO_BUFFER					INEP3_X

// For debug
#define DEBUGRAM stc_sfr_array(0xFF00)

