/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "MMDVMHost.h"
#include "Log.h"
#include "Version.h"
#include "StopWatch.h"
#include "Defines.h"
#include "DMRControl.h"
#include "TFTSerial.h"
#include "NullDisplay.h"

#include "DStarEcho.h"
#include "YSFEcho.h"

#include <cstdio>

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#include <signal.h>
#endif

static bool m_killed = false;

#if !defined(_WIN32) && !defined(_WIN64)
static void sigHandler(int)
{
  m_killed = true;
}
#endif

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2015, 2016 by Jonathan Naylor, G4KLX";

int main(int argc, char** argv)
{
  if (argc == 1) {
    ::fprintf(stderr, "Usage: MMDVMHost <conf file>\n");
    return 1;
  }

#if !defined(_WIN32) && !defined(_WIN64)
  ::signal(SIGUSR1, sigHandler);
#endif

  CMMDVMHost* host = new CMMDVMHost(std::string(argv[1]));
  int ret2 = host->run();

  delete host;

  ::LogFinalise();

  return ret2;
}

CMMDVMHost::CMMDVMHost(const std::string& confFile) :
m_conf(confFile),
m_modem(NULL),
m_dstarNetwork(NULL),
m_dmrNetwork(NULL),
m_display(NULL),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_ysfEnabled(false)
{
}

CMMDVMHost::~CMMDVMHost()
{
}

