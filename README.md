These are the source files for building the MMDVMHost, the program that interfaces to the MMDVM or DVMega on the one side, and a suitable network on the other. It supports D-Star, DMR, P25 Phase 1, NXDN, System Fusion, and POCSAG paging on the MMDVM, and D-Star, DMR, and System Fusion on the DVMega.

On the D-Star side the MMDVMHost interfaces with the ircDDB Gateway, on DMR it can connect to BrandMeister, DMR+, HB Link, XLX or [DMRGateway](https://github.com/g4klx/DMRGateway) (to connect to multiple DMR networks at once) on System Fusion it connects to the YSF Gateway to allow access to the FCS and YSF networks. On P25 it connects to the P25 Gateway. On NXDN it connects to the NXDN Gateway which provides access to the NXDN and NXCore talk groups. Finally it uses the DAPNET Gateway to access DAPNET to receive paging messages.

It builds on 32-bit and 64-bit Linux as well as on Windows using Visual Studio 2017 on x86 and x64. It can optionally control various Displays. Currently these are:

- HD44780 (sizes 2x16, 2x40, 4x16, 4x20)
	- Support for HD44780 via 4 bit GPIO connection (user selectable pins)
	- Adafruit 16x2 LCD+Keypad Kits (I2C)
	- Connection via PCF8574 GPIO Extender (I2C)
- Nextion TFTs (sizes 2.4", 2.8", 3.2" and 3.5")
- TFT display sold by Hobbytronics in UK
- OLED 128x64 (SSD1306)
- LCDproc

The Nextion displays can connect to the UART on the Raspberry Pi, or via a USB to TTL serial converter like the FT-232RL. It may also be connected to the UART output of the MMDVM modem (Arduino Due, STM32, Teensy), or to the UART output on the UMP.

The HD44780 displays are integrated with wiringPi for Raspberry Pi based platforms.

The Hobbytronics TFT Display, which is a Pi-Hat, connects to the UART on the Raspbery Pi.

The OLED display needs a extra library see OLED.md

The LCDproc support enables the use of a multitude of other LCD screens. See the [supported devices](http://lcdproc.omnipotent.net/hardware.php3) page on the LCDproc website for more info.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.
