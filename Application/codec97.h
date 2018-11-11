//==================================================
// Texas Instruments Strictly Private
// Copyright 2000, Texas Instruments Inc.
//==================================================
#ifndef	_CODEC97_H_
#define _CODEC97_H_

// Mixer registers
#define CO97_REG_RESET              0x00
#define CO97_REG_MASTER_VOL         0x02
#define CO97_REG_HEADTONE_VOL       0x04
#define CO97_REG_MONO_MASTER_VOL    0x06
#define CO97_REG_MASTER_TONE        0x08
#define CO97_REG_PC_BEEP_VOL        0x0A
#define CO97_REG_PHONE_VOL          0x0C

#define CO97_REG_MIC_VOL            0x0E
#define CO97_MIC_VOL_20DB_BOOST     0x40

#define CO97_REG_LINEIN_VOL         0x10
#define CO97_REG_CD_VOL             0x12
#define CO97_REG_VIDEO_VOL          0x14
#define CO97_REG_AUX_VOL            0x16
#define CO97_REG_PCM_OUT_VOL        0x18

#define CO97_REG_RECORD_SELECT          0x1A
#define CO97_RECORD_SELECT_MIC          0x0000
#define CO97_RECORD_SELECT_CDIN         0x0202
#define CO97_RECORD_SELECT_VIDEOIN      0x0404
#define CO97_RECORD_SELECT_AUXIN        0x0808
#define CO97_RECORD_SELECT_LINEIN       0x1010
#define CO97_RECORD_SELECT_STEREOMIX    0x2020
#define CO97_RECORD_SELECT_MONOMIX      0x4040
#define CO97_RECORD_SELECT_PHONE        0x8080

#define CO97_REG_RECORD_GAIN        0x1C
#define CO97_REG_MIC_RECORD_GAIN    0x1E
#define CO97_REG_GENERAL_PURPOSE    0x20
#define CO97_REG_3D_CONTROL         0x22
#define CO97_REG_RESERVED           0x24

#define CO97_REG_POWERDOWN_CTLRSTAT         0x26 
#define CO97_REG_POWERDOWN_STAT_READY_ADC   0x0001
#define CO97_REG_POWERDOWN_STAT_READY_DAC   0x0002
#define CO97_REG_POWERDOWN_STAT_READY_ANL   0x0004
#define CO97_REG_POWERDOWN_STAT_READY_REF   0x0008
#define CO97_REG_POWERDOWN_CTLR_PR0         0x0100
#define CO97_REG_POWERDOWN_CTLR_PR1         0x0200
#define CO97_REG_POWERDOWN_CTLR_PR2         0x0400
#define CO97_REG_POWERDOWN_CTLR_PR3         0x0800
#define CO97_REG_POWERDOWN_CTLR_PR4         0x1000
#define CO97_REG_POWERDOWN_CTLR_PR5         0x2000
#define CO97_REG_POWERDOWN_CTLR_PR6         0x4000
#define CO97_REG_POWERDOWN_CTLR_EAPD        0x8000

#define CO97_REG_VENDOR_ID1         0x7C
#define CO97_REG_VENDOR_ID2         0x7E

// Codec 97 2.0 Audio, extended audio registers
#define CO97_XAUDREG_ID                 0x28
#define CO97_XAUDREG_ID_VRA             0x0001
#define CO97_XAUDREG_ID_DRA             0x0002
#define CO97_XAUDREG_ID_VRM             0x0008
#define CO97_XAUDREG_ID_CDAC            0x0040
#define CO97_XAUDREG_ID_SDAC            0x0080
#define CO97_XAUDREG_ID_LDAC            0x0100
#define CO97_XAUDREG_ID_AMAP            0x0200
#define CO97_XAUDREG_ID_PRIMARY         0x0000
#define CO97_XAUDREG_ID_SECONDARY       0x4000
#define CO97_XAUDREG_ID_THIRDARY        0x8000
#define CO97_XAUDREG_ID_FOURTHARY       0xC000

#define CO97_XAUDREG_CTRLSTATUS         0x2A
#define CO97_XAUDREG_CTRL_ENA_VRA       0x0001
#define CO97_XAUDREG_CTRL_ENA_DRA       0x0002
#define CO97_XAUDREG_CTRL_ENA_VRM       0x0008
#define CO97_XAUDREG_STATUS_READY_CDAC  0x0040
#define CO97_XAUDREG_STATUS_READY_SDAC  0x0080
#define CO97_XAUDREG_STATUS_READY_LDAC  0x0100
#define CO97_XAUDREG_STATUS_READY_MADC  0x0200
#define CO97_XAUDREG_CTRL_OFF_PRI       0x0800
#define CO97_XAUDREG_CTRL_OFF_PRJ       0x1000
#define CO97_XAUDREG_CTRL_OFF_PRK       0x2000
#define CO97_XAUDREG_CTRL_OFF_PRL       0x4000
 
#define CO97_XAUDREG_PCMFRONTDAC_RATE   0x2C
#define CO97_XAUDREG_PCMSURRDAC_RATE    0x2E
#define CO97_XAUDREG_PCMLFEDAC_RATE     0x30
#define CO97_XAUDREG_PCMLRADC_RATE      0x32
#define CO97_XAUDREG_MICADC_RATE        0x34
#define CO97_XAUDREG_6CH_LFE_VOL        0x36
#define CO97_XAUDREG_6CH_LRSURR_VOL     0x38
#define CO97_XAUDREG_LINE1_ADCDAC_RATE  0x40
#define CO97_XAUDREG_LINE2_ADCDAC_RATE  0x42

// Wilson AC Codec vendor
#define CO97_WILSON_FRONT_MIXER_VOL     0x072

// Common Codec 97 bit flags if applied
#define CO97_MUTE_ON                    0x80


// jsmxxx - start
#define CO97_PRIMARY_CODEC_READY     0x40
#define CO97_SECONDARY_CODEC_READY   0x20            

#define CO97_VREF_READY         0x08            
#define CO97_ANL_MIXER_READY    0x04           
#define CO97_DAC_READY          0x02           
#define CO97_ADC_READY          0x01 
#define CO97_READY_MASK         0x0F          

#define CO97_CODEC_READY        (CO97_VREF_READY | \
                                 CO97_ANL_MIXER_READY | \
                                 CO97_DAC_READY | \
                                 CO97_ADC_READY)

#define CO97_REG_READ_ADDR(s)   (s | 0x80) 

 
// Codec 97 2.0 Modem


#endif