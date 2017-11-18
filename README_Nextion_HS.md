
Now Nextion Screen is more complex, all development are made and tested into a Raspberry Pi  and use some routines of this hardware.
It's possible that only work in Rpi and not in Orange Pi or similar

Are made with DMR only in mind for Hotspots, so only use Timeslot2 and not tested in mixed mode (P25,Dstar,etc)
The code made a new screens for  Nextion screens.

At this time displays:

Some extra information::
Locator, City and Frequency.

Real IP number at startup od Pi, etho or wlan detects.
Temperature of Raspberry Pi.
Real time Smeter for MMDVM Modems that outs RSSI information (MMDVM modes with radios as Motorola,for example)
Fake Smeters for the rests of types (DVMEGA,HS_HAT,etc)
Change of colors in sequence Idle/RX/TX of Hotspot.
Talker Alias display of information send by master.
Decode of TG number and display of picture associated to this TG. Now TG's are the EA most used.
Decode of Callsign Prefix and display of picture associated to this Prefix.Now are the EA,EB & EC prefix
Decode of 9990 and 4000 commands also displaying pictures
Decode of own callsign and display of own picture.
Remote Reboot and Shutdown of Pi eligible by sysop via DMR

Install & Run:

New entry in MMDVM.ini for path to config files in Nextion section:
FilesConfig=/home/pi/MMDVMHost/etc/

You have now 6 config files (work in progress to put in only one), don't comment or delete any line, the code are simple and need to end with a C/R line to EOF



info.ini  contains information for MMDVM home screen:

Callsign
Location
Frequency
DMR Network Name

ctrl.ini contains:

callsign of owner
remote commands on/off
TG number to shutdown Pi
TG number to reboot Pi
TG number to change mode to dmrplus network
TG number to change mode to DmrGateway mode
TG number to change mode to BrandMeister network

prefixA.ini, prefixB.ini and prefixC.ini contains a list of prefix to combine for displkay a picture map into a Nextion.
example:
EA-EA9, EB0-EB9,EC0-EC9, are the official prefix for Spain to hams (don't include a special stations)

tg.ini contains the number of the TG's (BrandMeister Network) to display a picture logo into a Nextion.
The first TG number equals to picture 30 into the Nextion tft files.


Nextion tft (compiled) and hmi (source) files contain the pictures to work.


The editable pics are 200x100 and 100x50 pixels

Open the Nextion HMI in Editor and observe the picture list 
Also Open the Nextion.ini file in a editor to see references

0-19 are semi-fixed pictures, change the 8 & 9 with the replace opcion for your own callsign picture

20-29 are the prefixes pictures 3 diferents  prefixes are allocated for every pic, in Nextion.ini section [WPX] you see PXA0,PXB0 and PXC0 and the  prefixes EA0 EB0 and EC0 When you transmit or receive a transmission from these prefixes are displayed the pic 20
If the transmission comes from EA2,EB2 or EC2 prefixes, display picture 21 and all  others.

The TG list are from picture 30 to picture 137, and works same that prefix, the list are in the tg.ini file.

example:
the TG localed in first position are 214, when receive a transmission of this TG displays the picture 30 in the Nextion.
214 (EA national Talkgroup) have the picture 30
example of change: If yoy want receive the 3100 USA Talkgroup  replace the image in the Nextion display (picture 30) for a USA Flag, change the entry 214 for 3100 in tg.ini


Remember that any change in tg.ini are needed to change in the Nextion Screen and upload again to the display.

You control if the remote is active when 1 or deactivate when 0 into ctrl.ini

99998 are Shutdown TG to transmit to shutdown the Pi
99999 are Reboot TG to transmit to reboot the Pi
TG number to change mode to dmrplus network
TG number to change mode to DmrGateway mode
TG number to change mode to BrandMeister network

Only the Owner  callsign are allowed to TX into those TG to activate functions.

Thanks to the effort of:
G4KLX, ON7LDS,G0WFV, VK6LS & more for code
EA4ATS for all graphics, a very BIG JOB !!
EA5DHO for test,test,test,test and TEST and ideas!!

73 & DX from EA5SW
