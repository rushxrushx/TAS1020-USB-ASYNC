//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 
/*====================================================================
UsbDfu.C
Handler for USB DFU class
====================================================================*/
#define _USB_DFU_
#include <string.h>
#include <stddef.h>                                         
#include <Reg52.h>
#include "types.h"
#include "reg_stc1.h"
#include "Mmap.h"
#include "Usb.h"
#include "UsbEng.h"
#include "dfuMmap.h"
#include "UsbDfu.h"
#include "RomBoot.h"
#include "Eeprom.h"
#include "xdata.h"
#include "DevRef.h"
#include "Utils.h"
#include "I2C.h"
#include "delay.h"

//
// Max number of times to try to do acknowledge polling to the eeprom. 
//
#define min(a, b)       ((a) < (b) ? (a) : (b))

// Miscellaneous define, will move to some files
#define DEVICE_DESC_VENDOR_OFFSET	0x8

// For convience and used locally only. I am a fan of Visual C++ editor

// The DfuEeprgHeaderTemp is used only at the first data download
// After that the data will be overwriten by the next data sent from host
// It is used here for convenience
// Don't change the data type of DfuEeprgHeaderTemp
EEPROM_HEADER_STRUCT volatile xdata DfuEeprgHeaderTemp _at_ USB_DFU_EP0_XFERDATA_ADDR; 

// DFU mode device record
DFU_DEVICE_STRUCT data DfuDevice;
DFU_DATA_STRUCT volatile xdata DfuData _at_ USB_DFU_ADDR_START;

void dfuModeEnter();                    
void dfuDnload();
void dfuUpload();
void dfuErrStalledPkt();
void dfuCopyStatusData();
void dfuHwdStateMachine(void);
#define dfuCopy(source, dest, size)  memcpy(dest, source, size)
void dfuDnloadHeader();
void dfuDnloadData();
void dfuDnloadTarget();
void dfuEepromCopy(byte *Source , word Offset, word Size);
void dfuUploadTarget();
bit dfuWriteEeprom(word Offset, byte *pSource, byte bBytesWritten, byte i2cFlags);      


// DFU mode descriptor set
// Default DFU device descriptor in case there is no vendor device descriptor
byte code DfuDeviceDesc[SIZEOF_DEVICE_DESC] = 
{
    SIZEOF_DEVICE_DESC,           // blength
    DESC_TYPE_DEVICE,             // Device descriptor type
    DFU_BCDUSB_L, DFU_BCDUSB_H,   // bcdUSB
    DFU_BCLASSCODE,               // Application specific Class code
    DFU_BSUBCLASSCODE,            // DFU code
    0x00,                         // No class specific protocol
    DFU_BMAXPACKETSIZE0,          // max packet size for EP0
    DFU_IDVENDOR_L,               // Vendor ID
    DFU_IDVENDOR_H,
    DFU_IDPRODUCT_L,              // Product ID
    DFU_IDPRODUCT_H,
    DFU_BCDDEVICE_L,              // Release number in bcd
    DFU_BCDDEVICE_H,
    DFU_IMANF_STR_ID,             // iManufacture, index of string descriptor
    DFU_IPRODUTC_STR_ID,          // iProduct, index of string descriptor
    0x00,                         // No iSerialNumber
    0x01                          // One configuration for DFU
};

// Default Configuration descriptor block
byte code DfuConfigDesc[DFU_SIZEOF_CONFIG_BLOCK] = 
{
    SIZEOF_CONFIG_DESCRIPTOR,	    // bLength
    DESC_TYPE_CONFIG,					    // bDescriptorType
    (BYTE )(DFU_SIZEOF_CONFIG_BLOCK),    // wTotalLength
    (BYTE )(DFU_SIZEOF_CONFIG_BLOCK >> 8),	
    DFU_NUM_IF,								    // bNumInterfaces
    DFU_CONFIG_VALUE,					    // bConfigurationValue
    0x00,											    // iConfiguration
    CFG_DESC_ATTR_SELF_POWERED,	
    DFU_MAXPOWER,							    // MaxPower 100 mA
    
    // DFU EEPROM download alternating interface setting 
    SIZEOF_INTERFACE_DESCRIPTOR,  // size of interface descriptor
    DESC_TYPE_INTERFACE,
    DFU_IFNUMBER,                 // interface number
    DFU_IF_SETTING,           // Alternating setting
    0x00,                         // Only using control endpoint
    DFU_BCLASSCODE,
    DFU_BSUBCLASSCODE,
    0x00,                         // No specific protocol
    DFU_IIF_STR_ID,            // Index of string descriptor
    
    // DFU Functional Descriptor
    DFU_FUNC_DESC_SIZE,
    DFU_FUNC_DESC_TYPE,
    DFU_MANIFESTTOL_BIT | DFU_UPLOAD_CAP_BIT | DFU_DNLOAD_CAP_BIT,
    DFU_DETACH_TIMEOUT_L,
    DFU_DETACH_TIMEOUT_H,
    DFU_WTRANSFER_SIZE_L,
    DFU_WTRANSFER_SIZE_H,        
};

// String descriptors
// String descriptors
byte code DfuLangIDDesc[DFU_LANGID_SIZE] = {
    DFU_LANGID_SIZE, DESC_TYPE_STRING, 0x09, 0x04
};

byte code DfuManfStrDesc[DFU_MANF_STR_SIZE] = {
    DFU_MANF_STR_SIZE, DESC_TYPE_STRING, 'U',0,'n',0,'k',0,'n',0,'o',0,
        'w',0,'n',0
};

byte code DfuProductStrDesc[DFU_PROD_STR_SIZE] = {
    DFU_PROD_STR_SIZE, DESC_TYPE_STRING, 'U',0,'n',0,'k',0,'n',0,'o',0,
        'w',0,'n',0
};

byte code DfuEepromStrDesc[DFU_EEPROM_STR_SIZE] = {
    DFU_EEPROM_STR_SIZE, DESC_TYPE_STRING, 'E',0,'E',0,'P',0,'R',0,'O',0 ,'M' ,0
};

