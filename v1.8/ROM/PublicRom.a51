;------------------------------------------------------------------------------
;  Copyright 2000, Texas Instruments Inc.
;------------------------------------------------------------------------------
;  PUBLICROM.A51:   This used to provide the application FW with a standard entry
;                   point for accessing ROM functions.
;
;------------------------------------------------------------------------------
;
                EXTRN CODE(_RomFunction)

                CSEG        AT  9FFDH      
                LJMP        _RomFunction            ; Jump to the actual code.
                END
