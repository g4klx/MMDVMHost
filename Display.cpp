/*
 *   Copyright (C) 2016,2017,2018,2020 by Jonathan Naylor G4KLX
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

#include "Display.h"
#include "Defines.h"
#include "SerialController.h"
#include "ModemSerialPort.h"
#include "NullDisplay.h"
#include "TFTSerial.h"
#include "TFTSurenoo.h"
#include "LCDproc.h"
#include "Nextion.h"
#include "CASTInfo.h"
#include "Conf.h"
#include "Modem.h"
#include "UMP.h"
#include "Log.h"

#if defined(HD44780)
#include "HD44780.h"
#endif

#if defined(OLED)
#include "OLED.h"
#endif

#include <cstdio>
#include <cassert>
#include <cstring>

CDisplay::CDisplay() :
m_timer1(3000U, 3U),
m_timer2(3000U, 3U),
m_mode1(MODE_IDLE),
m_mode2(MODE_IDLE)
{
}

CDisplay::~CDisplay()
{
}

void CDisplay::setIdle()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setIdleInt();
}

void CDisplay::setLockout()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setLockoutInt();
}

void CDisplay::setError(const char* text)
{
	assert(text != NULL);

	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setErrorInt(text);
}

void CDisplay::setQuit()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_QUIT;
	m_mode2 = MODE_QUIT;

	setQuitInt();
}

void CDisplay::setFM()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_FM;
	m_mode2 = MODE_FM;

	setFMInt();
}

void CDisplay::writeDStar(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeDStarInt(my1, my2, your, type, reflector);
}

void CDisplay::writeDStarRSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeDStarRSSIInt(rssi);
}

void CDisplay::writeDStarBER(float ber)
{
	writeDStarBERInt(ber);
}

void CDisplay::clearDStar()
{
	if (m_timer1.hasExpired()) {
		clearDStarInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_DSTAR;
	}
}

void CDisplay::writeDMR(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (slotNo == 1U) {
		m_timer1.start();
		m_mode1 = MODE_IDLE;
	} else {
		m_timer2.start();
		m_mode2 = MODE_IDLE;
	}
	writeDMRInt(slotNo, src, group, dst, type);
}

void CDisplay::writeDMR(unsigned int slotNo, const class CUserDBentry& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (slotNo == 1U) {
		m_timer1.start();
		m_mode1 = MODE_IDLE;
	} else {
		m_timer2.start();
		m_mode2 = MODE_IDLE;
	}

	if (int err = writeDMRIntEx(slotNo, src, group, dst, type)) {
		std::string src_str = src.get(keyCALLSIGN);
		if (err < 0 && !src.get(keyFIRST_NAME).empty()) {
		  	// emulate the result of old CDMRLookup::findWithName()
			//  (it returned callsign and firstname)
			src_str += " " + src.get(keyFIRST_NAME);
		}
		writeDMRInt(slotNo, src_str, group, dst, type);
	}
}

void CDisplay::writeDMRRSSI(unsigned int slotNo, unsigned char rssi)
{
	if (rssi != 0U)
		writeDMRRSSIInt(slotNo, rssi);
}

void CDisplay::writeDMRTA(unsigned int slotNo, unsigned char* talkerAlias, const char* type)
{
    if (strcmp(type," ")==0) { writeDMRTAInt(slotNo, (unsigned char*)"", type); return; }
    if (strlen((char*)talkerAlias)>=4U) writeDMRTAInt(slotNo, (unsigned char*)talkerAlias, type);
}

void CDisplay::writeDMRBER(unsigned int slotNo, float ber)
{
	writeDMRBERInt(slotNo, ber);
}

void CDisplay::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		if (m_timer1.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer1.stop();
			m_mode1 = MODE_IDLE;
		} else {
			m_mode1 = MODE_DMR;
		}
	} else {
		if (m_timer2.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer2.stop();
			m_mode2 = MODE_IDLE;
		} else {
			m_mode2 = MODE_DMR;
		}
	}
}

void CDisplay::writeFusion(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeFusionInt(source, dest, dgid, type, origin);
}

void CDisplay::writeFusionRSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeFusionRSSIInt(rssi);
}

void CDisplay::writeFusionBER(float ber)
{
	writeFusionBERInt(ber);
}

void CDisplay::clearFusion()
{
	if (m_timer1.hasExpired()) {
		clearFusionInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_YSF;
	}
}

void CDisplay::writeP25(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeP25Int(source, group, dest, type);
}

void CDisplay::writeP25RSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeP25RSSIInt(rssi);
}

void CDisplay::writeP25BER(float ber)
{
	writeP25BERInt(ber);
}

void CDisplay::clearP25()
{
	if (m_timer1.hasExpired()) {
		clearP25Int();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_P25;
	}
}

void CDisplay::writeNXDN(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeNXDNInt(source, group, dest, type);
}

void CDisplay::writeNXDN(const class CUserDBentry& source, bool group, unsigned int dest, const char* type)
{
	assert(type != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	if (writeNXDNIntEx(source, group, dest, type))
		writeNXDNInt(source.get(keyCALLSIGN).c_str(), group, dest, type);
}

void CDisplay::writeNXDNRSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeNXDNRSSIInt(rssi);
}

void CDisplay::writeNXDNBER(float ber)
{
	writeNXDNBERInt(ber);
}

void CDisplay::clearNXDN()
{
	if (m_timer1.hasExpired()) {
		clearNXDNInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_NXDN;
	}
}

void CDisplay::writePOCSAG(uint32_t ric, const std::string& message)
{
	m_timer1.start();
	m_mode1 = MODE_POCSAG;

	writePOCSAGInt(ric, message);
}

void CDisplay::clearPOCSAG()
{
	if (m_timer1.hasExpired()) {
		clearPOCSAGInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_POCSAG;
	}
}

void CDisplay::writeCW()
{
	m_timer1.start();
	m_mode1 = MODE_CW;

	writeCWInt();
}

void CDisplay::clock(unsigned int ms)
{
	m_timer1.clock(ms);
	if (m_timer1.isRunning() && m_timer1.hasExpired()) {
		switch (m_mode1) {
		case MODE_DSTAR:
			clearDStarInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_DMR:
			clearDMRInt(1U);
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_YSF:
			clearFusionInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_P25:
			clearP25Int();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_NXDN:
			clearNXDNInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_POCSAG:
			clearPOCSAGInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_CW:
			clearCWInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		default:
			break;
		}
	}

	// Timer/mode 2 are only used for DMR
	m_timer2.clock(ms);
	if (m_timer2.isRunning() && m_timer2.hasExpired()) {
		if (m_mode2 == MODE_DMR) {
			clearDMRInt(2U);
			m_mode2 = MODE_IDLE;
			m_timer2.stop();
		}
	}

	clockInt(ms);
}

void CDisplay::clockInt(unsigned int ms)
{
}

void CDisplay::writeDStarRSSIInt(unsigned char rssi)
{
}

void CDisplay::writeDStarBERInt(float ber)
{
}

int CDisplay::writeDMRIntEx(unsigned int slotNo, const class CUserDBentry& src, bool group, const std::string& dst, const char* type)
{
	/*
	 * return value:
	 *	< 0	error condition (i.e. not supported)
	 *		-> call writeXXXXInt() to display
	 *	= 0	no error, writeXXXXIntEx() displayed whole status
	 *	= 1	no error, writeXXXXIntEx() displayed partial status
	 *		-> call writeXXXXInt() to display remain part
	 *	> 1	reserved for future use
	 */
	return -1;	// not supported
}