byte code DfuRamStrDesc[DFU_RAM_STR_SIZE] = {
    DFU_RAM_STR_SIZE, DESC_TYPE_STRING, 'R',0,'A',0,'M',0
};

byte code DfuOtherStrDesc[DFU_OTHER_STR_SIZE] = {
    DFU_OTHER_STR_SIZE, DESC_TYPE_STRING, 'O',0,'T',0,'H',0,'E',0,'R',0
};

// Poll time table 
code DFU_POLLSTATE_STRUCT DfuPollTimeTable[DFU_NUM_POLLSTATES] = {
    //  App handles GET STATUS for cases App DECTACH/IDLE
    //  { 0x00, 0x01, 0xFF, DFU_STATE_appDETACH},
    { 0x00, 0x00, 0x00, DFU_STATE_dfuIDLE},
    { 0x00, 0x00, 0x00, DFU_STATE_dfuDNLOAD_SYNC},
    { 0x00, 0x00, 0x00, DFU_STATE_dfuDNLOAD_IDLE},
    { 0x00, 0x00, 0x00, DFU_STATE_dfuMANIFEST_SYNC},
    { 0x00, 0x00, 0x00, DFU_STATE_dfuUPLOAD_IDLE},
    { 0x00, 0x00, 0x00, DFU_STATE_dfuERROR},
    { 0x00, 0x00, 0x20, DFU_STATE_dfuMANIFEST}                
};

