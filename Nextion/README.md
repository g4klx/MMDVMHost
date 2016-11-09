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

# Updating the display

Now comes the easy part. Just execute:

```
$ python nextion.py MMDVM_3.5.tft /dev/ttyUSB0
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

