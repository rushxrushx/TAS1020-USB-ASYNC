//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 

/*==================================================================== 
UsbEng.c
Engine for USB communication.
micro type		: STC
====================================================================*/

#define _USB_ENGINE_
#include <string.h>
#include <Reg52.h>
#include "types.h"
#include "Reg_stc1.h"
#include "Mmap.h"
#include "hwMacro.h"
#include "RomBoot.h"
#include "UsbEng.h"
#include "Usb.h"
#include "UsbDfu.h"
#include "delay.h"
#include "devRef.h"

/*====================================================================
External 0 interrupt - stc
Note: this function handles the following sources of interrupts
- Packet Setup/Out/In Control Enpoint, endpoint 0.
- RSTR_INT, SUSR_INT, RESR_INT, XINT_INT, and SOF_INT.
- For others, devUserIntHandler() is called.
=====================================================================*/
engEx0() interrupt 0 { 
    
    UsbRequest.intSource = VECINT;
    EA = 0;  
    // case early connection as device is bus-powered
    if (RomRecord.state == ROM_BOOT_START)		 
    {
        if ((UsbRequest.intSource != SETUP_INT) &&
            (UsbRequest.intSource != IEP0_INT))
        {
            VECINT = 0;
            return;
        }
    }
    
    switch (UsbRequest.intSource) 
    {
    case OEP0_INT:
        VECINT=0; 
        engEp0RxDone();
        break;
        
    case IEP0_INT: 
        VECINT=0; 				
        engEp0TxDone(); 	
        break;
        
    case SETUP_INT : 
        engEp0SetupDone(); 
        break;
        
    case SUSR_INT: 
        engSuspendIntrHandler(); 
        VECINT = 0x00;
        break;
        
    case RESR_INT: 
        engResumeIntrHandler();
        VECINT = 0x00; 
        break;
        
    case XINT_INT: 
        VECINT = 0x00;
        engXintHandler(); 
        break;
        
    case RSTR_INT:
        USBFADR = 0;
        // use for DFU mode
        if ((RomRecord.state == ROM_APP_DETACH) || 
            (RomRecord.state == ROM_DFU_MODE))
        {
            dfuHwdStateMachine();
        }
        else
        {
            // application interrupt handler
            PARAMS_USERINTHANDLER(UsbRequest.intSource);
            DEV_FUNCTION(DEV_USERINTHANDLER, &DEV_SHARED_DATA); 
        } 
        VECINT=0;
        break;
        
    case SOF_INT:
        // use for DFU mode
        if ((RomRecord.state == ROM_DFU_MODE) ||
            (RomRecord.state == ROM_APP_DETACH))
        {
            // Here use Start of Frame as timer 
            // for DFU mode or maybe for anything
            // else ???
            if (DfuStateMachine.timer || DfuStateMachine.timer0)
            {
                if (DfuStateMachine.timer == 0)
                {
                    DfuStateMachine.timer = 0xFFFF;
                    DfuStateMachine.timer0--;
                }
                else
                {
                    DfuStateMachine.timer--;
                }
                
                if ((DfuStateMachine.timer == 0) && (DfuStateMachine.timer0 == 0))
                    dfuHwdStateMachine();
            }
        }
        else
        {
            // application interrupt handler
            PARAMS_USERINTHANDLER(UsbRequest.intSource);
            DEV_FUNCTION(DEV_USERINTHANDLER, &DEV_SHARED_DATA); 
        }
        VECINT=0;
        break;
        
    default: 
        // application interrupt handler
        PARAMS_USERINTHANDLER(UsbRequest.intSource);
        DEV_FUNCTION(DEV_USERINTHANDLER, &DEV_SHARED_DATA); 
        VECINT=0; 
        break;
    }
    EA = 1;
} 