/*====================================================================
dfuSetup(): Initialize for DFU operation
Input:  none
Output: none
Globals used: 
DfuStateMachine
Return: none
Not:
- Only called when RomRecord.state is ROM_BOOT_START or ROM_DFU_MODE
====================================================================*/
void dfuSetup(byte Target, byte *PollTimeTablePtr)
{
    
    // Store target
    DfuStateMachine.target = Target;
    
    // Default ROM DFU modes
    // Copy DFU default descriptors
    // Default device descriptor
    dfuCopy(DfuDeviceDesc, DfuData.deviceDesc, sizeof(DfuDeviceDesc));
    
    // Default configuration block descriptors
    dfuCopy(DfuConfigDesc, DfuData.configDesc, sizeof(DfuConfigDesc));
    
    // Default laguage ID string descriptor
    dfuCopy(DfuLangIDDesc, DfuData.langStrDesc, sizeof(DfuLangIDDesc));
    
    // Default vendor string descriptor
    dfuCopy(DfuManfStrDesc, DfuData.vendorStrDesc, sizeof(DfuManfStrDesc));
    
    // Default product string descriptor
    dfuCopy(DfuProductStrDesc, DfuData.prodStrDesc, sizeof(DfuProductStrDesc));
    
    // Default Interface string descriptor is RAM 
    dfuCopy(DfuRamStrDesc, DfuData.ifStrDesc, sizeof(DfuRamStrDesc));  
    if (DfuStateMachine.target == DFU_TARGET_EEPROM)
        dfuCopy(DfuEepromStrDesc, DfuData.ifStrDesc, sizeof(DfuEepromStrDesc));
    
    // Get polling time
    if (PollTimeTablePtr == 0)
    {
        // Default is Eeprom polling table 
        PollTimeTablePtr = (byte code *)DfuPollTimeTable;
    }
    
    // modify Vendor/Product IDs in the device descriptor if applied
    // Modify Ep0 maxpacket size if applied
    // Case device first start up
    switch (RomRecord.state)
    {
    case ROM_BOOT_START:
        // Note that DfuData.eepromHeaderData is invalid if App is running
        if ((DfuData.eepromHeaderData.dataType == EEPROM_DEVICE_TYPE) ||
            (DfuData.eepromHeaderData.dataType == EEPROM_APPCODE_UPDATING))
        {
            DfuData.deviceDesc[USB_DEVICE_DESC_IDVENDOR_OFFSET] = (byte )DfuData.eepromHeaderData.vendorId;
            DfuData.deviceDesc[USB_DEVICE_DESC_IDVENDOR_OFFSET + 1] = (byte )(DfuData.eepromHeaderData.vendorId >> 8);
            DfuData.deviceDesc[USB_DEVICE_DESC_IDPRODUCT_OFFSET] = (byte )DfuData.eepromHeaderData.productId;
            DfuData.deviceDesc[USB_DEVICE_DESC_IDPRODUCT_OFFSET + 1] = (byte )(DfuData.eepromHeaderData.productId >> 8);
            DfuData.configDesc[USB_CONFIG_DESC_MAXPOWER_OFFSET] = DfuData.eepromHeaderData.maxPower;
            
            // Check for Language String and update if applied
            if ((DfuData.eepromHeaderData.usbAttribute & EEPROM_SUPPORT_STR_BIT) &&
                (DfuData.eepromHeaderData.dataType == EEPROM_DEVICE_TYPE))
            {
                DfuData.tempBuffer[1] = EEPROM_HEADER_SIZE;
                dfuCopy((byte xdata *)(&DfuData.eepromHeaderData.headerChksum + DfuData.tempBuffer[1]),
                    DfuData.langStrDesc, 4);
                
                // Copy Vendor string if applied
                DfuData.tempBuffer[0] = 0;
                DfuData.tempBuffer[1] = *((byte *)(&DfuData.eepromHeaderData.headerChksum + EEPROM_HEADER_SIZE + 4));
                dfuCopy((byte *)(&DfuData.eepromHeaderData.headerChksum + EEPROM_HEADER_SIZE + 4),
                    DfuData.vendorStrDesc, DfuData.tempBuffer[1]);
                
                // Copy Product string if applied
                DfuData.tempBuffer[2] = *((byte *)(&DfuData.eepromHeaderData.headerChksum + EEPROM_HEADER_SIZE + 4 +
                    DfuData.tempBuffer[1] )); 
                dfuCopy((byte *)(&DfuData.eepromHeaderData.headerChksum + EEPROM_HEADER_SIZE + 4 + DfuData.tempBuffer[1]),
                    DfuData.prodStrDesc, DfuData.tempBuffer[2]); 
            }
            
            //
            // Disable upload for ROM boot DFU use.
            // 
            if (DfuStateMachine.target == DFU_TARGET_RAM)
                DFU_FUNC_DESC[DFU_FUNC_ATTRIB_OFFSET] &= ~DFU_UPLOAD_CAP_BIT;
        }
        break;
        
        // Case App changes state after App Detach  
    case ROM_DFU_MODE:   
        // Check if App provides device descriptor
        // Update vendor and product IDs if applied
        PARAMS_GETDEVDESC(&UsbRequest.xferLength);
        DEV_FUNCTION(DEV_GETDEVDESC, &DEV_SHARED_DATA);  
        dfuCopy(DEV_DATA_PTR, DfuData.deviceDesc, DEV_DATA_PTR[0]);
        
        // Check if App provides Config descriptor
        PARAMS_GETCONFIGDESC(&UsbRequest.xferLength, UsbRequest.hiwValue);
        DEV_FUNCTION(DEV_GETCONFIGDESC, &DEV_SHARED_DATA);
        dfuCopy(DEV_DATA_PTR, DfuData.configDesc, DEV_DATA_PTR[2] | (DEV_DATA_PTR[3] << 8));
        
        // Check for Language String and update if applied
        PARAMS_GETSTRDESC(&UsbRequest.xferLength, DFU_ILANG_STR_ID);
        DEV_FUNCTION(DEV_GETSTRDESC, &DEV_SHARED_DATA);
        dfuCopy(DEV_DATA_PTR, DfuData.langStrDesc, DEV_DATA_PTR[0]);
        
        // Check for Vendor String and update if applied
        PARAMS_GETSTRDESC(&UsbRequest.xferLength, DFU_IMANF_STR_ID);
        DEV_FUNCTION(DEV_GETSTRDESC, &DEV_SHARED_DATA);
        dfuCopy(DEV_DATA_PTR, DfuData.vendorStrDesc, DEV_DATA_PTR[0]);
        
        // Check for Product String and update if applied
        PARAMS_GETSTRDESC(&UsbRequest.xferLength, DFU_IPRODUTC_STR_ID);
        DEV_FUNCTION(DEV_GETSTRDESC, &DEV_SHARED_DATA);
        dfuCopy(DEV_DATA_PTR, DfuData.prodStrDesc, DEV_DATA_PTR[0]);
        
        // Check for Interface String and update if applied
        PARAMS_GETSTRDESC(&UsbRequest.xferLength, DFU_IIF_STR_ID);
        DEV_FUNCTION(DEV_GETSTRDESC, &DEV_SHARED_DATA);
        dfuCopy(DEV_DATA_PTR, DfuData.ifStrDesc, DEV_DATA_PTR[0]);
        break;
        
    default:
        // So whoever calls this routine is not in the right MODE 
        return;   
    }
    
    // Copy poll time table
    dfuCopy(PollTimeTablePtr, DfuData.pollTimeTable, DFU_POLLTIME_TABLE_SIZE);
    
    // Initialize state machine
//    dfuWritePattern((byte *)&DfuStateMachine, sizeof(DfuStateMachine), 0);
    dfuInitStateMachine();
    
    DfuStateMachine.target = Target; 
    
    // Initialize DFU device record and ROM state
    RomRecord.state = ROM_DFU_MODE;
    dfuWritePattern((byte *)&DfuDevice, sizeof(DfuDevice), 0);
    
    // switch PROGRAM to RAM
    if (DfuStateMachine.target == DFU_TARGET_RAM)
    {
        USBCTL |= 0x01;       // Ser SDW bit confirm
        MEMCFG &= ~MEMCFG_SDW_BIT;
        USBCTL &= ~0x01;
    }
    
    DfuStateMachine.state = DFU_STATE_dfuIDLE;
    
    // Turn on external interrupt
    GLOBCTL |= 0x40;
    
    // Setup control endpoint and connect to USB bus
    UsbRequest.ep0MaxPacket = DfuData.deviceDesc[USB_DEVICE_DESC_CONFIG_SIZE_OFFSET];
    engUsbInit();
    USBIMSK |= 0x10;    // SOF INT on	
    
    while(DfuDevice.configSetting == 0);
    
    // Loop here until mode is off which means the application is o
    while (RomRecord.state == ROM_DFU_MODE)
    {   
        if (GET_BIT(EngParms.bitFlags,ENG_BIT_SUSPEND))
        {
            // if tagert is RAM, ROM handle SUSPENSE
            // as there is no APP
            if (DfuStateMachine.target == DFU_TARGET_RAM)
            {
                PCON = 0x09;
                while (GET_BIT(EngParms.bitFlags,ENG_BIT_SUSPEND));
            }
            else
                DEV_FUNCTION(DEV_USERSUSPENSE, &DEV_SHARED_DATA);     
        }
        
        // in case the device is manifesting
        if ((DfuStateMachine.state == DFU_STATE_dfuMANIFEST) &&
            (DfuStateMachine.mnfState == DFU_MNF_PHASE_IN_PROG))
        {
            // Interact with the application
            if (DfuStateMachine.target == DFU_TARGET_OTHER)
            {
                DEV_FUNCTION(DEV_DFUCHKMANIFEST, &DEV_SHARED_DATA);
                DfuStateMachine.mnfState = DEV_DFU_MNFSTATE;
            }
            // For eeprom or RAM which handles completely by ROM
            else
                DfuStateMachine.mnfState = DFU_MNF_PHASE_COMPLETED;
        }
    };
    
    // Check again if target is RAM
    // Reset for App as App is now in RAM area
    // not program area
    switch (DfuStateMachine.target)
    {
    case DFU_TARGET_RAM:
//        USBCTL = 0;
//        USBFADR = 0;
        UtilResetCPU();
        
    case DFU_TARGET_EEPROM:
//        USBCTL = 0;
//        USBFADR = 0;
#ifdef _EMULATOR_
        UtilResetCPU();
#else
        UtilResetBootCPU();
#endif  
    default:
        break;
    }
    
}