int CMMDVMHost::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "MMDVMHost: cannot read the .ini file\n");
		return 1;
	}

	ret = ::LogInitialise(m_conf.getLogPath(), m_conf.getLogRoot(), m_conf.getLogDisplay());
	if (!ret) {
		::fprintf(stderr, "MMDVMHost: unable to open the log file\n");
		return 1;
	}

	::LogSetLevel(m_conf.getLogLevel());

	LogInfo(HEADER1);
	LogInfo(HEADER2);
	LogInfo(HEADER3);
	LogInfo(HEADER4);

	LogMessage("MMDVMHost-%s is starting", VERSION);

	readParams();

	ret = createModem();
	if (!ret)
		return 1;

	createDisplay();

	if (m_dstarEnabled && m_conf.getDStarNetworkEnabled()) {
		ret = createDStarNetwork();
		if (!ret)
			return 1;
	}

	if (m_dmrEnabled && m_conf.getDMRNetworkEnabled()) {
		ret = createDMRNetwork();
		if (!ret)
			return 1;
	}

	CTimer dmrBeaconTimer(1000U, 4U);
	bool dmrBeaconsEnabled = m_dmrEnabled && m_conf.getDMRBeacons();

	CStopWatch stopWatch;
	stopWatch.start();

	CDStarEcho* dstar = NULL;
	if (m_dstarEnabled)
		dstar = new CDStarEcho(2U, 10000U);

	CDMRControl* dmr = NULL;
	if (m_dmrEnabled) {
		unsigned int id        = m_conf.getDMRId();
		unsigned int colorCode = m_conf.getDMRColorCode();
		unsigned int timeout   = m_conf.getTimeout();

		LogInfo("DMR Parameters");
		LogInfo("    Id: %u", id);
		LogInfo("    Color Code: %u", colorCode);
		LogInfo("    Timeout: %us", timeout);

		dmr = new CDMRControl(id, colorCode, timeout, m_modem, m_dmrNetwork, m_display);
	}

	CYSFEcho* ysf = NULL;
	if (m_ysfEnabled)
		ysf = new CYSFEcho(2U, 10000U);

	unsigned char mode = MODE_IDLE;
	CTimer modeTimer(1000U, m_conf.getModeHang());

	m_display->setIdle();

	while (!m_killed) {
		unsigned char data[200U];
		unsigned int len;
		bool ret;

		len = m_modem->readDStarData(data);
		if (dstar != NULL && len > 0U) {
			if (mode == MODE_IDLE) {
				LogMessage("Mode set to D-Star");
				mode = MODE_DSTAR;
				m_display->setDStar();
				m_modem->setMode(MODE_DSTAR);
				if (m_dmrNetwork != NULL)
					m_dmrNetwork->enable(false);
			}

			if (mode == MODE_DSTAR) {
				dstar->writeData(data, len);
				modeTimer.start();
			}
		}

		len = m_modem->readDMRData1(data);
		if (dmr != NULL && len > 0U) {
			if (mode == MODE_IDLE) {
				bool ret = dmr->processWakeup(data);
				if (ret) {
					LogMessage("Mode set to DMR");
					mode = MODE_DMR;
					m_display->setDMR();
					// This sets the mode to DMR within the modem
					m_modem->writeDMRStart(true);
					if (m_dstarNetwork != NULL)
						m_dstarNetwork->enable(false);
					modeTimer.start();
				}
			} else if (mode == MODE_DMR) {
				dmr->writeModemSlot1(data);
				dmrBeaconTimer.stop();
				modeTimer.start();
			}
		}

		len = m_modem->readDMRData2(data);
		if (dmr != NULL && len > 0U) {
			if (mode == MODE_IDLE) {
				bool ret = dmr->processWakeup(data);
				if (ret) {
					LogMessage("Mode set to DMR");
					mode = MODE_DMR;
					m_display->setDMR();
					// This sets the mode to DMR within the modem
					m_modem->writeDMRStart(true);
					if (m_dstarNetwork != NULL)
						m_dstarNetwork->enable(false);
					modeTimer.start();
				}
			} else if (mode == MODE_DMR) {
				dmr->writeModemSlot2(data);
				dmrBeaconTimer.stop();
				modeTimer.start();
			}
		}

		len = m_modem->readYSFData(data);
		if (ysf != NULL && len > 0U) {
			if (mode == MODE_IDLE) {
				LogMessage("Mode set to System Fusion");
				mode = MODE_YSF;
				m_display->setFusion();
				m_modem->setMode(MODE_YSF);
				if (m_dmrNetwork != NULL)
					m_dmrNetwork->enable(false);
				if (m_dstarNetwork != NULL)
					m_dstarNetwork->enable(false);
			}

			if (mode == MODE_YSF) {
				ysf->writeData(data, len);
				modeTimer.start();
			}
		}

		if (modeTimer.isRunning() && modeTimer.hasExpired()) {
			LogMessage("Mode set to Idle");

			if (mode == MODE_DMR)
				m_modem->writeDMRStart(false);

			mode = MODE_IDLE;
			m_display->setIdle();
			m_modem->setMode(MODE_IDLE);

			if (m_dmrNetwork != NULL)
				m_dmrNetwork->enable(true);
			if (m_dstarNetwork != NULL)
				m_dstarNetwork->enable(true);

			modeTimer.stop();
		}

		if (dstar != NULL) {
			ret = m_modem->hasDStarSpace();
			if (ret) {
				len = dstar->readData(data);

				if (len > 0U && mode != MODE_DSTAR) {
					LogMessage("Mode set to D-Star");
					mode = MODE_DSTAR;
					m_display->setDStar();
					m_modem->setMode(MODE_DSTAR);
					if (m_dmrNetwork != NULL)
						m_dmrNetwork->enable(false);
				}

				if (len > 0U && mode == MODE_DSTAR) {
					m_modem->writeDStarData(data, len);
					modeTimer.start();
				}
			}
		}

		if (dmr != NULL) {
			ret = m_modem->hasDMRSpace1();
			if (ret) {
				len = dmr->readModemSlot1(data);

				if (len > 0U && mode == MODE_IDLE) {
					LogMessage("Mode set to DMR");
					m_modem->setMode(MODE_DMR);
					m_display->setDMR();
					mode = MODE_DMR;
					if (m_dstarNetwork != NULL)
						m_dstarNetwork->enable(false);
				}

				if (len > 0U && mode == MODE_DMR) {
					m_modem->writeDMRData1(data, len);
					dmrBeaconTimer.stop();
					modeTimer.start();
				}
			}

			ret = m_modem->hasDMRSpace2();
			if (ret) {
				len = dmr->readModemSlot2(data);

				if (len > 0U && mode == MODE_IDLE) {
					LogMessage("Mode set to DMR");
					m_modem->setMode(MODE_DMR);
					m_display->setDMR();
					mode = MODE_DMR;
					if (m_dstarNetwork != NULL)
						m_dstarNetwork->enable(false);
				}

				if (len > 0U && mode == MODE_DMR) {
					m_modem->writeDMRData2(data, len);
					dmrBeaconTimer.stop();
					modeTimer.start();
				}
			}
		}

		if (ysf != NULL) {
			ret = m_modem->hasYSFSpace();
			if (ret) {
				len = ysf->readData(data);

				if (len > 0U && mode != MODE_YSF) {
					LogMessage("Mode set to System Fusion");
					mode = MODE_YSF;
					m_display->setFusion();
					m_modem->setMode(MODE_YSF);
					if (m_dmrNetwork != NULL)
						m_dmrNetwork->enable(false);
					if (m_dstarNetwork != NULL)
						m_dstarNetwork->enable(false);
				}

				if (len > 0U && mode == MODE_YSF) {
					m_modem->writeYSFData(data, len);
					modeTimer.start();
				}
			}
		}

		if (m_dmrNetwork != NULL) {
			bool run = m_dmrNetwork->wantsBeacon();

			if (dmrBeaconsEnabled && run && mode == MODE_IDLE) {
				mode = MODE_DMR;
				m_modem->writeDMRStart(true);
				dmrBeaconTimer.start();
			}
		}

		unsigned int ms = stopWatch.elapsed();
		m_modem->clock(ms);
		modeTimer.clock(ms);
		if (dstar != NULL)
			dstar->clock(ms);
		if (dmr != NULL)
			dmr->clock(ms);
		if (ysf != NULL)
			ysf->clock(ms);
		stopWatch.start();

		dmrBeaconTimer.clock(ms);
		if (dmrBeaconTimer.isRunning() && dmrBeaconTimer.hasExpired()) {
			dmrBeaconTimer.stop();
			m_modem->writeDMRStart(false);
			mode = MODE_IDLE;
		}

		if (ms < 5U) {
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(5UL);		// 5ms
#else
			::usleep(5000);		// 5ms
#endif
		}
	}

	LogMessage("MMDVMHost is exiting on receipt of SIGHUP1");

	m_display->setIdle();

	m_modem->close();
	delete m_modem;

	m_display->close();
	delete m_display;

	if (m_dstarNetwork != NULL) {
		m_dstarNetwork->close();
		delete m_dstarNetwork;
	}

	if (m_dmrNetwork != NULL) {
		m_dmrNetwork->close();
		delete m_dmrNetwork;
	}

	delete dstar;
	delete dmr;
	delete ysf;

	return 0;
}

