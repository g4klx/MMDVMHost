Nextion Display Layouts by ON7LDS (for MMDVMHost)
=================================================

The screenlayout has to be selected with the parameter **ScreenLayout** in the
MMDVM.ini file under the Nextion section. This way, the extra functions
are activated.  

  0 = auto (future use, for now it's G4KLX layout)  
  1 = G4KLX layout  
  2 = ON7LDS layout (see README-L2)  
  3 = ON7LDS DIY layout  
  4 = ON7LDS DIY layout High Speed  
  
Layout 2 is a no-nonsense layout. It is the original (G4KLX) layout with the Talker Alias added. TA color and fonts size can not be changed. At least not easily.

Layout 3 (as is 4) is a layout without any predefined layout options (color, fonts). It sends the fields **and** information about what was sent to the display, so all layout processing can and should be done in the display itself. 

More information about the layouts can be found in  
  * README-L2 for the screenLayout 2 setting
  * README-L2 for the screenLayout 3 and 4 settings

  
When you want extra control over what has to be sent to the Nextion display, you could consider the program 'NextionDriver' at https://github.com/on7lds/NextionDriver as a companion to MMDVMHost.
This program sends extra information about the host to the display, can do callsign lookup with extended information (name, city, country) and can do more processing which would not be the task of MMDVMHost (for example: with the help of this program, it is possible to use buttons on the display to do actions on the host itself).  
In verbose mode, this program shows you all communication between MMDVMHost and the display.