/*====================================================================
dfuHandler(): Handle DFU request events
Input:  none
Output: none
Globals used:
UsbRequest
EngParms
Return: none
Note: 
- Other hardware events such as Reset, Timer out, ... are not handled 
in this routine.
====================================================================*/
void dfuHandler(void)
{  
    // Normally, the status is set to error in
    // dfuErrStalledPkt()
    
    // Dfu States that do not accept any request
    switch(DfuStateMachine.state)
    {
    case DFU_STATE_dfuDNBUSY:
    case DFU_STATE_dfuMANIFEST:
    case DFU_STATE_dfuMANIFEST_WAIT_RESET:
        dfuErrStalledPkt();
        return;
        break;
    default:
        break;
    }
    
    // Handle the common DFU Get Status and State
    switch(UsbRequest.bRequest)
    {
    case DFU_GETSTATUS:
        // If DFU_STATE_app_IDLE or DFU_STATE_app_DETACH
        // let application supports this
        if ((DfuStateMachine.state == DFU_STATE_appDETACH) ||
            (DfuStateMachine.state == DFU_STATE_appIDLE))
        {
            PARAMS_DFUGETSTATUS(DfuStateMachine.state, DfuStateMachine.status);
            DEV_FUNCTION(DEV_DFUGETSTATUS, &DEV_SHARED_DATA);
            UsbRequest.xferLength = sizeof(DFU_STATUS_STRUCT);
            UsbRequest.dataPtr = DEV_DATA_PTR;
            break;
        }
        
        UsbRequest.xferLength = sizeof(DFU_STATUS_STRUCT);
        UsbRequest.dataPtr = DfuData.getBuffer;
        
        // Note as we always have block transfer completed before we receive
        // another GET STATUS request from host, so we never get into this case
        //    if ((DfuStateMachine.state == DFU_STATE_dfuDNLOAD_SYNC) &&
        //        (Block Tranfer Not Completed))
        //    {
        //      DfuStateMachine.state = DFU_STATE_dfuDNBUSY;
        //    }
        
        if (DfuStateMachine.state == DFU_STATE_dfuDNLOAD_SYNC)
        {
            DfuStateMachine.state = DFU_STATE_dfuDNLOAD_IDLE;
        }
        else if (DfuStateMachine.state == DFU_STATE_dfuMANIFEST_SYNC)
        {
            
            if (DfuStateMachine.mnfState == DFU_MNF_PHASE_IN_PROG)
            {
                DfuStateMachine.state = DFU_STATE_dfuMANIFEST;
                dfuCopyStatusData();
                DfuStateMachine.timer0 = DfuData.getBuffer[1];
                DfuStateMachine.timer = (DfuData.getBuffer[2] << 8) | DfuData.getBuffer[3];
                
                // at least give timer 1 milisec
                if ((DfuStateMachine.timer == 0) && (DfuStateMachine.timer0 == 0))
                    DfuStateMachine.timer = DFU_MNF_TIMER_MIN;
            }
            else if ((DfuStateMachine.mnfState == DFU_MNF_PHASE_COMPLETED) &&
                (DFU_FuncManTolBit))
            {
                DfuStateMachine.state = DFU_STATE_dfuIDLE;
            }
            else
                dfuErrStalledPkt();
        }
        
        // Copy status data to FIFO
        dfuCopyStatusData();
        DfuData.getBuffer[0] = DfuStateMachine.status;
        DfuData.getBuffer[4] = DfuStateMachine.state;
        
        break;
        
    case DFU_GETSTATE:
        UsbRequest.dataPtr = &DfuStateMachine.state;
        UsbRequest.xferLength = 1;
        break;
        
    case DFU_DETACH:
        if (DfuStateMachine.state == DFU_STATE_appIDLE)
        {
            // Set to next state
            DfuStateMachine.state = DFU_STATE_appDETACH;
            RomRecord.state = ROM_APP_DETACH;
            
            // start interupt timer here using STOF ISR for count down
            // worked when ROmRecord is in ROM_DFU_MODE or ROM_APP_DETACH
            DfuStateMachine.timer0 = 0;
            DfuStateMachine.timer = (word )UsbRequest.lowValue | (word )(UsbRequest.hiwValue << 8);
        }
        else
            dfuErrStalledPkt();
        break;
        
    case DFU_DNLOAD:
        switch (DfuStateMachine.state)
        {
        case DFU_STATE_dfuIDLE:
            // Case wLength > 0 and bitCanDnload = 1
            if (UsbRequest.wLength && (DFU_FuncDnLoadCapBit))
            {
                // Start of download the firs block
                DfuStateMachine.mnfState = DFU_MNF_NOT;
                DfuStateMachine.loadStatus = DFU_LOAD_NOT;
                
                // case TARGET_OTHER, informs application that 
                // this is the start
                if (DfuStateMachine.target == DFU_TARGET_OTHER)
                    DEV_FUNCTION(DEV_DFUDNLOAD_START, &DEV_SHARED_DATA);
                
                dfuDnload();
            }
            else
                dfuErrStalledPkt();
            break;
            
        case DFU_STATE_dfuDNLOAD_IDLE:
            if (UsbRequest.wLength)
            {
                // Continue download
                dfuDnload();
            }
            else 
            {
                // End of download
                if (DfuStateMachine.loadStatus != DFU_LOAD_COMPLETED)
                {
                    if (DfuStateMachine.status == DFU_STATUS_OK)
                    {
                        DfuStateMachine.state = DFU_STATE_dfuERROR;
                        DfuStateMachine.status = DFU_STATUS_errNOTDONE;
                    }
                }
                else
                {
                    if (DfuStateMachine.status == DFU_STATUS_OK)
                    {
                        DfuStateMachine.state = DFU_STATE_dfuMANIFEST_SYNC;
                        DfuStateMachine.mnfState = DFU_MNF_PHASE_IN_PROG;
                    }
                }
            }
            break;
            
        default:
            dfuErrStalledPkt();
            break;
        }
        break;
        
    case DFU_UPLOAD:
        switch (DfuStateMachine.state)
        {
        case DFU_STATE_dfuIDLE:
            // Case wLength > 0 and bitCanUpload = 1
            if (UsbRequest.wLength && (DFU_FuncUpLoadCapBit))                    
            { 
                
                DfuStateMachine.loadStatus = DFU_LOAD_NOT;
                // case TARGET_OTHER, informs application that 
                // this is the start
                if (DfuStateMachine.target == DFU_TARGET_OTHER)
                    DEV_FUNCTION(DEV_DFUUPLOAD_START, &DEV_SHARED_DATA);
                
                // Start of upload block 
                dfuUpload();
                
                if (DfuStateMachine.status == DFU_STATUS_OK)
                    DfuStateMachine.state = DFU_STATE_dfuUPLOAD_IDLE;
            }
            else
                dfuErrStalledPkt();
            break;
            
        case DFU_STATE_dfuUPLOAD_IDLE:
            if (UsbRequest.wLength)
            {
                // Continue Upload
                dfuUpload();
            }
            else
            {
                DfuStateMachine.state = DFU_STATE_dfuIDLE;
            }
            break;
            
        default:
            dfuErrStalledPkt();
            break;
        }
        break;
        
    case DFU_CLRSTATUS:
        if (DfuStateMachine.state == DFU_STATE_dfuERROR)
        {
            DfuStateMachine.state = DFU_STATE_dfuIDLE;
            DfuStateMachine.status = DFU_STATUS_OK;
            if (DfuStateMachine.target != DFU_TARGET_EEPROM)
                DfuStateMachine.loadStatus = DFU_LOAD_NOT;
        }
        else
            dfuErrStalledPkt();
        break;
        
    case DFU_ABORT:
        switch (DfuStateMachine.state)
        {
            // firmware may be corrupted???
        case DFU_STATE_dfuDNLOAD_IDLE:
        case DFU_STATE_dfuIDLE:
        case DFU_STATE_dfuUPLOAD_IDLE:
            DfuStateMachine.state = DFU_STATE_dfuIDLE;
            DfuStateMachine.status = DFU_STATUS_OK;
            if (DfuStateMachine.target != DFU_TARGET_EEPROM)
                DfuStateMachine.loadStatus = DFU_LOAD_NOT;
            break;
            
        default:    
            dfuErrStalledPkt();
            break;
        } 
        break;
        
    default:
        dfuErrStalledPkt();
        break;
    }
    return;
}

