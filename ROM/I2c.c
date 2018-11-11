//=============================================================================
// i2c.c
//
// This module contains I2C read/write routines.
//
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//============================================================================= 

#include <reg52.h>
#include "reg_stc1.h"
#include "delay.h"
#include "xdata.h"
#include "i2c.h"



//
// Function: Waits for an I2C operation to complete within some specified
//					 number of microseconds.
//
// Input: 	I2CStatusWaitValue - The I2C status register value which will
//							indicate that the I2C operation is completed.
//					nTimeoutMicroSec - number of microseconds to wait.
//
// Returns	1 is operation is completed successfully within the timeout
//					period. 0 o.w.
//
bit WaitOnI2C(byte StatusWaitValue)
{
    word timeout = I2C_DELAY;

    while (!(I2CSTA & (StatusWaitValue | ERROR)) && timeout)
    {
        timeout--;
    }
    
    return ((timeout != 0) && (!(I2C_ERROR))) ;
}



bit I2CAccess()
{ 			
    if (I2CParams.Flags & I2C_400_KHZ)
        I2CSTA |= FREQ_400KHZ;
    else
        I2CSTA &= ~FREQ_400KHZ;
    
    if (I2CParams.Flags & I2C_START)
    {
        //
        // Write phase; write out slave address and subaddress
        //
        
        I2CSTA	&= CLEAR_ALL;
        I2CADR	= I2C_WRITE_ADDR(I2CParams.SlaveAddr);
        
        // Check if word address access
        if (I2CParams.Flags & I2C_WORD_ADDR_TYPE)
        {
            // Write MSB address first
            I2CDATO = (BYTE )(I2CParams.SubAddr >> 8);
            
            
            //
            // Wait a certain amount for transmission to complete. 
            //
            if (!WaitOnI2C(XMIT_DATA_EMPTY))
                return 0;
        }
        
        I2CDATO = (BYTE )I2CParams.SubAddr;
        
        //
        // Wait a certain amount for transmission to complete. 
        //
        
        if (!WaitOnI2C(XMIT_DATA_EMPTY))
            return 0;
        
        
        I2CSTA &= CLEAR_ALL;
        
        if (I2CParams.Flags & I2C_READ)
        {
            // 
            // Read phase; read in data.
            //
            // Need to just put some value into I2CDAT0 address to fire
            // off the write. The hardware, based on the read bit of the
            // address, should know to send out only the slave address and
            // not the dummy data byte.
            //
            
            I2CADR	= I2C_READ_ADDR(I2CParams.SlaveAddr);
            
            // case read one byte only
            if (I2CParams.nLen == 1)
            {
                I2CSTA |= STOP_READ;
                I2CDATO = 0xFF;
                if (!WaitOnI2C(RCV_DATA_FULL))
                    return 0;
                
                *I2CParams.pBuffer = I2CDATI;
                I2CSTA &= CLEAR_ALL;
                return 1;
            }
            else
                I2CDATO = 0xFF;
        }
    }
    
    
    for (; I2CParams.nLen; I2CParams.nLen--)
    {
        //
        // Setup timer to track timeout time.
        //
        
        if (I2CParams.Flags & I2C_READ)
        {
            //
            // Setup timer to track timeout time.
            //
            
            if (!WaitOnI2C(RCV_DATA_FULL))
                return 0;
            //
            // If this is the second last byte to be read for this read command,
            // set bit indicating that a nak/stop condition should be sent.so when
            // we read the second last byte, the STOP will take effect for the last
            // byte!!!
            //
            if (I2CParams.nLen == 2 && (I2CParams.Flags & I2C_STOP))
                I2CSTA |= STOP_READ;
            
            // Read byte
            *I2CParams.pBuffer++ = I2CDATI;
        }
        else
        {
            //
            // If this is the last byte to be write for this write command,
            // set bit indicating that a nak/stop condition should be sent.
            //
            if (I2CParams.nLen == 1 && (I2CParams.Flags & I2C_STOP))
                I2CSTA |= STOP_WRITE;
            
            // Write data
            I2CDATO = *I2CParams.pBuffer++;
            
            //
            // Setup timer to track timeout time.
            //
            if (!WaitOnI2C(XMIT_DATA_EMPTY))
                return 0;
            
        }
        
    }
    
    I2CSTA &= CLEAR_ALL;
    return(1);							// return data
    
}
