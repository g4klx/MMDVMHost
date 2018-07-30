/*
 *   Copyright (C) 2018 by Shawn Chain BG5HHP
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "DisplayFactory.h"
#include "SerialController.h"
#include "ModemSerialPort.h"
#include "TFTSerial.h"
#include "LCDproc.h"
#include "Nextion.h"
#include "NullDisplay.h"

#if defined(HD44780)
#include "HD44780.h"
#endif

#if defined(OLED)
#include "OLED.h"
#endif

#include "Log.h"

#include <string>


CDisplay* CDisplayFactory::createDisplay(const CConf &conf, CUMP *ump, CModem *modem){
    CDisplay *display = NULL;

    std::string type   = conf.getDisplay();
	unsigned int dmrid = conf.getDMRId();

	LogInfo("Display Parameters");
	LogInfo("    Type: %s", type.c_str());

	if (type == "TFT Serial") {
		std::string port        = conf.getTFTSerialPort();
		unsigned int brightness = conf.getTFTSerialBrightness();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);

		ISerialPort* serial = NULL;
		if (port == "modem")
			serial = new CModemSerialPort(modem);
		else
			serial = new CSerialController(port, SERIAL_9600);

		display = new CTFTSerial(conf.getCallsign(), dmrid, serial, brightness);
	} else if (type == "Nextion") {
		std::string port            = conf.getNextionPort();
		unsigned int brightness     = conf.getNextionBrightness();
		bool displayClock           = conf.getNextionDisplayClock();
		bool utc                    = conf.getNextionUTC();
		unsigned int idleBrightness = conf.getNextionIdleBrightness();
		unsigned int screenLayout   = conf.getNextionScreenLayout();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);
		LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");
		if (displayClock)
			LogInfo("    Display UTC: %s", utc ? "yes" : "no");
		LogInfo("    Idle Brightness: %u", idleBrightness);

		switch (screenLayout) {
		case 0U:
			LogInfo("    Screen Layout: G4KLX (Default)");
			break;
		case 2U:
			LogInfo("    Screen Layout: ON7LDS");
			break;
		case 3U:
			LogInfo("    Screen Layout: DIY by ON7LDS");
			break;
		case 4U:
			LogInfo("    Screen Layout: DIY by ON7LDS (High speed)");
			break;
		default:
			LogInfo("    Screen Layout: %u (Unknown)", screenLayout);
			break;
		}

		if (port == "modem") {
			ISerialPort* serial = new CModemSerialPort(modem);
			display = new CNextion(conf.getCallsign(), dmrid, serial, brightness, displayClock, utc, idleBrightness, screenLayout);
		} else if (port == "ump") {
			if (ump != NULL)
				display = new CNextion(conf.getCallsign(), dmrid, ump, brightness, displayClock, utc, idleBrightness, screenLayout);
            else
                display = new CNullDisplay;
		} else {
			SERIAL_SPEED baudrate = SERIAL_9600;
			if (screenLayout==4U)
				baudrate = SERIAL_115200;
			ISerialPort* serial = new CSerialController(port, baudrate);
			display = new CNextion(conf.getCallsign(), dmrid, serial, brightness, displayClock, utc, idleBrightness, screenLayout);
		}
	} else if (type == "LCDproc") {
		std::string address       = conf.getLCDprocAddress();
		unsigned int port         = conf.getLCDprocPort();
		unsigned int localPort    = conf.getLCDprocLocalPort();
		bool displayClock         = conf.getLCDprocDisplayClock();
		bool utc                  = conf.getLCDprocUTC();
		bool dimOnIdle            = conf.getLCDprocDimOnIdle();

		LogInfo("    Address: %s", address.c_str());
		LogInfo("    Port: %u", port);

		if (localPort == 0 )
			LogInfo("    Local Port: random");
		else
			LogInfo("    Local Port: %u", localPort);

		LogInfo("    Dim Display on Idle: %s", dimOnIdle ? "yes" : "no");
		LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");

		if (displayClock)
			LogInfo("    Display UTC: %s", utc ? "yes" : "no");

		display = new CLCDproc(address.c_str(), port, localPort, conf.getCallsign(), dmrid, displayClock, utc, conf.getDuplex(), dimOnIdle);
#if defined(HD44780)
	} else if (type == "HD44780") {
		unsigned int rows              = conf.getHD44780Rows();
		unsigned int columns           = conf.getHD44780Columns();
		std::vector<unsigned int> pins = conf.getHD44780Pins();
		unsigned int i2cAddress        = conf.getHD44780i2cAddress();
		bool pwm                       = conf.getHD44780PWM();
		unsigned int pwmPin            = conf.getHD44780PWMPin();
		unsigned int pwmBright         = conf.getHD44780PWMBright();
		unsigned int pwmDim            = conf.getHD44780PWMDim();
		bool displayClock              = conf.getHD44780DisplayClock();
		bool utc                       = conf.getHD44780UTC();

		if (pins.size() == 6U) {
			LogInfo("    Rows: %u", rows);
			LogInfo("    Columns: %u", columns);

#if defined(ADAFRUIT_DISPLAY) || defined(PCF8574_DISPLAY)
			LogInfo("    Device Address: %#x", i2cAddress);
#else
			LogInfo("    Pins: %u,%u,%u,%u,%u,%u", pins.at(0U), pins.at(1U), pins.at(2U), pins.at(3U), pins.at(4U), pins.at(5U));
#endif

			LogInfo("    PWM Backlight: %s", pwm ? "yes" : "no");
			if (pwm) {
				LogInfo("    PWM Pin: %u", pwmPin);
				LogInfo("    PWM Bright: %u", pwmBright);
				LogInfo("    PWM Dim: %u", pwmDim);
			}

			LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");
			if (displayClock)
				LogInfo("    Display UTC: %s", utc ? "yes" : "no");

			m_display = new CHD44780(rows, columns, conf.getCallsign(), dmrid, pins, i2cAddress, pwm, pwmPin, pwmBright, pwmDim, displayClock, utc, conf.getDuplex());
		}
#endif

#if defined(OLED)
	} else if (type == "OLED") {
        unsigned char type       = conf.getOLEDType();
        unsigned char brightness = conf.getOLEDBrightness();
        bool          invert     = conf.getOLEDInvert();
		bool          scroll     = conf.getOLEDScroll();
		display = new COLED(type, brightness, invert, scroll, conf.getDMRNetworkSlot1(), conf.getDMRNetworkSlot2());
#endif
	} else {
		LogWarning("No valid display found, disabling");
		display = new CNullDisplay;
	}

	bool ret = display->open();
	if (!ret) {
		delete display;
		display = new CNullDisplay;
	}
    return display;
}