//================================================== 
// Texas Instruments Strictly Private 
// Copyright 1999, Texas Instruments Inc. 
//================================================== 
/*================================================== 
DEVICEDESC.C
Device Descriptors
//================================================*/
#include "..\ROM\types.h"
#include <Reg52.h>
#include "..\ROM\usb.h"
#include "..\ROM\UsbEng.h"
#include "..\ROM\UsbDfu.h"
#include "devMap.h"
#include "devDesc.h"


code byte AppDevDesc[SIZEOF_DEVICE_DESC] = 
{	
    SIZEOF_DEVICE_DESC,
    DESC_TYPE_DEVICE,
    0x00,					        // USB spec 1.00, LSB
    0x01,					        // MSB
    0x00,					        // class code - enumarated at interface
    0x00,					        // subclass code
    0x00,					        // protocol code
    DEV_MAX_EP0_PKT,	            // max EP0 packet size
    DEV_VENDOR_ID_L,DEV_VENDOR_ID_H,	
    DEV_PRODUCT_ID_L,	
    DEV_PRODUCT_ID_H,
    DEV_DEVICE_REL_L,	
    DEV_DEVICE_REL_H,	 
    0x01,						    // iManufactirer
    0x02,						    // iProduct
    0x00,						    // iSerialNumber
    0x01						    // bNumConfigurations
};


