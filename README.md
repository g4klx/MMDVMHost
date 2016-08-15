These are the source files for building the MMDVMHost, the program that interfaces to the MMDVM or DVMega on the one side, and a suitable network on the other. On the D-Star side the MMDVMHost interfaces with the ircDDB Gateway, on DMR it connects to Brand Meister, DMR+, and HB Link, on System Fusion it connects to the YSF Gateway..

It supports D-Star, DMR, and System Fusion.

It builds on 32-bit and 64-bit Linux as well as on Windows using VS2015 on x86 and x64. It can optionally control various Displays. Currently these are:

- HD44780 (sizes 2x16, 2x40, 4x16, 4x20)
	- Support for HD44780 via 4 bit GPIO connection (user selectable pins)
	- Adafruit 16x2 LCD+Keypad Kits (I2C)
	- Connection via PCF8574 GPIO Extender (I2C)
- Nextion TFTs (sizes 2.4", 3.2" and 3.5")
- TFT displays sold by Hobbytronics in UK
- OLED 128x64 (SSD1306)

The HD44780 displays are integrated with wiringPi for Raspberry Pi based platforms. The other displays can be directly connected to the UART on Raspberry Pis or with FT-232RL modules to any USB port.

The OLED display needs a extra lib see OLED.md

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.