/*====================================================================
engResumeIntrHandler(): External Interrupt service routine to 
handle remote wakeup.
=====================================================================*/
void engXintHandler (void)
{ 	 
    if (RomRecord.state == ROM_DFU_MODE)
    {
        if (DfuStateMachine.target == DFU_TARGET_RAM)
        {
            if (DfuDevice.devStatus[0] & DFU_STATUS_REMOTEWAKEUP)
            {
                USBCTL |= 0x20;
                USBCTL &= ~0x20;
                // Clocks already started..
            }
        }
    }
    else if (RomRecord.state == ROM_APP_RUNNING)
    {
        PARAMS_USERINTHANDLER(UsbRequest.intSource);
        DEV_FUNCTION(DEV_USERINTHANDLER, &DEV_SHARED_DATA); 
    }

    return;
}

/*====================================================================
engResumeIntrHandler(): Resume Interrupt service routine 
=====================================================================*/
void engResumeIntrHandler (void)
{
    
    if (RomRecord.state == ROM_DFU_MODE)
    {
        if (DfuStateMachine.target == DFU_TARGET_RAM)
        {
            // Clear the Suspend Variable
            CLEAR_BIT(EngParms.bitFlags,ENG_BIT_SUSPEND);
            
            // Clocks already started..
        }
    }
    else
    {
        PARAMS_USERINTHANDLER(UsbRequest.intSource);
        DEV_FUNCTION(DEV_USERINTHANDLER, &DEV_SHARED_DATA); 
    }
    return;
}

/*====================================================================
engSuspendIntrHandler(): Suspend Interrupt service routine 
=====================================================================*/
void engSuspendIntrHandler (void)
{
    // Set the Suspend Variable
    if (RomRecord.state == ROM_DFU_MODE)
    {
        if (DfuStateMachine.target == DFU_TARGET_RAM)
        {
            // Clear the Suspend Variable
            SET_BIT(EngParms.bitFlags,ENG_BIT_SUSPEND);
        }
    }
    else
    {
        PARAMS_USERINTHANDLER(UsbRequest.intSource);
        DEV_FUNCTION(DEV_USERINTHANDLER, &DEV_SHARED_DATA); 
    }
    
    return;
}

/*====================================================================
engEp0SetupDone(): Interrupt service routine for the control endpoint
when completed receiving the USB packet setup transaction.
Note: this routine is part of the USB state machine.
- VECINT is cleared in this routine.
=====================================================================*/
void engEp0SetupDone(void)
{
    // Reset all parameters
    // Clear Tall IN/OUT
    STALLClrInEp0;
    STALLClrOutEp0;
    
    // toggle DATA 0/1 or 1/0
    TOGGLEInEp0Data;
    TOGGLEOutEp0Data;
    
    // Empty/Flush EP0 Out/In FIFO
    EMPTYOutEp0;							
    EMPTYInEp0;
    
    // Clear Fatal error
    CLEAR_BIT(EngParms.bitFlags,ENG_BIT_EXCEPT);
    
    /*	// ??? for testing
    if (UsbRequest.wLength == 0x999) 
    UsbRequest.wLength = 0x40;
    */
    
    // Copy request
    UsbRequest.bmRequest = SETPACK[0];
    UsbRequest.bRequest = SETPACK[1];
    UsbRequest.lowValue = SETPACK[2];
    UsbRequest.hiwValue = SETPACK[3];
    UsbRequest.lowIndex = SETPACK[4];
    UsbRequest.hiwIndex = SETPACK[5];
    UsbRequest.wLength = SETPACK[6] | (SETPACK[7] << 8);
    
    // As the USB connection time riquirement
    // only the SET ADDRESS request is handled
    // Other returns with VECINT turn not cleared
    if (RomRecord.state == ROM_BOOT_START)
    {
        if ((UsbRequest.bmRequest != 0) || (UsbRequest.bRequest != USB_REQ_SET_ADDRESS))
        {
            // NAK IN/OUT off all other requests 
            USBIMSK = 0x00; 
            
            // AS VECINT is not cleared, the application will get an ISR for this SETUP
            // package again
            return;
        }
    }
    
    // Clear data tx/rx count
    EngParms.dataCount = 0;
    
    // handle the READ request	
    // engUsbClearEvent() is called by usbProtoColHandler()
    // to handle the result and USB state change
    if (UsbRequest.bmRequest & USB_REQ_TYPE_READ )
    {
        // USB_WAIT_SETUP state
        EngParms.state = USB_WAIT_FW_DATA;
        EngParms.event = USB_READ_EVENT;
        usbProtocolHandler();
        engUsbClearEvent();
        
        // In case host asks for more than expected
        if (UsbRequest.xferLength < UsbRequest.wLength)
            UsbRequest.wLength = UsbRequest.xferLength;
        
        // Make sure that if device has no data to send,
        // it will send zero data packet.
        UsbRequest.xferLength = UsbRequest.ep0MaxPacket;
        
        if (EngParms.state == USB_WAIT_DATA_IN)
            engLoadTxFifo();
    }
    
    // Case of WRITE request with 0 data		
    // engUsbClearEvent() is called by usbProtoColHandler()
    // to handle the result and USB state change
    else if (UsbRequest.wLength == 0)
    {
        EngParms.state = USB_WAIT_FW_RESULT;
        EngParms.event = USB_WRITE_EVENT;
        usbProtocolHandler();
        engUsbClearEvent();
    }
    
    // Case WRITE request with require more data		
    // usbProtoColHandler() is called when the device completes
    // receiving data from host. So the usbProtoColHandler() is called
    // in Ep0RxInt().
    else
    { 
        // Set machine waiting to receive data from host
        EngParms.state = USB_WAIT_DATA_OUT;
        EngParms.event = USB_WRITE_EVENT;
    }  
    
    // clear interrupt vector
    VECINT=0; 
}