// Configuration descriptor
byte code AppConfigDesc[DEV_CONFIG_BLOCK_SIZE] =
{
    SIZEOF_CONFIG_DESCRIPTOR,
    DESC_TYPE_CONFIG,
    (BYTE )DEV_CONFIG_BLOCK_SIZE, 
    (BYTE )(DEV_CONFIG_BLOCK_SIZE >> 8),
    DEV_MAX_IF,								// bNumInterfaces
    0x01,									// bConfigurationValue
    0x00,									// iConfiguration
#ifdef SELFPOWER_REMOTE
    CFG_DESC_ATTR_SELF_POWERED | CFG_DESC_ATTR_REMOTE_WAKE,	
    0x32,							        // MaxPower N/A
#endif
#ifdef BUSPOWER_REMOTE
    CFG_DESC_ATTR_REMOTE_WAKE,	
    0x32,							        // MaxPower 100 mA
#endif
#ifdef SELFPOWER_NOREMOTE
    CFG_DESC_ATTR_SELF_POWERED,
    0x32,							        // MaxPower N/A
#endif
#ifdef BUSPOWER_NOREMOTE
    0,
    0x32,							        // MaxPower 100 mA
#endif
    
    // ============================================ 	
    // Audio Control Interface
    // =========================================== 
    
    SIZEOF_INTERFACE_DESCRIPTOR,
    DESC_TYPE_INTERFACE,
    DEV_AUD_CTLR_IF_ID,	
    0x00,  				    // bAlternateSetting
    0x00,                   // bNumEndpoints, use default endpoint
    0x01,					// bInterfaceClass
    0x01,					// bInterfaceSubClass
    0x00,					// bInterfaceProtocol
    0x00,					// iInterface	
    
    // ============================================ 	
    // Class Specific AC interface
    // =========================================== 
    
    0x09,					// bLength
    0x24,					// bDescriptorType
    0x01,	
    0x00, 0x01,             // BcdADC
    30,	0x00, 	        // wTotalLength    
    0x01,					// Number of streaming interfaces	
    DEV_SPK_IF_ID,			// speaker Audiostreaming interface 1
//    DEV_MICREC_IF_ID,			// microphone Audiostreaming interface 2
    

    //-------- For Host USB audio stream out -----
    // ============================================ 
    // Input Terminal Descriptor for Host USB 
    // audio stream out
    // =========================================== 
    0x0C,		            // bLength
    0x24,		            // bDescriptorType
    0x02,	                // bDescriptorSubtype
    DEV_HOST_IT_ID, 	    // bTerminalID
    0x01, 0x01,	            // wTerminalType USB stream
    0x00,		            // bAssocTerminal		
    0x02,                   // bNrChannels
    0x03, 0x00,	            // wChannelConfig
    0x00,                   // iChannel
    0x00,                   // iTerminal

    
    //---------------- For Speaker Ouput ----------
    // ============================================ 
    // Feature Unit
    // =========================================== 
/*    0x0A,		            // bLength
    0x24,		            // bDescriptorType
    0x06,					// bDescriptorSubtype
    DEV_SPK_FU_ID,          // bUnitId
    DEV_SPK_MIXER_ID,		    // bSourceId
    0x01,		            // bControlSize
    0x01,		            // master mute
    0x02,		            // channel 1 volume
    0x02,		            // channel 2 volume
    0x00,                   // iFeature
*/    
    // ============================================ 
    // Output Terminal Descriptor
    // =========================================== 
    
    0x09,		        // bLength
    0x24,		        // bDescriptorType
    0x03,	    	    // bDescriptorSubtype
    DEV_SPK_OT_ID, 	    // bTerminalID
    0x01, 0x03,	        // wTerminalType, generic speaker		
    0x00,		        // bAssocTerminal		
    DEV_HOST_IT_ID, //DEV_SPK_FU_ID,      // bSourceID
    0x00,               // iTerminal
    

    // Speaker streaming interface
    // ============================================ 	
    // Standard AS Interface Alternate Setting 0 zero bandwidth
    // =========================================== 
    
    SIZEOF_INTERFACE_DESCRIPTOR,	// bLength
    DESC_TYPE_INTERFACE,			// bDescriptorType
    DEV_SPK_IF_ID,					// bInterfaceNumber
    0x00,  											// bAlternateSetting
    0x00,               			// bNumEndpoints
    0x01,				            // bInterfaceClass
    0x02,				            // bInterfaceSubClass
    0x00,				            // bInterfaceProtocol
    0x00,				            // iInterface
    
    // ============================================ 
    // Standard AS Interface:  Alternate Setting 1
    // ===========================================
    
    SIZEOF_INTERFACE_DESCRIPTOR,	// bLength
    DESC_TYPE_INTERFACE,            // bDescriptorType
    DEV_SPK_IF_ID,					// bInterfaceNumber
    0x01,  												// bAlternateSetting
    0x02,               			// bNumEndpoints 带反馈一共有2个端点
    0x01,				// bInterfaceClass
    0x02,				// bInterfaceSubClass
    0x00,				// bInterfaceProtocol
    0x00,				// iInterface
    
    // ============================================ 
    // Class-specific AS General Interface Descriptor
    // ===========================================
    
    0x07,	 	        // bLength
    0x24,	 	        // bDescriptorType
    0x01,               // bDescriptorSubtype
    DEV_HOST_IT_ID,     // bTerminalLink
    0x01,               // bDelay 1 frame
    0x01, 0x00,			// wFormat PCM
    
    // ============================================ 
    // Data Type 1 Format Descriptor
    // ===========================================
    11,	 	        // bLength	
    0x24,	 	        // bDescriptorType
    0x02,               // bDescriptorSubtype
    0x01,               // bFormatType
    DEV_DESC_SPK_NUM_CHANNELS,  // bNrChannels
    DEV_DESC_SPK_SUBFRAMESIZE, 	// bSubFrameSize
    DEV_SPK_BIT_RES,            // bBitResolution
    0x01,						// bSamFreqType, 2 fixed frequency
    0x44,0xAc,0x00,	// iSamFreq, 44.1 KHZ  		
//    0x80,0xBB,0x00, // iSamFreq, 48 KHZ
	    
    // Endpoint Descriptor
    0x09,						// bLength
    0x05,						// bDescriptorType
    DEV_SPK_EP_ID ,	            // bEndpointAddress
    0x05,//Async Iso     //0x09,	// bmAttributes, adaptive 
	(BYTE )(OUTEP1_RSIZE >> 1),	// Max packet size
	(BYTE )(OUTEP1_RSIZE >> 9),	
    0x01,  					    // bInterval
    0x00,						// bRefresh
    0x82,						// bSynchAddress
    
    // Class-Specific Iso Audio Data Endpoint Descriptor
    0x07,	            // bLength
    0x25,	            // bDescriptorType
    0x01,	            // bDescriptor Subtype
    0x01,	            // bmAttributes, support sampling frequency
    0X00,	            // bLockDelayUnits
    0x00, 0x00,	        // wLockDelay

    // Endpoint   异步反馈 EP 3
		//AS Isochronous Synch Endpoint Descriptor
    0x09,						// bLength
    0x05,						// bDescriptorType
    0x82,	            // bEndpointAddress (Direction=IN EndpointID=3)
    01,	// TransferType=Isochronous  SyncType=None  EndpointType=Data  
	03,	// Max packet size
	00,	
    0x01,  					    // bInterval
    0x05,						// bRefresh
    0x00,						// bSynchAddress    

#ifdef _HID_ 
       
    //---------------- For HID ------------
    // start of HID interface
    DEV_SIZEOF_HID_DESC,
    DESC_TYPE_INTERFACE,	// interface descriptor type
    DEV_HID_IF_ID,			// Interface Number
    0x00,					// Altersetting 0
    0x01,					// Number of endpoints used by this interface
    0x03,					// HID Interfce Class Code
    0x00,					// subclass
    0x00,					// no protocol
    0x00,					// no string index
    
    // HID descriptor
    0x09,					// 
    0x21,					// HID descriptor type
    0x00,					// HID class 01.00
    0x01,					// MSB
    0x00,					// hardware target country, none
    0x01,					// Number of HID class descriptor to follow
    0x22,					// Report descriptor type to follow
    DEV_SIZEOF_HID_REPORT_DESC,	// sizeof(Report_Descriptor),
    0x00,					// MSB
    
    // (Endpoint descriptor)
    0x07,
    DESC_TYPE_ENDPOINT,		// endpoint descriptor type
    DEV_HID_EP_ID,			// endpoint address IN direction
    0x03,					// interrupt transfer type
    DEV_HID_MAX_PKT,		// max packet size
    0x00,                   // MSB
	DEV_HID_PRATE			// polling rate
#endif


};



