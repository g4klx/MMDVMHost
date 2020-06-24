# Prerequisite 

Enable I2C and SPI modules directly with raspi-config tool, issue a
```
sudo raspi-config
```

Then go to menu Advanced Option, select SPI and under question ” Would you like the SPI kernel module to be loaded by default ?”, select Yes, Do the same thing for I2C Advanced Option.

As I don’t use monitor or TV connected to Pi, I decreased dedicated video memory, always in raspi-config, go to Advanced Options then Memory Split, then type 16 Mo (the minimal) used for GPU, then select Finish, and select Yes when asked to reboot.

To be able to compile you will need the compiler and some others tools, issue a :

```
sudo apt-get install build-essential git-core libi2c-dev i2c-tools lm-sensors wiringpi
```
*italic* Sometimes I2C and SPI modules are not started and thus he cannot start the sample code. The solution to start the modules at startup by adding the two following lines into the file /etc/modules

```
i2c-dev
spidev
```
Reboot, then you MUST see SPI and I2C devices with the following command

```
root@raspberrypi:~# ls /dev/i2c*
/dev/i2c-0
root@raspberrypi:~# ls /dev/spi*
/dev/spidev0.0  /dev/spidev0.1
```
# Installation of the generic Driver

The Driver is based on Adafruit Arduino library, I ported the code to be able to compile and run on Raspberry Pi but added also some features.

Get all the file from github dedicated repo :
```
git clone https://github.com/hallard/ArduiPi_OLED
cd ArduiPi_OLED
sudo make
```

# Building MMDVMHost
```
make -f Makefile.Pi.OLED
```

The initial guide is written by [Charles](http://hallard.me/adafruit-oled-display-driver-for-pi/)