/*====================================================================
engEp0TxDone(): Interrupt service routine for the control endpoint
when completed transmiting one USB data in transaction.
Note: this routine is part of the USB state machine.
=====================================================================*/
void engEp0TxDone(void)
{   
    // Check if we get this by STALL IN
    if ((EngParms.state == USB_WAIT_SETUP) || (IEPCNF0 & 0x08))
        return;
    
    // READ REQUEST
    if (EngParms.event == USB_READ_EVENT) 
    {
        // USB_WAIT_DATA_IN
        if (EngParms.state == USB_WAIT_DATA_IN)
        {
            // Call to send data to host if any
            engLoadTxFifo();
            
            // if no more data sent, to another state 		
            if (EngParms.xferStatus == ENG_TX_COMPLETE)
            {
                // wait for host Status/OUT ACK
                // will be checked in Ep0RxInt()
                EngParms.state = USB_WAIT_OUT_ACK;
                STALLInEp0;
                STALLClrOutEp0;
                EMPTYOutEp0;
            }
            // else keep in the same state and event
        }
        // Any thing else is error
        // either hardware interrupts or USB_WAIT_SETUP data error
        else
        {
            engInitWaiSetup();
        }
    }
    // WRITE REQUEST
    else if (EngParms.event == USB_WRITE_EVENT)
    { 		 
        if (EngParms.state == USB_WAIT_IN_ACK)
        { 			
            // got device Status/IN ACK 	
            engInitWaiSetup();
        }
        // case Address Ack at enumeration
        else if (EngParms.state == USB_WAIT_ADDR_ACK)
        {
            // Set address
            USBFADR = UsbRequest.lowValue;
            engInitWaiSetup();
        }
        // Any thing else is error either hardware 
        // interrupts or USB_WAIT_SETUP data error
        else
            // Error: Reset to waiting for setup
            engInitWaiSetup();
    }
    // Any thing else is error either hardware 
    // interrupts or USB_WAIT_SETUP data error
    else
        // Error: Reset to waiting for setup
        engInitWaiSetup();	
}

