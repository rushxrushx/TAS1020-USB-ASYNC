Devices are generally configured as a stereo channel speaker, microphone, 
and line in based on AC97 Codec. Differences are described below:
app016.XX: 16 bit resolution using accessing parameters indirectly through the 
shared external buffer.
app024.XX: 24 bit resolution using accessing parameters indirectly through the 
shared external buffer.
app116.XX: 16 bit resolution using accessing parameters through USB request data
in DATA area directly.
app124.XX: 24 bit resolution using accessing parameters through USB request data
in DATA area directly.
The binary images are built using the header.txt file and saved in the bin 
directory.

v1.6 07/27/01
1) Added code so that XINT interrupt is only handled when in suspend.
2) Added approx. 5 ms delay when exiting suspend to debounce buttons if used to do
   remote wake up.
3) Renamed suspend flag, "AppSuspense", to "AppSuspend".

v1.7 07/30/01
1) Removed code that was turning SOF off if speaker or mic interface was off.

v1.8 12/04/01
1) Moved USB out input terminal descriptor directly after the class specific 
audio control interface descriptor. This was done to work around what appears 
to be a problem with Apple OS X