/*====================================================================
dfuHwdStateMachine(void): The DFU state machine when receiving the 
hardware signal events: USB reset and DFU timer timeout 
Input:  none
Output: none
Globals used:
UsbRequest
EngParms
Return: none
Note: 
- Other Host Request events are handled by dfuHandler
- The DFU timer is based on the SOF_INT: When timer
is expired, the SOF_INT isr call this routine.
- For USB reset, when the device in half DFU mode, the
RSTR_INT ISR call this routine.
====================================================================*/
void dfuHwdStateMachine(void)
{
    // Case App is running
    DfuStateMachine.timer0 = 0;
    DfuStateMachine.timer = 0;
    switch (DfuStateMachine.state)
    {
    case DFU_STATE_appIDLE:
        break;
        
    case DFU_STATE_appDETACH:
        switch(UsbRequest.intSource)
        {
        case RSTR_INT:
            RomRecord.state = ROM_DFU_MODE;
            break;
            
            // Timer out using SOF_INT
        case SOF_INT:
            RomRecord.state = ROM_APP_RUNNING;
            DfuStateMachine.state = DFU_STATE_appIDLE;
            DfuStateMachine.status = DFU_STATUS_OK;
            break;
            
        default:
            // There must be some firmware bug or hardware error 
            break;
        }
        break;
        
    case DFU_STATE_dfuIDLE:
        // make sure the device is configured
        // as we may get unexpected ISR
        if (DfuDevice.configSetting == 0)
            break;
    
            /*	case DFU_STATE_dfuIDLE:
            case DFU_STATE_dfuDNLOAD_SYNC:		
            case DFU_STATE_dfuDNBUSY: 	 
            case DFU_STATE_dfuDNLOAD_IDLE:
            case DFU_STATE_dfuMANIFEST_SYNC:
            case DFU_STATE_dfuMANIFEST:
            case DFU_STATE_dfuMANIFEST_WAIT_RESET:
            case DFU_STATE_dfuUPLOAD_IDLE:
            case DFU_STATE_dfuERROR:
        */
    default:
        if ((UsbRequest.intSource == SOF_INT) && (DfuStateMachine.state == DFU_STATE_dfuMANIFEST))
        { 
            if (DFU_FuncManTolBit)
                DfuStateMachine.state = DFU_STATE_dfuMANIFEST_SYNC;
            else
                DfuStateMachine.state = DFU_STATE_dfuMANIFEST_WAIT_RESET;
        }
        else if (UsbRequest.intSource == RSTR_INT)
        {
            USBCTL = 0;
            switch (DfuStateMachine.loadStatus)
            {
            case DFU_LOAD_COMPLETED:
                DfuStateMachine.state = DFU_STATE_appIDLE;
                RomRecord.state = ROM_APP_RUNNING;
                break;
            
            case DFU_LOAD_NOT:
                if (DfuStateMachine.target == DFU_TARGET_RAM)
                {
                    engUsbInit();
                }
                else
                {
                    DfuStateMachine.state = DFU_STATE_appIDLE;
                    RomRecord.state = ROM_APP_RUNNING;
                }
                break;
            
            default:
                DfuStateMachine.state = DFU_STATE_dfuERROR; 
                DfuStateMachine.status = DFU_STATUS_errPOR;
                engUsbInit();
                break;
            }
        }
        break;
    }
}

/*====================================================================
dfuErrStalledPkt(): Set the DFU state machine to reflect the
stalled packet error
Input:  none
Output: none
Globals used:
UsbRequest
EngParms
Return: none
====================================================================*/
void dfuErrStalledPkt()
{
    UsbRequest.status = EVENT_ERROR;
    if ((DfuStateMachine.state == DFU_STATE_appIDLE) || 
        (DfuStateMachine.state == DFU_STATE_appDETACH))
    {
        DfuStateMachine.state = DFU_STATE_appIDLE;
        DfuStateMachine.status = DFU_STATUS_OK;
    }
    else
    {
        DfuStateMachine.state = DFU_STATE_dfuERROR;
        DfuStateMachine.status = DFU_STATUS_errSTALLEDPKT;
    }
}

