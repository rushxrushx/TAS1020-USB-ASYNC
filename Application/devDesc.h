//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*=======================================================
devDesc.h: header file for DevDesc.cpp which contains all
the descriptors.
=======================================================*/
#include "..\ROM\types.h"
#include "..\ROM\usb.h"
#include "..\ROM\UsbDfu.h"

#ifndef	_DEVDESC_H
#define _DEVDESC_H
// Device descriptor
#define DEV_MAX_EP0_PKT 	    0x08
#define	DEV_VENDOR_ID_H		    0x04
#define	DEV_VENDOR_ID_L		    0x51
#define DEV_PRODUCT_ID_H	    0x10
#define DEV_PRODUCT_ID_L	    0x20
#define DEV_DEVICE_REL_H	    0x01
#define DEV_DEVICE_REL_L	    0x00
#define DEV_IMANUFACTURE	    0x01

#define DEV_DFU_PRODUCT_ID_H	0x10
#define DEV_DFU_PRODUCT_ID_L	0x21

#ifdef _HID_
#define HID_IF_COUNT            1
#define HID_DESC_SIZE           (SIZEOF_INTERFACE_DESCRIPTOR + 0x07 + 0x09)
#else
#define HID_IF_COUNT            0
#define HID_DESC_SIZE           0
#endif

#ifdef _DFU_
#define DFU_IF_COUNT            1
#define DFU_DESC_SIZE           (SIZEOF_INTERFACE_DESCRIPTOR + DFU_FUNC_DESC_SIZE)
#else
#define DFU_IF_COUNT            0
#define DFU_DESC_SIZE           0
#endif

// Configuration descriptor
#define DEV_CONFIG_BLOCK_SIZE	(0x6d + HID_DESC_SIZE + DFU_DESC_SIZE)
#define DEV_MAX_IF			    2 + DFU_IF_COUNT + HID_IF_COUNT

// Interface descriptors

// interface Ids/numbers
#define DEV_AUD_CTLR_IF_ID	    0x01
#define	DEV_SPK_IF_ID			0x02
#define DEV_MICREC_IF_ID		0x03
#define	DEV_HID_IF_ID			0x04


#define DEV_SPK_SETTING_0		0x00
#define DEV_SPK_SETTING_1		0x01

// Audio Control definitions
#define DEV_HOST_IT_ID			0x01 
#define DEV_SPK_FU_ID			0x02
#define DEV_SPK_OT_ID			0x03

// Endpoints.
#define DEV_EP_ID_MASK			0x7F
#define DEV_AUD_CTLR_EP_ID	    0x00
#define DEV_SPK_EP_ID			0x01
#define DEV_FB_EP_ID			0x82
#define DEV_HID_EP_ID			0x83

// Miscellaneous
#define	DEV_SPK_MAX_SUBTYPES	0x03
#define DEV_MIC_MAX_SUBTYPES	0x03

// Speaker
#define DEV_DESC_SPK_NUM_CHANNELS	0x02
#ifdef _APP_24_BIT_
#define DEV_SPK_BIT_RES		        0x18
#else
#define DEV_SPK_BIT_RES				0x10
#endif
#define DEV_DESC_SPK_SUBFRAMESIZE	((DEV_SPK_BIT_RES + 7) >> 3)
#define DEV_NUM_BYTE_PER_SAMPLE		(DEV_DESC_SPK_SUBFRAMESIZE*DEV_DESC_SPK_NUM_CHANNELS) 


// HID definitions
#define	DEV_SIZEOF_HID_REPORT_DESC	    45
#define DEV_SIZEOF_HID_DESC             0x09
#define DEV_HID_MAX_PKT	                0x01
#define DEV_HID_PRATE	                0x10
//#define DEV_HID_PRATE	                0x1

// String descriptors
#define DEV_LANG_STR_IDX      0x00
#define DEV_COMPANY_STR_IDX   0x01
#define DEV_PROD_STR_IDX      0x02

#define DEV_PROD_STR_SIZE     0x1C
#define DEV_COMPANY_STR_SIZE  0x24
#define DEV_LANGID_SIZE       0x04

// proto
extern code byte AppDevDesc[];
extern byte code AppConfigDesc[];
extern byte code AppDevHidDesc[];
extern byte code AppDevHidReportDesc[];
extern byte code AppDevProductStringDesc[];
extern byte code AppDevLanguageIDDesc[];
extern byte code AppDevCompanyStringDesc[];

#endif
