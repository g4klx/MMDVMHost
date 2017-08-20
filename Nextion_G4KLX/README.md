# Update Nextion Displays from the Command Line

This directory contains a simple python script which you can use to update the
Nextion displays. All you need is a compiled .tft file which is written to the
display's flash memory. The precompiled .tft files with the MMDVMHost default
layout are to be found in this directory as well.

To update the Nextion display you just need to know the serial port the display
is connected to. It could be /dev/ttyUSBx for USB<->Serial adapters or
/dev/ttyAMA0 for the UART on the Raspberry Pi for example.

# Prerequisites

You need to have python installed as well as the python-serial package. That can
normally be found in your distro's package manager.

# File Naming Convention

There are compiled .tft files in the repo for basic and enhanced Nextion
displays of sizes 2.4", 2.8", 3.2" and 3.5". Please choose depending on the
model number printed on the back of the display. 

The basic displays are denoted by a "T" as 7th character in the filename and
model number whereas enhanced displays have a "K" in that position. The actual
display size can be derived from the last two digits. The four digits in between
the characters refert to the diplay's resolution.

For example: A model number NX4832T035 represents a display with:

 - a resolution of 320x480 pixels
 - basic command set
 - a screen size of 3.5"

# Updating the Display

Now comes the easy part. Just execute:

```
$ python nextion.py NX4832T035.tft /dev/ttyUSB0
```

And the output should be as follows:

```
Trying with baudrate: 2400...
Trying with baudrate: 4800...
Trying with baudrate: 9600...
Connected with baudrate: 9600...
Status: comok
Touchscreen: yes
Model: NX4832T035_011R
Firmware version: 68
MCU code: 61488
Serial: D2658880C35D2124
Flash size: 16777216
Downloading, 100%...
File transferred successfully
```

# Known errors

Especially when using USB<->Serial adapters there are cases in which the scripts
stops at different times. This is known and due to the very simple update
protocol. In this case you have to fix the display by using a SD-Card to update
the display. The /dev/ttyAMAx ports do not seems to suffer from this issue.