/*====================================================================
dfuCopyStatusData(): Copy tstaus polling time table to buffer t be sent
Input:  none
Output: none
Globals used:
UsbRequest
EngParms
Return: none
====================================================================*/
void dfuCopyStatusData()
{
    byte temp;
    
    // search the temp table to find the related state
    for (temp = 0; temp < DFU_NUM_POLLSTATES; temp++)
    {
        if (DfuStateMachine.state == DfuData.pollTimeTable[temp].state)
            break;
    }
    
    // Copy only the poll time and state
    dfuCopy((byte xdata *)&DfuData.pollTimeTable[temp], (byte xdata *)(DfuData.getBuffer + 1), 
        sizeof(DFU_POLLSTATE_STRUCT));
    
    // iString not used
    DfuData.getBuffer[sizeof(DFU_STATUS_STRUCT) - 1] = 0;
    
}

/*====================================================================
dfuGetStrDesc(): Handle get string descriptors when device in DFU mode
Input:  none
Output: none
Globals used:
UsbRequest
EngParms
Return: none
====================================================================*/
void dfuGetStrDesc()
{
    switch(UsbRequest.lowValue)
    {
    case DFU_ILANG_STR_ID:
        UsbRequest.dataPtr = DfuData.langStrDesc;
        UsbRequest.xferLength = DFU_LANGID_SIZE;
        break;
        
    case DFU_IMANF_STR_ID:
        UsbRequest.dataPtr = DfuData.vendorStrDesc;
        UsbRequest.xferLength = DfuData.vendorStrDesc[0];
        break;
        
    case DFU_IPRODUTC_STR_ID:
        UsbRequest.dataPtr = DfuData.prodStrDesc;
        UsbRequest.xferLength = DfuData.prodStrDesc[0];
        break;
        
    case DFU_IIF_STR_ID:
        UsbRequest.dataPtr = DfuData.ifStrDesc;
        UsbRequest.xferLength = DfuData.ifStrDesc[0];
        break;
        
    default:
        break;
    }
}

/*====================================================================
dfuDnload(): function to program the data downloaded from host
Input:  none
Output: none
Globals used:
Return: none
Note:
- Data address is predefined in Mmap.h
- The download files except the case for DFU_TARGET_OTHER,
should contains the eerpom header
====================================================================*/
void dfuDnload()
{
    switch(DfuStateMachine.target)
    {
    case DFU_TARGET_OTHER:
        PARAMS_DFUDNLOAD(UsbRequest.wLength, (unsigned char xdata *)USB_EP0_XFERDATA);
        DEV_FUNCTION(DEV_DFUDNLOAD, &DEV_SHARED_DATA);
        UsbRequest.status = DEV_DFU_USB_STATUS;
        DfuStateMachine.status = DEV_DFU_STATUS;
        DfuStateMachine.loadStatus = DEV_DFU_LOAD_STATUS;
        break;
        
    case DFU_TARGET_RAM:
    case DFU_TARGET_EEPROM:
        dfuDnloadTarget();
        break;
        
    default:
        UsbRequest.status = EVENT_ERROR;
        break;
    }
    
    if (UsbRequest.status == EVENT_OK)
        DfuStateMachine.state = DFU_STATE_dfuDNLOAD_SYNC;
    else
    {
        if (DfuStateMachine.status == DFU_STATUS_OK)
        {
            DfuStateMachine.status = DFU_STATUS_errPROG;
            DfuStateMachine.state = DFU_STATE_dfuERROR;
        }
    }
}

/*====================================================================
dfuDnloadTarget(): Handle download data to RAM
Return: none
Note : 
====================================================================*/
void dfuDnloadTarget()
{
    // For the first download, strip the header off
    switch (DfuStateMachine.loadStatus)
    {
    case DFU_LOAD_NOT: 
        DfuStateMachine.loadStatus = DFU_LOAD_HEADER;
        DfuStateMachine.headerCount = DfuEeprgHeaderTemp.headerSize;
        DfuStateMachine.bufferAddr = PROG_RAM_ADDR_START;
        DfuStateMachine.dataRemain = DfuEeprgHeaderTemp.payloadSize;  
    case DFU_LOAD_HEADER:
        dfuDnloadHeader();
        break;
        
    case DFU_LOAD_NOW:
        dfuDnloadData();
        break;
        
    default:
        break;
    }
}

/*====================================================================
dfuDnloadHeader(): Case still down load header
Return: none
Note : 
====================================================================*/
void dfuDnloadHeader()
{
    // Save header data
    if (DfuStateMachine.headerCount > EngParms.dataCount)
    {
        dfuCopy((byte xdata *)USB_EP0_XFERDATA, 
            (byte xdata *)(&DfuData.eepromHeaderData.headerChksum + DfuStateMachine.bufferAddr),
            EngParms.dataCount);
        DfuStateMachine.headerCount -= EngParms.dataCount; 
        DfuStateMachine.bufferAddr += EngParms.dataCount;
    }
    else
    {
        // Copy the remaining of the header data
        dfuCopy((byte xdata *)USB_EP0_XFERDATA, 
            (byte xdata *)(&DfuData.eepromHeaderData.headerChksum + DfuStateMachine.bufferAddr),
            DfuStateMachine.headerCount);
        
        DfuStateMachine.bufferAddr += DfuStateMachine.headerCount;
        
        // Check header
        if (eepromCheckFirmware((byte xdata *)&DfuData.eepromHeaderData.headerChksum) == FALSE)
        {
            DfuStateMachine.status = DFU_STATUS_errFILE;
            DfuStateMachine.state = DFU_STATE_dfuERROR;
        }
        // Modify the data type of the eeprom as a check for dirty bit
        else
        {
            // if EEPROM_HEADER_OVERWRITE bit set, program the eeprom header
            // this feature is used for programming the blank eeprom which
            // is usefull for production if applied
            if (DfuData.eepromHeaderData.attribute & EEPROM_HEADER_OVERWRITE)
            {
                dfuEepromCopy((byte xdata *)&DfuData.eepromHeaderData.headerChksum , 
                    0, sizeof(EEPROM_HEADER_STRUCT));  
            }
            
            if (DfuStateMachine.target == DFU_TARGET_EEPROM)
            {
                DfuData.tempBuffer[0] = EEPROM_APPCODE_UPDATING;
                dfuEepromCopy((byte xdata *)DfuData.tempBuffer , 
                    offsetof(EEPROM_HEADER_STRUCT,dataType), 1); 
            }     
        }
        
        // Now copy the data after the header data
        if (DfuStateMachine.target == DFU_TARGET_RAM)
        {
            DfuStateMachine.bufferAddr = 0;
        }
        dfuDnloadData();      
        DfuStateMachine.headerCount = 0;
        DfuStateMachine.loadStatus = DFU_LOAD_NOW;
    }
}

