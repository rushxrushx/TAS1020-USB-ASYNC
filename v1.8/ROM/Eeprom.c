//==================================================================== 
// Texas Instruments Strictly Private 
// Copyright 2000, Texas Instruments Inc. 
//==================================================================== 
/*====================================================================  
Eeprom.c
Handle eeprom related functions
====================================================================*/
#include "types.h"
#include <Reg52.h>
#include "reg_stc1.h"
#include "Mmap.h"
#include "eeprom.h"
#include "Utils.h"
#include "romboot.h"
#include "usbEng.h"
#include "usbdfu.h"
#include "xdata.h"
#include "DfuMmap.h"
#include "I2c.h"

bit eepromReadEeeprom(word SubAddr, byte xdata *pBuffer, word Len, byte Flags) small;

/*====================================================================
eepromBoot() : Copy Eeprom application data to program RAM
Input:  none
Output: none
Return: 
=====================================================================*/
bit eepromBoot()
{
    word temp;
    word size;
    byte i2cFlag;
    
    
    // in case of bus power device which requires time limit connection
    // Setup the usb engine which will only handle the SET ADDR as the 
    // RomRecord state is ROM_BOOT_START
    if (DfuData.eepromHeaderData.usbAttribute & EEPROM_BUS_POWER)
    {
        UsbRequest.ep0MaxPacket = 8;
        engUsbInit();
        // Setup control endpoint 0, connect to bus, and mask off SETUP ISR
        // This keep NAK for USB IN/OUT transaction
    }
    
    
    // if eeprom is app type, return error
    size = DfuData.eepromHeaderData.payloadSize;
    temp = DfuData.eepromHeaderData.rPageSize;
    if (temp == EEPROM_RPAGESIZE_ALL)
        temp = size;
    
    i2cFlag = I2C_READ | I2C_START | I2C_STOP;
    
    if (DfuData.eepromHeaderData.attribute & EEPROM_SUPPORT_400_KHZ)
    {
        I2CSTA |= FREQ_400KHZ;
        i2cFlag |= I2C_400_KHZ;
    }
    
    if (RomRecord.attribute & ROM_EEPROM_WORD_ACCESS_MODE)
        i2cFlag |= I2C_WORD_ADDR_TYPE;
    
    while(size)
    {
        if (temp > size)
            temp = size;
        
        if (!eepromReadEeeprom(DfuData.eepromHeaderData.headerSize + DfuData.eepromHeaderData.payloadSize - size,
            (unsigned char xdata *)PROG_RAM_ADDR_START + DfuData.eepromHeaderData.payloadSize - size,
            temp,
            i2cFlag))
            return FALSE;
        
        size -= temp;
    }
    
    return TRUE;
}

/*====================================================================
eepromExist() : Read eeprom header and save
Input:  none
Output: none
Return: TRUE if eeprom exist, else FALSE
Note: Checksum is computed by adding bytes
=====================================================================*/
void eepromExist()
{
    byte flags;
    
    // clear eeprom header struct data first
    dfuWritePattern((byte xdata *)&DfuData.eepromHeaderData.headerChksum, sizeof(DfuData.eepromHeaderData), 0);
    
    // Check which address mode the eeprom supports by trying to read the
    // signatures in byte access mode and in word access mode
    flags = I2C_READ | I2C_START | I2C_STOP; 
    SET_I2C (EEPROM_I2C_ADDR, EEPROM_SIGNATURES_OFFSET, 
        (unsigned char xdata *)&DfuData.eepromHeaderData.signatures[0], 
        1, flags);
    
    if (I2CAccess())
    { 
        RomRecord.attribute &= ~ROM_EEPROM_WORD_ACCESS_MODE;
        if (DfuData.eepromHeaderData.signatures[0] != 0x12)
        {
            RomRecord.attribute |= ROM_EEPROM_WORD_ACCESS_MODE;
            flags |= I2C_WORD_ADDR_TYPE;
        }
    }
    else
    {
        DfuData.eepromHeaderData.dataType = EEPROM_UNEXIST;
        return;
    }

    // read header size
    SET_I2C (EEPROM_I2C_ADDR, EEPROM_HEADERSIZE_OFFSET, 
        (unsigned char xdata *)&DfuData.eepromHeaderData.headerSize, 
        1, flags);
    if (!I2CAccess())
        DfuData.eepromHeaderData.dataType = EEPROM_INVALID;    
    else
    {
        // Now readind the header
        // read header
        SET_I2C (EEPROM_I2C_ADDR, EEPROM_START_OFFSET, 
            (unsigned char xdata *)&DfuData.eepromHeaderData.headerChksum, 
            DfuData.eepromHeaderData.headerSize, flags);
        
        if (I2CAccess())
        {
            // Check checksum
            if (eepromCheckFirmware((byte xdata *)&DfuData.eepromHeaderData.headerChksum) == FALSE)
                if (DfuData.eepromHeaderData.dataType != EEPROM_APPCODE_UPDATING)
                    DfuData.eepromHeaderData.dataType = EEPROM_INVALID;
        }
        else
            DfuData.eepromHeaderData.dataType = EEPROM_INVALID;
    }
    
    return;
}