bool CMMDVMHost::createModem()
{
    std::string port       = m_conf.getModemPort();
    bool rxInvert          = m_conf.getModemRXInvert();
    bool txInvert          = m_conf.getModemTXInvert();
    bool pttInvert         = m_conf.getModemPTTInvert();
    unsigned int txDelay   = m_conf.getModemTXDelay();
    unsigned int rxLevel   = m_conf.getModemRXLevel();
    unsigned int txLevel   = m_conf.getModemTXLevel();
    bool debug             = m_conf.getModemDebug();
	unsigned int colorCode = m_conf.getDMRColorCode();

	LogInfo("Modem Parameters");
	LogInfo("    Port: %s", port.c_str());
	LogInfo("    RX Invert: %s", rxInvert ? "yes" : "no");
	LogInfo("    TX Invert: %s", txInvert ? "yes" : "no");
	LogInfo("    PTT Invert: %s", pttInvert ? "yes" : "no");
	LogInfo("    TX Delay: %u", txDelay);
	LogInfo("    RX Level: %u", rxLevel);
	LogInfo("    TX Level: %u", txLevel);

	m_modem = new CModem(port, rxInvert, txInvert, pttInvert, txDelay, rxLevel, txLevel, debug);
	m_modem->setModeParams(m_dstarEnabled, m_dmrEnabled, m_ysfEnabled);
	m_modem->setDMRParams(colorCode);

	bool ret = m_modem->open();
	if (!ret) {
		delete m_modem;
		m_modem = NULL;
		return false;
	}

	return true;
}