/*====================================================================
dfuDnloadData(): Case download data to RAM
Return: none
Note : 
====================================================================*/
void dfuDnloadData()
{
    // Check if data remaining, data sent minus header are still valid
    if (DfuStateMachine.dataRemain >= (EngParms.dataCount - DfuStateMachine.headerCount))
    {
        // target is RAM
        if (DfuStateMachine.target == DFU_TARGET_RAM)
        {
            dfuCopy((byte xdata *)(USB_EP0_XFERDATA + DfuStateMachine.headerCount),
                (byte xdata *)DfuStateMachine.bufferAddr, EngParms.dataCount - DfuStateMachine.headerCount);
        }
        // target is EEPROM
        else
        {
            dfuEepromCopy((byte xdata *)(USB_EP0_XFERDATA + DfuStateMachine.headerCount), 
                DfuStateMachine.bufferAddr,
                EngParms.dataCount - DfuStateMachine.headerCount);
        }
        DfuStateMachine.bufferAddr += (EngParms.dataCount - DfuStateMachine.headerCount);
        DfuStateMachine.dataRemain -= (EngParms.dataCount - DfuStateMachine.headerCount);
    }
    // Here comes the error as data remaining is less than data sent
    // Do some thing???
    else
    {
        DfuStateMachine.status = DFU_STATUS_errFILE;
        DfuStateMachine.state = DFU_STATE_dfuERROR;
        DfuStateMachine.loadStatus = DFU_LOAD_ERROR;
        return;
    } 
    
    // Case download completed
    if (DfuStateMachine.dataRemain == 0)
    { 
        // Program the data payload, data type, checksum
        if (DfuStateMachine.target == DFU_TARGET_EEPROM)
        {
            // Checksum
            dfuEepromCopy((byte xdata *)&DfuData.eepromHeaderData.headerChksum,0, 1);
            
            // Data type
            dfuEepromCopy((byte xdata *)&DfuData.eepromHeaderData.dataType, 
                offsetof(EEPROM_HEADER_STRUCT,dataType) , sizeof(DfuData.eepromHeaderData.dataType));
            
            // pay load
            dfuEepromCopy((byte xdata *)&DfuData.eepromHeaderData.payloadSize,
                offsetof(EEPROM_HEADER_STRUCT, payloadSize), sizeof(DfuData.eepromHeaderData.payloadSize));
        }
        
        // So download completed
        if (DfuStateMachine.status == DFU_STATUS_OK)
            DfuStateMachine.loadStatus = DFU_LOAD_COMPLETED;
        else
            DfuStateMachine.loadStatus = DFU_LOAD_ERROR;
    }   
}

/*====================================================================
dfuEepromCopy(): Copy with checking
Input: 
Output:
Return: none
====================================================================*/
void dfuEepromCopy(byte *Source , word Offset, word Size)
{
    BYTE bPageSize = DfuData.eepromHeaderData.wPageSize;
    BYTE bBytesWritten;
    BYTE i2cFlags;
    
    if (!Size)
        return;
    
    
    i2cFlags = I2C_WRITE | I2C_START | I2C_STOP;
    
    if (DfuData.eepromHeaderData.attribute & EEPROM_SUPPORT_400_KHZ)
        i2cFlags |= I2C_400_KHZ;
    
    if (RomRecord.attribute & ROM_EEPROM_WORD_ACCESS_MODE)
        i2cFlags |= I2C_WORD_ADDR_TYPE;
    
    //
    // First we need to determine if we are in the middle of a page. If we are,
    // we will right the partial first page and then continue writing the 
    // following pages.
    //
    
    //
    // Determine the number of pages in the first page. It could be a partial
    // page.
    //
    
    bBytesWritten = min(bPageSize - (Offset % bPageSize), Size);
    
    if (!dfuWriteEeprom(Offset, Source, bBytesWritten, i2cFlags))
    {
        DfuStateMachine.status = DFU_STATUS_errPROG;
        DfuStateMachine.state = DFU_STATE_dfuERROR;      
        return;
    }
    
    
    Offset += bBytesWritten;
    Size -= bBytesWritten;
    Source += bBytesWritten;
    
    while (Size)
    {
        bBytesWritten = min(Size, bPageSize);
        
        if (!dfuWriteEeprom(Offset, Source, bBytesWritten, i2cFlags))
        {
            DfuStateMachine.status = DFU_STATUS_errPROG;
            DfuStateMachine.state = DFU_STATE_dfuERROR;      
            return;
        }
        
        Offset += bBytesWritten;
        Size -= bBytesWritten;
        Source += bBytesWritten;
        
    }
    
}

//
// dfuWriteEeprom - Does a sequential write of default eeprom. Acknowledge
//      polling is done prior to the transfer.
//
// Input:
//          Offset - Offset within eeprom.
//          Source - ptr to source buffer.
//          bByteWritten - # of bytes to write.
//          i2cFlags - I2CAccess flags.
//
// Returns 1 if succesful, 0 o.w.
//
bit dfuWriteEeprom(word Offset, byte *pSource, byte bBytesWritten, byte i2cFlags)
{
    byte i;
    
    //
    // Do Acknowlege polling to make sure EEPROM is ready for more data.
    //
    
    i2cFlags |= I2C_START;
    i2cFlags &= ~I2C_STOP;
    SET_I2C(EEPROM_I2C_ADDR, Offset, 0, 0, i2cFlags);
    
    for (i = 0; i < I2C_MAX_ACK_TRIES; i++)
    {
        if (I2CAccess())
            break;
    }
    
    if (i >= I2C_MAX_ACK_TRIES)
        return 0;
    
    i2cFlags &= ~I2C_START;
    i2cFlags |= I2C_STOP;
    
    SET_I2C(EEPROM_I2C_ADDR, Offset, pSource, bBytesWritten, i2cFlags);
    
    if (!I2CAccess())
        return 0 ;
    
    return 1; 
}