/*====================================================================
eepromCheckFirmware(): Check to see if header checksum is OK
Input:  none
Output: none
Globals used:
Return: FALSE or TRUE
====================================================================*/
bit eepromCheckFirmware(byte *EHeader)
{
    byte i;
    byte chksum;
    
    for (i = 1, chksum = 0; i < EHeader[1]; i++)
        chksum += EHeader[i];
    if (chksum == EHeader[0])
        return TRUE;
    
    return FALSE;
}


bit WaitI2cRcvDataFull() small
{
    word wTimeout;
    
    for (wTimeout = I2C_DELAY; 
    !(I2CSTA & (RCV_DATA_FULL | ERROR)) && wTimeout--;);
    
    if (wTimeout == 0 || I2C_ERROR)
        return 0;
    
    return (wTimeout != 0) && !I2C_ERROR ;
}

//
// eepromReadEeprom - Reads EEPROM into xdata memory
//
// Input:
//		SubAddr - Eeprom address which to read.
//		pBuffer - pointer to xdata buffer.
//		Len - Number of bytes to read.
//		Flags - I2C flags.
//
// Output: 1 if successful, 0 o.w.
//
//
bit eepromReadEeeprom(word SubAddr, byte xdata *pBuffer, word Len, byte Flags) small
{
    
    //
    // Write phase; write out slave address and subaddress
    //
    
    I2CSTA	&= CLEAR_ALL;
    I2CADR	= I2C_WRITE_ADDR(EEPROM_I2C_ADDR);
    
    // Check if word address access
    if (Flags & I2C_WORD_ADDR_TYPE)
    {
        // Write MSB address first
        I2CDATO = (BYTE )(SubAddr >> 8);
        
        
        //
        // Wait a certain amount for transmission to complete. 
        //
        if (!WaitOnI2C(XMIT_DATA_EMPTY))
            return 0;
    }
    
    I2CDATO = (BYTE )SubAddr;
    
    //
    // Wait a certain amount for transmission to complete. 
    //
    
    if (!WaitOnI2C(XMIT_DATA_EMPTY))
        return 0;
    
    
    I2CSTA &= CLEAR_ALL;
    
    // 
    // Read phase; read in data.
    //
    // Need to just put some value into I2CDAT0 address to fire
    // off the write. The hardware, based on the read bit of the
    // address, should know to send out only the slave address and
    // not the dummy data byte.
    //
    
    I2CADR	= I2C_READ_ADDR(EEPROM_I2C_ADDR);
    
    I2CDATO = 0xFF;
    
    
    for (Len -= 2; Len; Len--)
    {
        
        
        if (!WaitI2cRcvDataFull())
            return 0;
        
        
        // Read byte
        *pBuffer++ = I2CDATI;
        
    }
    
    
    if (!WaitI2cRcvDataFull())
        return 0;
    
    I2CSTA |= STOP_READ;
    
    *pBuffer++ = I2CDATI;
    
    if (!WaitI2cRcvDataFull())
        return 0;
    
    // Read byte
    *pBuffer++ = I2CDATI;
    
    I2CSTA &= CLEAR_ALL;
    return 1;		
    
    
}