/*====================================================================
engEp0RxDone(): Interrupt service routine for the control endpoint
when completed receiving one USB data out transaction.
Note: this routine is part of the USB state machine.
=====================================================================*/
void engEp0RxDone(void)
{
    // Check if we get this by STALL IN
    if ((EngParms.state == USB_WAIT_SETUP) || (OEPCNF0 & 0x08))
    {
        return;
    }
    // READ REQUEST
    if (EngParms.event == USB_READ_EVENT) 
    {
        // USB_WAIT_OUT_ACK 	 
        // USB_WAIT_DATA_IN: in case the host wants to terminate 
        // the READ sooner this is valid USB specs. 		
        if ((EngParms.state == USB_WAIT_OUT_ACK) || (EngParms.state == USB_WAIT_DATA_IN))
        {
            // Error: Reset to waiting for setup
            engInitWaiSetup();
        }
        // Any thing else is error
        // either hardware interrupts or USB_WAIT_SETUP data error	
        else
        {
            // Error: Reset to waiting for setup
            engInitWaiSetup();
        }
    }
    // WRITE REQUEST
    else if (EngParms.event == USB_WRITE_EVENT)
    {
        if (EngParms.state == USB_WAIT_DATA_OUT)
        { 
            // Save data and check if there are more
            engSaveRxFifo();
            
            // case we got an error
            if (EngParms.xferStatus == ENG_RX_ERROR)
            {
                // Error: Reset to waiting for setup
                engInitWaiSetup();
            }
            else if (EngParms.xferStatus == ENG_RX_COMPLETE)
            {
                // Here we receive all data
                // so call usbProtoColHandler();
                EngParms.state = USB_WAIT_FW_RESULT;
                usbProtocolHandler();
                engUsbClearEvent();
            }
            // if ENG_RX_INCOMPLETE, keep the same EngParms.state 		
        }
        // Any thing else is error either hardware 
        // interrupts or USB_WAIT_SETUP data error
        else
        {
            // Error: Reset to waiting for setup
            engInitWaiSetup();
        }
    }
    // Any thing else is error either hardware 
    // interrupts or USB_WAIT_SETUP data error
    else
    {
        // Error: Reset to waiting for setup
        engInitWaiSetup();
    }
}

/*====================================================================
engUsbClearEvent(): this routine is part of the USB state machine.
Output: Update the USB state machine base on the result and state 
and event type of the USB engine.
Note: this routine is part of the USB state machine.
=====================================================================*/
void engUsbClearEvent()
{
    if (EngParms.event == USB_READ_EVENT)
    {
        if (EngParms.state == USB_WAIT_FW_DATA)
        {
            if (UsbRequest.status == EVENT_OK)
            {
                EngParms.state = USB_WAIT_DATA_IN;
                STALLClrInEp0;
                // Here we clear Out EP0 in case the host wants to terminate the
                // IN DATA transaction sooner
                STALLClrOutEp0;
            }
            else
            { 
                // Error: Reset to waiting for setup
                engInitWaiSetup();
            }
        }
        // Any thing else is error
        else
        { 
            // Error: Reset to waiting for setup
            engInitWaiSetup();
        }
    }
    // WRITE EVENT
    else if (EngParms.event == USB_WRITE_EVENT)
    { 			
        if (EngParms.state == USB_WAIT_ADDR_ACK)
        { 
            // prepare zero packet in fifo
            ZEROPACKInEp0;
            STALLOutEp0;
        } 
        else if (EngParms.state == USB_WAIT_FW_RESULT)
        { 		
            // No error
            if (UsbRequest.status == EVENT_OK)
            { 
                EngParms.state = USB_WAIT_IN_ACK;
                
                // prepare zero packet in fifo
                ZEROPACKInEp0;
                STALLOutEp0;
            } 
            else
                // Error: Reset to waiting for setup
                engInitWaiSetup();
        }
        // Any thing else is error
        else
            // Error: Reset to waiting for setup
            engInitWaiSetup();
    }
    // Any thing else is error
    else
        // Error: Reset to waiting for setup
        engInitWaiSetup();
}

