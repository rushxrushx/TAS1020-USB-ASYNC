;------------------------------------------------------------------------------
;  Copyright 2000, Texas Instruments Inc.
;------------------------------------------------------------------------------
;  PUBLICROM.A51:   This used to provide the application FW with a standard entry
;                   point for accessing ROM functions.
;
;------------------------------------------------------------------------------
;
                EXTRN CODE(_DevFunctionEntryParser)      
                CSEG        AT  0100H      
                LJMP        _DevFunctionEntryParser           ; Jump to the actual code.
                END