// String descriptors
byte code AppDevLanguageIDDesc[DEV_LANGID_SIZE] = {
    DEV_LANGID_SIZE, DESC_TYPE_STRING, 0x09, 0x04
};

byte code AppDevCompanyStringDesc[DEV_COMPANY_STR_SIZE] = {
    DEV_COMPANY_STR_SIZE, DESC_TYPE_STRING, 'T',0,'e',0,'x',0,'a',0,'s',0,' ',
    0,'I',0,'n',0,'s',0,'t',0,'r', 0,
    'u',0,'m', 0, 'e',0,'n', 0, 't',0,'s', 0
};

byte code AppDevProductStringDesc[DEV_PROD_STR_SIZE] = {
    DEV_PROD_STR_SIZE, DESC_TYPE_STRING, 'T',0,'A',0,'S',0,' ',0,'1',0 ,'0' ,0 ,'2' ,0 ,'0' ,
    0 ,'A',0 ,' ',0 ,'E',0 ,'V' ,0 ,'M' ,0
};

// HID Report Desscriptor
byte code AppDevHidReportDesc[DEV_SIZEOF_HID_REPORT_DESC] =  {   
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    // For the speaker
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0xe9,                    //   USAGE (Volume Up)
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0xea,                    //   USAGE (Volume Down)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x95, 0x02,                    //   REPORT_COUNT (2)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x81, 0x42,                    //   INPUT (Data,Var,Abs, Null State)
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0xe2,                    //     USAGE (Mute)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x81, 0x06,                    //   INPUT (Data,Var,Rel)
    
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x05,                    //   REPORT_SIZE (5)
    0x81, 0x01,                    //   INPUT (Cnst)
    0xc0                           // END_COLLECTION
};


// HID Report descriptor
byte code AppDevHidDesc[DEV_SIZEOF_HID_DESC] = {
    // HID descriptor
   	DEV_SIZEOF_HID_DESC,					// 
    0x21,					// HID descriptor type
    0x00,					// HID class 01.00
    0x01,					// MSB
    0x00,					// hardware target country, none
    0x01,					// Number of HID class descriptor to follow
    0x22,					// Report descriptor type to follow
    DEV_SIZEOF_HID_REPORT_DESC,	// sizeof(Report_Descriptor),
    0x00,					// MSB
};