/*====================================================================
dfuUpload(): function to upload the data to host
Input:  none
Output: none
Globals used:
Return: none
Note:
- Data address is predefined in Mmap.h
====================================================================*/
void dfuUpload()
{
    switch(DfuStateMachine.target)
    {
    case DFU_TARGET_OTHER:
        PARAMS_DFUUPLOAD(UsbRequest.wLength,(unsigned char xdata *)USB_EP0_XFERDATA);
        DEV_FUNCTION(DEV_DFUUPLOAD, &DEV_SHARED_DATA);
        UsbRequest.status = DEV_DFU_USB_STATUS;
        DfuStateMachine.status = DEV_DFU_STATUS;
        UsbRequest.xferLength = DEV_DFU_XFER_LENGTH;
        if (DfuStateMachine.status == DFU_STATUS_OK)
        {
            UsbRequest.dataPtr = USB_EP0_XFERDATA;
            // Upload completed
            if (UsbRequest.xferLength < UsbRequest.wLength)
                DfuStateMachine.state = DFU_STATE_dfuIDLE;
        }
        else
            DfuStateMachine.state = DFU_STATE_dfuERROR;
        break;
        
    case DFU_TARGET_RAM:
    case DFU_TARGET_EEPROM:
        switch(DfuStateMachine.state)
        {
        case DFU_STATE_dfuIDLE:
            
            // Initialize address, size for upload
            DfuStateMachine.bufferAddr = 0;
            DfuStateMachine.timer0 = 0;
            DfuStateMachine.timer = 0;
            DfuStateMachine.dataRemain = PROG_RAM_SIZE;
            if (DfuStateMachine.target == DFU_TARGET_EEPROM)
            {      
                // Get the header
                eepromExist();
                DfuStateMachine.dataRemain = DfuData.eepromHeaderData.payloadSize + DfuData.eepromHeaderData.headerSize;
            }
            dfuUploadTarget();
            break;
            
        case DFU_STATE_dfuUPLOAD_IDLE:
            dfuUploadTarget();
            break;
            
        default:
            break;
        }
        break;
        
        default:
            UsbRequest.status = EVENT_ERROR;
            break;
    }
    return;
}

/*====================================================================
dfuUploadTarget(): Copy data for sending to host
Input: 
Return: none
Note : 
====================================================================*/
void dfuUploadTarget()
{ 
    byte i2cFlag;
    word size;
    word dataCount;
    
    UsbRequest.xferLength = UsbRequest.wLength;
    
    // Check if end upload data
    if (DfuStateMachine.dataRemain < UsbRequest.xferLength)
    {  
        UsbRequest.xferLength = DfuStateMachine.dataRemain;
        DfuStateMachine.state = DFU_STATE_dfuIDLE;
        DfuStateMachine.status = DFU_STATUS_OK; 
        //    return;
    }
    
    switch(DfuStateMachine.target)
    {
    case DFU_TARGET_RAM:
        dfuCopy((unsigned char xdata *)DfuStateMachine.bufferAddr, (unsigned char xdata *)USB_EP0_XFERDATA, 
            UsbRequest.xferLength); // HN&SBH changed wlength to xferlength
        
        DfuStateMachine.bufferAddr += UsbRequest.xferLength;
        DfuStateMachine.dataRemain -= UsbRequest.xferLength;
        
        break;
        
    case DFU_TARGET_EEPROM:
        i2cFlag = I2C_READ | I2C_START | I2C_STOP;
        
        if (DfuData.eepromHeaderData.attribute & EEPROM_SUPPORT_400_KHZ)
            i2cFlag |= I2C_400_KHZ;
        
        if (RomRecord.attribute & ROM_EEPROM_WORD_ACCESS_MODE)
            i2cFlag |= I2C_WORD_ADDR_TYPE;
        
        dataCount = UsbRequest.xferLength;
        
        size = DfuData.eepromHeaderData.rPageSize;
        
        if ((size == 0) || (size >= UsbRequest.xferLength))
            size = UsbRequest.xferLength;
        
        
        while(dataCount)
        {
            SET_I2C (EEPROM_I2C_ADDR, DfuStateMachine.bufferAddr, 
                (unsigned char xdata *)USB_EP0_XFERDATA, 
                size, i2cFlag);
            
            if (I2CAccess()  == FALSE)
            {
                // Error read eeprom, do some thing here ???
                DfuStateMachine.status = DFU_STATUS_errPROG;
                DfuStateMachine.state = DFU_STATE_dfuERROR; 
                break;
            }
            else
            {
                DfuStateMachine.bufferAddr += size;
                DfuStateMachine.dataRemain -= size;
                dataCount -= size;
                if (size > dataCount)
                    size = dataCount;
            }
        }
        break;
        
    default:
        break;
    }
    
    if (DfuStateMachine.state != DFU_STATE_dfuERROR) 
    { 
        DfuStateMachine.status = DFU_STATUS_OK;
        UsbRequest.dataPtr = (byte xdata *)USB_EP0_XFERDATA;
    }
    
}

/*====================================================================
dfuInitStateMachine(): Initialize the state machine all to zero
Input: 
Return: none
Note : 
====================================================================*/
void dfuInitStateMachine()
{
    // Initialize state machine
    dfuWritePattern((byte *)&DfuStateMachine, sizeof(DfuStateMachine), 0);
}

/*====================================================================
dfuWritePattern(): Write with constant pattern
Input: buffer: pointers to source and destination buffers
size: size of buffer to be copied 
pattern: pattern to be written to buffer
Return: none
Note : 
====================================================================*/
void dfuWritePattern(byte *buffer, word size, byte pattern)
{
    for(; size; size--, buffer++)
        *buffer = pattern; 
}