void CDisplay::writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi)
{
}

void CDisplay::writeDMRTAInt(unsigned int slotNo, unsigned char* talkerAlias, const char* type)
{
}

void CDisplay::writeDMRBERInt(unsigned int slotNo, float ber)
{
}

void CDisplay::writeFusionRSSIInt(unsigned char rssi)
{
}

void CDisplay::writeFusionBERInt(float ber)
{
}

void CDisplay::writeP25RSSIInt(unsigned char rssi)
{
}

void CDisplay::writeP25BERInt(float ber)
{
}

void CDisplay::writeNXDNRSSIInt(unsigned char rssi)
{
}

void CDisplay::writeNXDNBERInt(float ber)
{
}

int CDisplay::writeNXDNIntEx(const class CUserDBentry& source, bool group, unsigned int dest, const char* type)
{
	/* return value definition is same as writeDMRIntEx() */
	return -1;	// not supported
}

	
/* Factory method extracted from MMDVMHost.cpp - BG5HHP */
CDisplay* CDisplay::createDisplay(const CConf& conf, CUMP* ump, CModem* modem)
{
        CDisplay *display = NULL;

        std::string type   = conf.getDisplay();
	unsigned int dmrid = conf.getDMRId();

	LogInfo("Display Parameters");
	LogInfo("    Type: %s", type.c_str());

	if (type == "TFT Serial" || type == "TFT Surenoo") {
		std::string port        = conf.getTFTSerialPort();
		unsigned int brightness = conf.getTFTSerialBrightness();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);

		ISerialPort* serial = NULL;
		if (port == "modem")
			serial = new CModemSerialPort(modem);
		else
			serial = new CSerialController(port, (type == "TFT Serial") ? SERIAL_9600 : SERIAL_115200);

		if (type == "TFT Surenoo")
			display = new CTFTSurenoo(conf.getCallsign(), dmrid, serial, brightness, conf.getDuplex());
		else
			display = new CTFTSerial(conf.getCallsign(), dmrid, serial, brightness);
	} else if (type == "Nextion") {
		std::string port            = conf.getNextionPort();
		unsigned int brightness     = conf.getNextionBrightness();
		bool displayClock           = conf.getNextionDisplayClock();
		bool utc                    = conf.getNextionUTC();
		unsigned int idleBrightness = conf.getNextionIdleBrightness();
		unsigned int screenLayout   = conf.getNextionScreenLayout();
		unsigned int txFrequency    = conf.getTXFrequency();
		unsigned int rxFrequency    = conf.getRXFrequency();
		bool displayTempInF         = conf.getNextionTempInFahrenheit();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);
		LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");
		if (displayClock)
			LogInfo("    Display UTC: %s", utc ? "yes" : "no");
		LogInfo("    Idle Brightness: %u", idleBrightness);
		LogInfo("    Temperature in Fahrenheit: %s ", displayTempInF ? "yes" : "no");
 
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
			display = new CNextion(conf.getCallsign(), dmrid, serial, brightness, displayClock, utc, idleBrightness, screenLayout, txFrequency, rxFrequency, displayTempInF, conf.getLocation());
		} else if (port == "ump") {
			if (ump != NULL) {
				display = new CNextion(conf.getCallsign(), dmrid, ump, brightness, displayClock, utc, idleBrightness, screenLayout, txFrequency, rxFrequency, displayTempInF, conf.getLocation());
                        } else {
                		LogInfo("    NullDisplay loaded");
                		display = new CNullDisplay;
                        }
		} else {
			SERIAL_SPEED baudrate = SERIAL_9600;
			if (screenLayout&0x0cU)
				baudrate = SERIAL_115200;
			
			LogInfo("    Display baudrate: %u ",baudrate);
			ISerialPort* serial = new CSerialController(port, baudrate);
			display = new CNextion(conf.getCallsign(), dmrid, serial, brightness, displayClock, utc, idleBrightness, screenLayout, txFrequency, rxFrequency, displayTempInF, conf.getLocation());
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

			display = new CHD44780(rows, columns, conf.getCallsign(), dmrid, pins, i2cAddress, pwm, pwmPin, pwmBright, pwmDim, displayClock, utc, conf.getDuplex());
		}
#endif
#if defined(OLED)
	} else if (type == "OLED") {
	        unsigned char type       = conf.getOLEDType();
	        unsigned char brightness = conf.getOLEDBrightness();
	        bool          invert     = conf.getOLEDInvert();
	        bool          scroll     = conf.getOLEDScroll();
		bool          rotate     = conf.getOLEDRotate();
		bool          logosaver  = conf.getOLEDLogoScreensaver();

		display = new COLED(type, brightness, invert, scroll, rotate, logosaver, conf.getDMRNetworkSlot1(), conf.getDMRNetworkSlot2());
#endif
	} else if (type == "CAST") {
		display = new CCASTInfo(modem);
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