bool CMMDVMHost::createDStarNetwork()
{
	std::string gatewayAddress = m_conf.getDStarGatewayAddress();
	unsigned int gatewayPort = m_conf.getDStarGatewayPort();
	unsigned int localPort = m_conf.getDStarLocalPort();
	bool debug = m_conf.getDStarNetworkDebug();

	LogInfo("D-Star Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Local Port: %u", localPort);

	m_dstarNetwork = new CDStarNetwork(gatewayAddress, gatewayPort, localPort, VERSION, debug);

	bool ret = m_dstarNetwork->open();
	if (!ret) {
		delete m_dstarNetwork;
		m_dstarNetwork = NULL;
		return false;
	}

	m_dstarNetwork->enable(true);

	return true;
}

bool CMMDVMHost::createDMRNetwork()
{
	std::string address  = m_conf.getDMRNetworkAddress();
	unsigned int port    = m_conf.getDMRNetworkPort();
	unsigned int id      = m_conf.getDMRId();
	std::string password = m_conf.getDMRNetworkPassword();
	bool debug           = m_conf.getDMRNetworkDebug();

	LogInfo("DMR Network Parameters");
	LogInfo("    Address: %s", address.c_str());
	LogInfo("    Port: %u", port);

	m_dmrNetwork = new CHomebrewDMRIPSC(address, port, id, password, VERSION, "MMDVM", debug);

	std::string callsign     = m_conf.getCallsign();
	unsigned int rxFrequency = m_conf.getRxFrequency();
	unsigned int txFrequency = m_conf.getTxFrequency();
	unsigned int power       = m_conf.getPower();
	unsigned int colorCode   = m_conf.getDMRColorCode();
	float latitude           = m_conf.getLatitude();
	float longitude          = m_conf.getLongitude();
	int height               = m_conf.getHeight();
	std::string location     = m_conf.getLocation();
	std::string description  = m_conf.getDescription();
	std::string url          = m_conf.getURL();

	LogInfo("Info Parameters");
	LogInfo("    Callsign: %s", callsign.c_str());
	LogInfo("    RX Frequency: %uHz", rxFrequency);
	LogInfo("    TX Frequency: %uHz", txFrequency);
	LogInfo("    Power: %uW", power);
	LogInfo("    Latitude: %fdeg N", latitude);
	LogInfo("    Longitude: %fdeg E", longitude);
	LogInfo("    Height: %um", height);
	LogInfo("    Location: \"%s\"", location.c_str());
	LogInfo("    Description: \"%s\"", description.c_str());
	LogInfo("    URL: \"%s\"", url.c_str());

	m_dmrNetwork->setConfig(callsign, rxFrequency, txFrequency, power, colorCode, latitude, longitude, height, location, description, url);

	bool ret = m_dmrNetwork->open();
	if (!ret) {
		delete m_dmrNetwork;
		m_dmrNetwork = NULL;
		return false;
	}

	m_dmrNetwork->enable(true);

	return true;
}

void CMMDVMHost::readParams()
{
	m_dstarEnabled = m_conf.getDStarEnabled();
	m_dmrEnabled   = m_conf.getDMREnabled();
	m_ysfEnabled   = m_conf.getFusionEnabled();

	if (!m_conf.getDuplex() && m_dmrEnabled) {
		LogWarning("DMR operation disabled because system is not duplex");
		m_dmrEnabled = false;
	}
}

void CMMDVMHost::createDisplay()
{
	std::string type = m_conf.getDisplay();

	LogInfo("Display Parameters");
	LogInfo("    Type: %s", type.c_str());

	if (type == "TFT Serial") {
		std::string port = m_conf.getTFTSerialPort();

		LogInfo("    Port: %s", port.c_str());

		m_display = new CTFTSerial(port);
	} else {
		m_display = new CNullDisplay;
	}

	bool ret = m_display->open();
	if (!ret) {
		delete m_display;
		m_display = new CNullDisplay;
	}
}