/*====================================================================
engLoadTxFifo() : Load TX/IN FIFO for sending data to host
return	ENG_TX_COMPLETE if completed else return ENG_TX_INCOMPLETE
Note: this routine is part of the USB state machine.
=====================================================================*/
void engLoadTxFifo()
{
    // 
    if ((UsbRequest.wLength == 0) && (UsbRequest.xferLength < UsbRequest.ep0MaxPacket))
    {
        EngParms.xferStatus = ENG_TX_COMPLETE;
        return;
    }
    
    EngParms.xferStatus = ENG_TX_INCOMPLETE;
    
    // Copy data to TX FIFO for EP0
    if (UsbRequest.wLength > UsbRequest.ep0MaxPacket)
    {
        memcpy(INPACK,UsbRequest.dataPtr, UsbRequest.ep0MaxPacket);
        UsbRequest.wLength -= UsbRequest.ep0MaxPacket;
        UsbRequest.xferLength = UsbRequest.ep0MaxPacket;
        UsbRequest.dataPtr += UsbRequest.ep0MaxPacket;                            
    }
    else
    {
        memcpy(INPACK,UsbRequest.dataPtr, UsbRequest.wLength);
        UsbRequest.xferLength = UsbRequest.wLength;
        UsbRequest.dataPtr += UsbRequest.wLength;                            
        UsbRequest.wLength = 0;
    }
    EMPTYInEp0;
    IEPDCNTX0 = (byte )UsbRequest.xferLength;
    
    
    return;
}

/*====================================================================
engSaveRxFifo(): Save data following the requests.
return: ENG_RX_COMPLETE or ENG_RX_INCOMPLETE
Note: this routine is part of the USB state machine.
=====================================================================*/
void engSaveRxFifo(void)
{
    // read data receive counter
    UsbRequest.xferLength = OEPDCNTX0 & 0x7F;
    
    // Case host terminate earlier by sending less than max packet
    if ((byte )UsbRequest.xferLength < UsbRequest.ep0MaxPacket)
        UsbRequest.wLength = EngParms.dataCount + UsbRequest.xferLength;
    
    // Copy data from RX FIFO to USB_EP0_XFERDATA
    memcpy(USB_EP0_XFERDATA + EngParms.dataCount, OUTPACK, UsbRequest.xferLength);
    EngParms.dataCount += UsbRequest.xferLength;
    EMPTYOutEp0;
    
    // in case the host want to terminate the transaction sooner
    ZEROPACKInEp0;
    
    // check if READ completed
    if (EngParms.dataCount >= UsbRequest.wLength)
        EngParms.xferStatus = ENG_RX_COMPLETE;
    else
        EngParms.xferStatus = ENG_RX_INCOMPLETE;
    return;
}

/*====================================================================
engInitWaiSetup(): Set Usb Engine for wait setup stage
=====================================================================*/
void engInitWaiSetup(void)	
{
    EngParms.state = USB_WAIT_SETUP;
    EngParms.event = USB_NO_EVENT;
    EMPTYInEp0;
    STALLInEp0;
    STALLOutEp0;
}

/*====================================================================
engUsbInit(): USB initialization - initialize interrupt, enpoint control,
=====================================================================*/
void engUsbInit(void)  
{    
    // configure IN_endpoint_0
    // EP0 Enabled, ISO off, Toggle=0, XBuff only, 
    // Don't STALL IN packet, int enabled 
    IEPCNF0 = 0x84;
    
    // Setup IO buffer address and size for in control endpoint
    IEPBBAX0 = INPACK_OFFSET; 
    IEPBSIZ0 = UsbRequest.ep0MaxPacket >> 3;
    
    // Set NAK bit for IEP, share memory is not clear by reset
    IEPDCNTX0 = 0x80;
    
    // configure OUT_endpoint_0
    // EP0 Enabled, ISO off, Toggle=0, XBuf only, 
    // Don't stall OUT packet, int enabled
    OEPCNF0 = 0x84; 
    
    // Setup IO buffer address and size for out control endpoint
    OEPBBAX0 = OUTPACK_OFFSET;
    OEPBSIZ0 = UsbRequest.ep0MaxPacket >> 3;
    
    // Clear NAK bit, share memory is not clear by reset 
    OEPDCNTX0 = 0x00; 			
           
    // reset state machine
    EngParms.bitFlags = 0;
    engInitWaiSetup();
    
    // Enable Reset, Resume, Suspend, SETUP and STPOW	 
    USBIMSK = 0xE5; 		
    
    // interrupts
    EX0 = 1;					// Enable External 0 interrupt - stc 
    IT0 = 0;					// level activated 
    EA = 1;
    
    USBCTL = 0xC0;		// connect PUR, enable function address, disable FRSTE 
}

