The mods are made with a personal HotSpot in mind.
DVMega, MMDVM_HS, low cost or ZumSpot are best candidates.

This code works with DMR SlotTime 2 only, all Slot 1 info are pulled out.

The Talker Alias, IP number and Temperature for Raspberry Pi are added.
In Orange Pi the code for temperature works with small changes.

Are added some logos for differents networks in 128x26 & 128 x 32 formats for the Idle Screen and small logos(64x16) for the TX/RX screens, also a logos for Parrot and disconnect codes (9990 and 4000)

*** New entry into MMDVVM.ini ***


Choose your loyout with:
# 0 number Layout G4KLX / 2 number Layout EA5SW
OledLayout = 0


All TG codes are easy to change into OLED.cpp

Example TG codes:

4000 display a broken chains logo
9990 display a parrot logo
9 or 8 displays a DmrPlus logo into tx/rx screen
6 displays a XLX logo
any other TG display a BrandMeister Logo.

Code also have a small remote command to shutdown,reboot and change modes of operation, simply when TX into a determined TG number:

9999 Reboot Raspberry
9998 Shutdown Raspberry
9997 Send mm_plus command to start a new MMDDVMHost in dmrplus mode
9996 Send mm_gate command to start a new MMDDVMHost in DMRGateway mode
9995 Send mm_BM command to start a new MMDDVMHost in BrandMeister mode




73 & DX
EA5SW
Jose
