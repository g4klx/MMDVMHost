/*
 *   Copyright (C) 2015,2016,2017,2018 by Jonathan Naylor G4KLX
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
#include "MMDVMTask.h"
#include "RSSIInterpolator.h"
#include "SerialController.h"
#include "Version.h"
#include "StopWatch.h"
#include "Defines.h"
#include "DStarControl.h"
#include "DMRControl.h"
#include "YSFControl.h"
#include "P25Control.h"
#include "NXDNControl.h"
#include "POCSAGControl.h"
#include "Thread.h"
#include "Log.h"
#include "GitVersion.h"

#include <cstdio>
#include <vector>

#include <cstdlib>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "MMDVM.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/MMDVM.ini";
#endif

static bool m_killed = false;
static int  m_signal = 0;

#if !defined(_WIN32) && !defined(_WIN64)
static void sigHandler(int signum)
{
	m_killed = true;
	m_signal = signum;
}
#endif

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2015-2018 by Jonathan Naylor, G4KLX and others";

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
 		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "MMDVMHost version %s git #%.7s\n", VERSION, gitversion);
				return 0;
			} else if (arg.substr(0,1) == "-") {
				::fprintf(stderr, "Usage: MMDVMHost [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

#if !defined(_WIN32) && !defined(_WIN64)
	::signal(SIGINT,  sigHandler);
	::signal(SIGTERM, sigHandler);
	::signal(SIGHUP,  sigHandler);
#endif

	int ret = 0;

	do {
		m_signal = 0;

		CMMDVMHost* host = new CMMDVMHost(std::string(iniFile));
		ret = host->run();

		delete host;

		if (m_signal == 2)
			::LogInfo("MMDVMHost-%s exited on receipt of SIGINT", VERSION);

		if (m_signal == 15)
			::LogInfo("MMDVMHost-%s exited on receipt of SIGTERM", VERSION);

		if (m_signal == 1)
			::LogInfo("MMDVMHost-%s is restarting on receipt of SIGHUP", VERSION);
	} while (m_signal == 1);

	::LogFinalise();

	return ret;
}

CMMDVMHost::CMMDVMHost(const std::string& confFile) :
m_conf(confFile),
m_modem(NULL),
m_dstarNetwork(NULL),
m_dmrTask(NULL),
m_ysfNetwork(NULL),
m_p25Network(NULL),
m_nxdnNetwork(NULL),
m_pocsagNetwork(NULL),
m_display(NULL),
m_ump(NULL),
m_mode(MODE_IDLE),
m_dstarRFModeHang(10U),
m_ysfRFModeHang(10U),
m_p25RFModeHang(10U),
m_nxdnRFModeHang(10U),
m_dstarNetModeHang(3U),
m_ysfNetModeHang(3U),
m_p25NetModeHang(3U),
m_nxdnNetModeHang(3U),
m_pocsagNetModeHang(3U),
m_modeTimer(1000U),
m_cwIdTimer(1000U),
m_duplex(false),
m_timeout(180U),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_ysfEnabled(false),
m_p25Enabled(false),
m_nxdnEnabled(false),
m_pocsagEnabled(false),
m_cwIdTime(0U),
m_dmrLookup(NULL),
m_nxdnLookup(NULL),
m_callsign(),
m_id(0U),
m_cwCallsign()
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

	ret = daemonize();
	if (!ret) {
		return -1;
	}

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), m_conf.getLogDisplayLevel());
	if (!ret) {
		::fprintf(stderr, "MMDVMHost: unable to open the log file\n");
		return 1;
	}

	LogInfo(HEADER1);
	LogInfo(HEADER2);
	LogInfo(HEADER3);
	LogInfo(HEADER4);

	LogMessage("MMDVMHost-%s is starting", VERSION);
	LogMessage("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	readParams();

	ret = createModem();
	if (!ret)
		return 1;

	if (m_conf.getUMPEnabled()) {
		std::string port = m_conf.getUMPPort();

		LogInfo("Universal MMDVM Peripheral");
		LogInfo("    Port: %s", port.c_str());

		m_ump = new CUMP(port);
		ret = m_ump->open();
		if (!ret) {
			delete m_ump;
			m_ump = NULL;
		}
	}

	m_display = CDisplay::createDisplay(m_conf,m_ump,m_modem);

	ret = createDStarNetwork();
	if (!ret)
		return 1;

	ret = createYSFNetwork();
	if (!ret)
		return 1;

	ret = createP25Network();
	if (!ret)
		return 1;

	ret = createNXDNNetwork();
	if (!ret)
		return 1;

	ret = createPOCSAGNetwork();
	if (!ret)
		return 1;

	in_addr transparentAddress;
	unsigned int transparentPort = 0U;
	CUDPSocket* transparentSocket = NULL;

	if (m_conf.getTransparentEnabled()) {
		std::string remoteAddress = m_conf.getTransparentRemoteAddress();
		unsigned int remotePort   = m_conf.getTransparentRemotePort();
		unsigned int localPort    = m_conf.getTransparentLocalPort();

		LogInfo("Transparent Data");
		LogInfo("    Remote Address: %s", remoteAddress.c_str());
		LogInfo("    Remote Port: %u", remotePort);
		LogInfo("    Local Port: %u", localPort);

		transparentAddress = CUDPSocket::lookup(remoteAddress);
		transparentPort    = remotePort;

		transparentSocket = new CUDPSocket(localPort);
		ret = transparentSocket->open();
		if (!ret) {
			LogWarning("Could not open the Transparent data socket, disabling");
			delete transparentSocket;
			transparentSocket = NULL;
		}
	}

	if (m_conf.getCWIdEnabled()) {
		unsigned int time = m_conf.getCWIdTime();
		m_cwCallsign      = m_conf.getCWIdCallsign();

		LogInfo("CW Id Parameters");
		LogInfo("    Time: %u mins", time);
		LogInfo("    Callsign: %s", m_cwCallsign.c_str());

		m_cwIdTime = time * 60U;

		m_cwIdTimer.setTimeout(m_cwIdTime / 4U);
		m_cwIdTimer.start();
	}

	// For all modes we handle RSSI
	std::string rssiMappingFile = m_conf.getModemRSSIMappingFile();

	CRSSIInterpolator* rssi = new CRSSIInterpolator;
	if (!rssiMappingFile.empty()) {
		LogInfo("RSSI");
		LogInfo("    Mapping File: %s", rssiMappingFile.c_str());
		rssi->load(rssiMappingFile);
	}

	// For DMR and P25 we try to map IDs to callsigns
	if (m_dmrEnabled || m_p25Enabled) {
		std::string lookupFile  = m_conf.getDMRIdLookupFile();
		unsigned int reloadTime = m_conf.getDMRIdLookupTime();

		LogInfo("DMR Id Lookups");
		LogInfo("    File: %s", lookupFile.length() > 0U ? lookupFile.c_str() : "None");
		if (reloadTime > 0U)
			LogInfo("    Reload: %u hours", reloadTime);

		m_dmrLookup = new CDMRLookup(lookupFile, reloadTime);
		m_dmrLookup->read();
	}

	CStopWatch stopWatch;
	stopWatch.start();

	CDStarControl* dstar = createDStarControl(rssi);

	CMMDVMTaskContext ctx;
	ctx.host = this;
	ctx.rssi = rssi;
	if (m_dmrEnabled)
	{
		m_dmrTask = CDMRTask::create(ctx);
		assert(m_dmrTask);
	}

	CYSFControl* ysf = createYSFControl(rssi);

	CP25Control* p25 = createP25Control(rssi);

	CNXDNControl* nxdn = createNXDNControl(rssi);

	CTimer pocsagTimer(1000U, 30U);
	CPOCSAGControl* pocsag = createPOCSAGControl(pocsagTimer);

	setMode(MODE_IDLE);

	LogMessage("MMDVMHost-%s is running", VERSION);

	while (!m_killed) {
		bool lockout1 = m_modem->hasLockout();
		bool lockout2 = false;
		if (m_ump != NULL)
			lockout2 = m_ump->getLockout();
		if ((lockout1 || lockout2) && m_mode != MODE_LOCKOUT)
			setMode(MODE_LOCKOUT);
		else if ((!lockout1 && !lockout2) && m_mode == MODE_LOCKOUT)
			setMode(MODE_IDLE);

		bool error = m_modem->hasError();
		if (error && m_mode != MODE_ERROR)
			setMode(MODE_ERROR);
		else if (!error && m_mode == MODE_ERROR)
			setMode(MODE_IDLE);

		if (m_ump != NULL) {
			bool tx = m_modem->hasTX();
			m_ump->setTX(tx);
			bool cd = m_modem->hasCD();
			m_ump->setCD(cd);
		}

		unsigned char data[200U];
		unsigned int len;
		bool ret;
		ctx.data = data;

		len = m_modem->readDStarData(data);
		if (dstar != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = dstar->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_dstarRFModeHang);
					setMode(MODE_DSTAR);
				}
			} else if (m_mode == MODE_DSTAR) {
				dstar->writeModem(data, len);
				m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("D-Star modem data received when in mode %u", m_mode);
			}
		}

		if (m_dmrTask)
			m_dmrTask->run(&ctx);

		len = m_modem->readYSFData(data);
		if (ysf != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = ysf->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_ysfRFModeHang);
					setMode(MODE_YSF);
				}
			} else if (m_mode == MODE_YSF) {
				ysf->writeModem(data, len);
				m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("System Fusion modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readP25Data(data);
		if (p25 != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = p25->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_p25RFModeHang);
					setMode(MODE_P25);
				}
			} else if (m_mode == MODE_P25) {
				p25->writeModem(data, len);
				m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("P25 modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readNXDNData(data);
		if (nxdn != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = nxdn->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_nxdnRFModeHang);
					setMode(MODE_NXDN);
				}
			} else if (m_mode == MODE_NXDN) {
				nxdn->writeModem(data, len);
				m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("NXDN modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readTransparentData(data);
		if (transparentSocket != NULL && len > 0U)
			transparentSocket->write(data, len, transparentAddress, transparentPort);

		if (m_modeTimer.isRunning() && m_modeTimer.hasExpired())
			setMode(MODE_IDLE);

		if (dstar != NULL) {
			ret = m_modem->hasDStarSpace();
			if (ret) {
				len = dstar->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_dstarNetModeHang);
						setMode(MODE_DSTAR);
					}
					if (m_mode == MODE_DSTAR) {
						m_modem->writeDStarData(data, len);
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("D-Star data received when in mode %u", m_mode);
					}
				}
			}
		}

		if (ysf != NULL) {
			ret = m_modem->hasYSFSpace();
			if (ret) {
				len = ysf->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_ysfNetModeHang);
						setMode(MODE_YSF);
					}
					if (m_mode == MODE_YSF) {
						m_modem->writeYSFData(data, len);
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("System Fusion data received when in mode %u", m_mode);
					}
				}
			}
		}

		if (p25 != NULL) {
			ret = m_modem->hasP25Space();
			if (ret) {
				len = p25->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_p25NetModeHang);
						setMode(MODE_P25);
					}
					if (m_mode == MODE_P25) {
						m_modem->writeP25Data(data, len);
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("P25 data received when in mode %u", m_mode);
					}
				}
			}
		}

		if (nxdn != NULL) {
			ret = m_modem->hasNXDNSpace();
			if (ret) {
				len = nxdn->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_nxdnNetModeHang);
						setMode(MODE_NXDN);
					}
					if (m_mode == MODE_NXDN) {
						m_modem->writeNXDNData(data, len);
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("NXDN data received when in mode %u", m_mode);
					}
				}
			}
		}

		if (pocsag != NULL) {
			ret = m_modem->hasPOCSAGSpace();
			if (ret) {
				len = pocsag->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_pocsagNetModeHang);
						setMode(MODE_POCSAG);
					}
					if (m_mode == MODE_POCSAG) {
						m_modem->writePOCSAGData(data, len);
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("POCSAG data received when in mode %u", m_mode);
					}
				}
			}
		}

		if (transparentSocket != NULL) {
			in_addr address;
			unsigned int port = 0U;
			len = transparentSocket->read(data, 200U, address, port);
			if (len > 0U)
				m_modem->writeTransparentData(data, len);
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_display->clock(ms);

		m_modem->clock(ms);
		m_modeTimer.clock(ms);

		if (dstar != NULL)
			dstar->clock();
		if (ysf != NULL)
			ysf->clock(ms);
		if (p25 != NULL)
			p25->clock(ms);
		if (nxdn != NULL)
			nxdn->clock(ms);
		if (pocsag != NULL)
			pocsag->clock(ms);

		if (m_dstarNetwork != NULL)
			m_dstarNetwork->clock(ms);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->clock(ms);
		if (m_p25Network != NULL)
			m_p25Network->clock(ms);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->clock(ms);
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->clock(ms);

		m_cwIdTimer.clock(ms);
		if (m_cwIdTimer.isRunning() && m_cwIdTimer.hasExpired()) {
			if (m_mode == MODE_IDLE && !m_modem->hasTX()){
				LogDebug("sending CW ID");
				m_display->writeCW();
				m_modem->sendCWId(m_cwCallsign);

				m_cwIdTimer.setTimeout(m_cwIdTime);
				m_cwIdTimer.start();
			}
		}

		pocsagTimer.clock(ms);
		if (pocsagTimer.isRunning() && pocsagTimer.hasExpired()) {
			assert(m_pocsagNetwork != NULL);
			m_pocsagNetwork->enable(m_mode == MODE_IDLE || m_mode == MODE_POCSAG);
			pocsagTimer.start();
		}

		if (m_ump != NULL)
			m_ump->clock(ms);

		if (ms < 5U)
			CThread::sleep(5U);
	}

	setMode(MODE_IDLE);

	m_modem->close();
	delete m_modem;

	m_display->close();
	delete m_display;

	if (m_ump != NULL) {
		m_ump->close();
		delete m_ump;
	}

	if (m_dmrLookup != NULL)
		m_dmrLookup->stop();

	if (m_nxdnLookup != NULL)
		m_nxdnLookup->stop();

	if (m_dstarNetwork != NULL) {
		m_dstarNetwork->close();
		delete m_dstarNetwork;
	}

	if (m_ysfNetwork != NULL) {
		m_ysfNetwork->close();
		delete m_ysfNetwork;
	}

	if (m_p25Network != NULL) {
		m_p25Network->close();
		delete m_p25Network;
	}

	if (m_nxdnNetwork != NULL) {
		m_nxdnNetwork->close();
		delete m_nxdnNetwork;
	}

	if (m_pocsagNetwork != NULL) {
		m_pocsagNetwork->close();
		delete m_pocsagNetwork;
	}

	if (transparentSocket != NULL) {
		transparentSocket->close();
		delete transparentSocket;
	}

	delete dstar;

	if (m_dmrTask != NULL)
	{
		delete m_dmrTask;
		m_dmrTask = NULL;
	}

	delete ysf;
	delete p25;
	delete nxdn;
	delete pocsag;

	return 0;
}

bool CMMDVMHost::createModem()
{
	std::string port             = m_conf.getModemPort();
	std::string protocol	     = m_conf.getModemProtocol();
	unsigned int address	     = m_conf.getModemAddress();
	bool rxInvert                = m_conf.getModemRXInvert();
	bool txInvert                = m_conf.getModemTXInvert();
	bool pttInvert               = m_conf.getModemPTTInvert();
	unsigned int txDelay         = m_conf.getModemTXDelay();
	unsigned int dmrDelay        = m_conf.getModemDMRDelay();
	float rxLevel                = m_conf.getModemRXLevel();
	float cwIdTXLevel            = m_conf.getModemCWIdTXLevel();
	float dstarTXLevel           = m_conf.getModemDStarTXLevel();
	float dmrTXLevel             = m_conf.getModemDMRTXLevel();
	float ysfTXLevel             = m_conf.getModemYSFTXLevel();
	float p25TXLevel             = m_conf.getModemP25TXLevel();
	float nxdnTXLevel            = m_conf.getModemNXDNTXLevel();
	float pocsagTXLevel          = m_conf.getModemPOCSAGTXLevel();
	bool trace                   = m_conf.getModemTrace();
	bool debug                   = m_conf.getModemDebug();
	unsigned int colorCode       = m_conf.getDMRColorCode();
	bool lowDeviation            = m_conf.getFusionLowDeviation();
	unsigned int txHang          = m_conf.getFusionTXHang();
	unsigned int rxFrequency     = m_conf.getRXFrequency();
	unsigned int txFrequency     = m_conf.getTXFrequency();
	unsigned int pocsagFrequency = m_conf.getPOCSAGFrequency();
	int rxOffset                 = m_conf.getModemRXOffset();
	int txOffset                 = m_conf.getModemTXOffset();
	int rxDCOffset               = m_conf.getModemRXDCOffset();
	int txDCOffset               = m_conf.getModemTXDCOffset();
	float rfLevel                = m_conf.getModemRFLevel();

	LogInfo("Modem Parameters");
	LogInfo("    Port: %s", port.c_str());
	LogInfo("    Protocol: %s", protocol.c_str());
	if (protocol == "i2c")
		LogInfo("    i2c Address: %02X", address);
	LogInfo("    RX Invert: %s", rxInvert ? "yes" : "no");
	LogInfo("    TX Invert: %s", txInvert ? "yes" : "no");
	LogInfo("    PTT Invert: %s", pttInvert ? "yes" : "no");
	LogInfo("    TX Delay: %ums", txDelay);
	LogInfo("    RX Offset: %dHz", rxOffset);
	LogInfo("    TX Offset: %dHz", txOffset);
	LogInfo("    RX DC Offset: %d", rxDCOffset);
	LogInfo("    TX DC Offset: %d", txDCOffset);
	LogInfo("    RF Level: %.1f%%", rfLevel);
	LogInfo("    DMR Delay: %u (%.1fms)", dmrDelay, float(dmrDelay) * 0.0416666F);
	LogInfo("    RX Level: %.1f%%", rxLevel);
	LogInfo("    CW Id TX Level: %.1f%%", cwIdTXLevel);
	LogInfo("    D-Star TX Level: %.1f%%", dstarTXLevel);
	LogInfo("    DMR TX Level: %.1f%%", dmrTXLevel);
	LogInfo("    YSF TX Level: %.1f%%", ysfTXLevel);
	LogInfo("    P25 TX Level: %.1f%%", p25TXLevel);
	LogInfo("    NXDN TX Level: %.1f%%", nxdnTXLevel);
	LogInfo("    POCSAG TX Level: %.1f%%", pocsagTXLevel);
	LogInfo("    RX Frequency: %uHz (%uHz)", rxFrequency, rxFrequency + rxOffset);
	LogInfo("    TX Frequency: %uHz (%uHz)", txFrequency, txFrequency + txOffset);

	m_modem = new CModem(port, m_duplex, rxInvert, txInvert, pttInvert, txDelay, dmrDelay, trace, debug);
	m_modem->setSerialParams(protocol,address);
	m_modem->setModeParams(m_dstarEnabled, m_dmrEnabled, m_ysfEnabled, m_p25Enabled, m_nxdnEnabled, m_pocsagEnabled);
	m_modem->setLevels(rxLevel, cwIdTXLevel, dstarTXLevel, dmrTXLevel, ysfTXLevel, p25TXLevel, nxdnTXLevel, pocsagTXLevel);
	m_modem->setRFParams(rxFrequency, rxOffset, txFrequency, txOffset, txDCOffset, rxDCOffset, rfLevel, pocsagFrequency);
	m_modem->setDMRParams(colorCode);
	m_modem->setYSFParams(lowDeviation, txHang);

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
	if (!m_dstarEnabled || !m_conf.getDStarNetworkEnabled())
		return true;

	std::string gatewayAddress = m_conf.getDStarGatewayAddress();
	unsigned int gatewayPort   = m_conf.getDStarGatewayPort();
	unsigned int localPort     = m_conf.getDStarLocalPort();
	bool debug                 = m_conf.getDStarNetworkDebug();
	m_dstarNetModeHang         = m_conf.getDStarNetworkModeHang();

	LogInfo("D-Star Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Local Port: %u", localPort);
	LogInfo("    Mode Hang: %us", m_dstarNetModeHang);

	m_dstarNetwork = new CDStarNetwork(gatewayAddress, gatewayPort, localPort, m_duplex, VERSION, debug);

	bool ret = m_dstarNetwork->open();
	if (!ret) {
		delete m_dstarNetwork;
		m_dstarNetwork = NULL;
		return false;
	}

	m_dstarNetwork->enable(true);

	return true;
}

bool CMMDVMHost::createYSFNetwork()
{
	if (!m_ysfEnabled || m_conf.getFusionNetworkEnabled())
		return true;

	std::string myAddress  = m_conf.getFusionNetworkMyAddress();
	unsigned int myPort    = m_conf.getFusionNetworkMyPort();
	std::string gatewayAddress = m_conf.getFusionNetworkGatewayAddress();
	unsigned int gatewayPort   = m_conf.getFusionNetworkGatewayPort();
	m_ysfNetModeHang       = m_conf.getFusionNetworkModeHang();
	bool debug             = m_conf.getFusionNetworkDebug();

	LogInfo("System Fusion Network Parameters");
	LogInfo("    Local Address: %s", myAddress.c_str());
	LogInfo("    Local Port: %u", myPort);
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Mode Hang: %us", m_ysfNetModeHang);

	m_ysfNetwork = new CYSFNetwork(myAddress, myPort, gatewayAddress, gatewayPort, m_callsign, debug);

	bool ret = m_ysfNetwork->open();
	if (!ret) {
		delete m_ysfNetwork;
		m_ysfNetwork = NULL;
		return false;
	}

	m_ysfNetwork->enable(true);

	return true;
}

bool CMMDVMHost::createP25Network()
{
	if (!m_p25Enabled || !m_conf.getP25NetworkEnabled())
		return true;

	std::string gatewayAddress = m_conf.getP25GatewayAddress();
	unsigned int gatewayPort   = m_conf.getP25GatewayPort();
	unsigned int localPort     = m_conf.getP25LocalPort();
	m_p25NetModeHang           = m_conf.getP25NetworkModeHang();
	bool debug                 = m_conf.getP25NetworkDebug();

	LogInfo("P25 Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Local Port: %u", localPort);
	LogInfo("    Mode Hang: %us", m_p25NetModeHang);

	m_p25Network = new CP25Network(gatewayAddress, gatewayPort, localPort, debug);

	bool ret = m_p25Network->open();
	if (!ret) {
		delete m_p25Network;
		m_p25Network = NULL;
		return false;
	}

	m_p25Network->enable(true);

	return true;
}

bool CMMDVMHost::createNXDNNetwork()
{
	if (!m_nxdnEnabled || !m_conf.getNXDNNetworkEnabled())
		return true;

	std::string gatewayAddress = m_conf.getNXDNGatewayAddress();
	unsigned int gatewayPort   = m_conf.getNXDNGatewayPort();
	std::string localAddress   = m_conf.getNXDNLocalAddress();
	unsigned int localPort     = m_conf.getNXDNLocalPort();
	m_nxdnNetModeHang          = m_conf.getNXDNNetworkModeHang();
	bool debug                 = m_conf.getNXDNNetworkDebug();

	LogInfo("NXDN Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %u", localPort);
	LogInfo("    Mode Hang: %us", m_nxdnNetModeHang);

	m_nxdnNetwork = new CNXDNNetwork(localAddress, localPort, gatewayAddress, gatewayPort, debug);

	bool ret = m_nxdnNetwork->open();
	if (!ret) {
		delete m_nxdnNetwork;
		m_nxdnNetwork = NULL;
		return false;
	}

	m_nxdnNetwork->enable(true);

	return true;
}

bool CMMDVMHost::createPOCSAGNetwork()
{
	if (!m_pocsagEnabled || !m_conf.getPOCSAGNetworkEnabled())
		return true;

	std::string gatewayAddress = m_conf.getPOCSAGGatewayAddress();
	unsigned int gatewayPort   = m_conf.getPOCSAGGatewayPort();
	std::string localAddress   = m_conf.getPOCSAGLocalAddress();
	unsigned int localPort     = m_conf.getPOCSAGLocalPort();
	m_pocsagNetModeHang        = m_conf.getPOCSAGNetworkModeHang();
	bool debug                 = m_conf.getPOCSAGNetworkDebug();

	LogInfo("POCSAG Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %u", localPort);
	LogInfo("    Mode Hang: %us", m_pocsagNetModeHang);

	m_pocsagNetwork = new CPOCSAGNetwork(localAddress, localPort, gatewayAddress, gatewayPort, debug);

	bool ret = m_pocsagNetwork->open();
	if (!ret) {
		delete m_pocsagNetwork;
		m_pocsagNetwork = NULL;
		return false;
	}

	m_pocsagNetwork->enable(true);

	return true;
}

bool CMMDVMHost::daemonize()
{
#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return false;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1){
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return false;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1){
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return false;
		}

		#if !defined(HD44780) && !defined(OLED) && !defined(_OPENWRT)
		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return false;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (::setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return false;
			}

			if (::setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return false;
			}

			// Double check it worked (AKA Paranoia)
			if (::setuid(0) != -1){
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return false;
			}
		}
		#else
		::fprintf(stderr, "Dropping root permissions in daemon mode is disabled.\n");
		#endif

		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif
	return true;
}

void CMMDVMHost::readParams()
{
	m_dstarEnabled  = m_conf.getDStarEnabled();
	m_dmrEnabled    = m_conf.getDMREnabled();
	m_ysfEnabled    = m_conf.getFusionEnabled();
	m_p25Enabled    = m_conf.getP25Enabled();
	m_nxdnEnabled   = m_conf.getNXDNEnabled();
	m_pocsagEnabled = m_conf.getPOCSAGEnabled();
	m_duplex        = m_conf.getDuplex();
	m_callsign      = m_conf.getCallsign();
	m_id            = m_conf.getId();
	m_timeout       = m_conf.getTimeout();

	LogInfo("General Parameters");
	LogInfo("    Callsign: %s", m_callsign.c_str());
	LogInfo("    Id: %u", m_id);
	LogInfo("    Duplex: %s", m_duplex ? "yes" : "no");
	LogInfo("    Timeout: %us", m_timeout);
	LogInfo("    D-Star: %s", m_dstarEnabled ? "enabled" : "disabled");
	LogInfo("    DMR: %s", m_dmrEnabled ? "enabled" : "disabled");
	LogInfo("    YSF: %s", m_ysfEnabled ? "enabled" : "disabled");
	LogInfo("    P25: %s", m_p25Enabled ? "enabled" : "disabled");
	LogInfo("    NXDN: %s", m_nxdnEnabled ? "enabled" : "disabled");
	LogInfo("    POCSAG: %s", m_pocsagEnabled ? "enabled" : "disabled");
}

void CMMDVMHost::setMode(unsigned char mode)
{
	assert(m_modem != NULL);
	assert(m_display != NULL);

	if (m_dstarNetwork != NULL)
		m_dstarNetwork->enable(false);
	if (m_dmrTask != NULL)
		m_dmrTask->enableNetwork(false);
	if (m_ysfNetwork != NULL)
		m_ysfNetwork->enable(false);
	if (m_p25Network != NULL)
		m_p25Network->enable(false);
	if (m_nxdnNetwork != NULL)
		m_nxdnNetwork->enable(false);
	if (m_pocsagNetwork != NULL)
		m_pocsagNetwork->enable(false);

	switch (mode) {
	case MODE_DSTAR:
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(true);
		m_modem->setMode(MODE_DSTAR);
		if (m_ump != NULL)
			m_ump->setMode(MODE_DSTAR);
		m_mode = MODE_DSTAR;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_DMR:
		if (m_dmrTask != NULL)
			m_dmrTask->enableNetwork(true);
		m_modem->setMode(MODE_DMR);
		if (m_ump != NULL)
			m_ump->setMode(MODE_DMR);
		if (m_duplex) {
			m_dmrTask->startDMR(true);
		}
		m_mode = MODE_DMR;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_YSF:
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(true);
		m_modem->setMode(MODE_YSF);
		if (m_ump != NULL)
			m_ump->setMode(MODE_YSF);
		m_mode = MODE_YSF;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_P25:
		if (m_p25Network != NULL)
			m_p25Network->enable(true);
		m_modem->setMode(MODE_P25);
		if (m_ump != NULL)
			m_ump->setMode(MODE_P25);
		m_mode = MODE_P25;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_NXDN:
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(true);
		m_modem->setMode(MODE_NXDN);
		if (m_ump != NULL)
			m_ump->setMode(MODE_NXDN);
		m_mode = MODE_NXDN;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_POCSAG:
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
		m_modem->setMode(MODE_POCSAG);
		if (m_ump != NULL)
			m_ump->setMode(MODE_POCSAG);
		m_mode = MODE_POCSAG;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_LOCKOUT:
		LogMessage("Mode set to Lockout");
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX() && m_dmrTask != NULL) {
			m_dmrTask->startDMR(false);
		}
		m_modem->setMode(MODE_IDLE);
		if (m_ump != NULL)
			m_ump->setMode(MODE_IDLE);
		m_display->setLockout();
		m_mode = MODE_LOCKOUT;
		m_modeTimer.stop();
		m_cwIdTimer.stop();
		break;

	case MODE_ERROR:
		LogMessage("Mode set to Error");
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX() && m_dmrTask != NULL) {
			m_dmrTask->startDMR(false);
		}
		if (m_ump != NULL)
			m_ump->setMode(MODE_IDLE);
		m_display->setError("MODEM");
		m_mode = MODE_ERROR;
		m_modeTimer.stop();
		m_cwIdTimer.stop();
		break;

	default: /* MODE_IDLE */
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(true);
		if (m_dmrTask != NULL)
			m_dmrTask->enableNetwork(true);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(true);
		if (m_p25Network != NULL)
			m_p25Network->enable(true);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(true);
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(true);
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX() && m_dmrTask != NULL) {
			m_dmrTask->startDMR(false);
		}
		m_modem->setMode(MODE_IDLE);
		if (m_ump != NULL)
			m_ump->setMode(MODE_IDLE);
		if (m_mode == MODE_ERROR) {
			m_modem->sendCWId(m_callsign);
			m_cwIdTimer.setTimeout(m_cwIdTime);
			m_cwIdTimer.start();
		} else {
			m_cwIdTimer.setTimeout(m_cwIdTime / 4U);
			m_cwIdTimer.start();
		}
		m_display->setIdle();
		m_mode = MODE_IDLE;
		m_modeTimer.stop();
		break;
	}
}

CDStarControl* CMMDVMHost::createDStarControl(CRSSIInterpolator* rssi){
	CDStarControl *dstar = NULL;
	if (m_dstarEnabled) {
		std::string module                 = m_conf.getDStarModule();
		bool selfOnly                      = m_conf.getDStarSelfOnly();
		std::vector<std::string> blackList = m_conf.getDStarBlackList();
		bool ackReply                      = m_conf.getDStarAckReply();
		unsigned int ackTime               = m_conf.getDStarAckTime();
		bool errorReply                    = m_conf.getDStarErrorReply();
		bool remoteGateway                 = m_conf.getDStarRemoteGateway();
		m_dstarRFModeHang                  = m_conf.getDStarModeHang();

		LogInfo("D-Star RF Parameters");
		LogInfo("    Module: %s", module.c_str());
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Ack Reply: %s", ackReply ? "yes" : "no");
		LogInfo("    Ack Time: %ums", ackTime);
		LogInfo("    Error Reply: %s", errorReply ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    Mode Hang: %us", m_dstarRFModeHang);

		if (blackList.size() > 0U)
			LogInfo("    Black List: %u", blackList.size());

		dstar = new CDStarControl(m_callsign, module, selfOnly, ackReply, ackTime, errorReply, blackList, m_dstarNetwork, m_display, m_timeout, m_duplex, remoteGateway, rssi);
	}
	return dstar;
}

CYSFControl* CMMDVMHost::createYSFControl(CRSSIInterpolator* rssi){
	CYSFControl *ysf = NULL;
	if (m_ysfEnabled) {
		bool lowDeviation   = m_conf.getFusionLowDeviation();
		bool remoteGateway  = m_conf.getFusionRemoteGateway();
		unsigned int txHang = m_conf.getFusionTXHang();
		bool selfOnly       = m_conf.getFusionSelfOnly();
		bool sqlEnabled     = m_conf.getFusionSQLEnabled();
		unsigned char sql   = m_conf.getFusionSQL();
		m_ysfRFModeHang     = m_conf.getFusionModeHang();

		LogInfo("YSF RF Parameters");
		LogInfo("    Low Deviation: %s", lowDeviation ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    TX Hang: %us", txHang);
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    DSQ: %s", sqlEnabled ? "yes" : "no");
		if (sqlEnabled)
			LogInfo("    DSQ Value: %u", sql);
		LogInfo("    Mode Hang: %us", m_ysfRFModeHang);

		ysf = new CYSFControl(m_callsign, selfOnly, m_ysfNetwork, m_display, m_timeout, m_duplex, lowDeviation, remoteGateway, rssi);
		ysf->setSQL(sqlEnabled, sql);
	}
	return ysf;
}

CP25Control* CMMDVMHost::createP25Control(CRSSIInterpolator* rssi){
	CP25Control *p25 = NULL;
	if (m_p25Enabled) {
		unsigned int id    = m_conf.getP25Id();
		unsigned int nac   = m_conf.getP25NAC();
		bool uidOverride   = m_conf.getP25OverrideUID();
		bool selfOnly      = m_conf.getP25SelfOnly();
		bool remoteGateway = m_conf.getP25RemoteGateway();
		m_p25RFModeHang    = m_conf.getP25ModeHang();

		LogInfo("P25 RF Parameters");
		LogInfo("    Id: %u", id);
		LogInfo("    NAC: $%03X", nac);
		LogInfo("    UID Override: %s", uidOverride ? "yes" : "no");
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    Mode Hang: %us", m_p25RFModeHang);

		p25 = new CP25Control(nac, id, selfOnly, uidOverride, m_p25Network, m_display, m_timeout, m_duplex, m_dmrLookup, remoteGateway, rssi);
	}
	return p25;
}

CNXDNControl* CMMDVMHost::createNXDNControl(CRSSIInterpolator* rssi){
	CNXDNControl *nxdn = NULL;
	if (m_nxdnEnabled) {
		std::string lookupFile  = m_conf.getNXDNIdLookupFile();
		unsigned int reloadTime = m_conf.getNXDNIdLookupTime();

		LogInfo("NXDN Id Lookups");
		LogInfo("    File: %s", lookupFile.length() > 0U ? lookupFile.c_str() : "None");
		if (reloadTime > 0U)
			LogInfo("    Reload: %u hours", reloadTime);

		m_nxdnLookup = new CNXDNLookup(lookupFile, reloadTime);
		m_nxdnLookup->read();

		unsigned int id    = m_conf.getNXDNId();
		unsigned int ran   = m_conf.getNXDNRAN();
		bool selfOnly      = m_conf.getNXDNSelfOnly();
		bool remoteGateway = m_conf.getNXDNRemoteGateway();
		m_nxdnRFModeHang   = m_conf.getNXDNModeHang();

		LogInfo("NXDN RF Parameters");
		LogInfo("    Id: %u", id);
		LogInfo("    RAN: %u", ran);
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    Mode Hang: %us", m_nxdnRFModeHang);

		nxdn = new CNXDNControl(ran, id, selfOnly, m_nxdnNetwork, m_display, m_timeout, m_duplex, remoteGateway, m_nxdnLookup, rssi);
	}
	return nxdn;
}

CPOCSAGControl* CMMDVMHost::createPOCSAGControl(CTimer& pocsagTimer){
	CPOCSAGControl *pocsag = NULL;
	if (m_pocsagEnabled) {
		unsigned int frequency = m_conf.getPOCSAGFrequency();

		LogInfo("POCSAG RF Parameters");
		LogInfo("    Frequency: %uHz", frequency);

		pocsag = new CPOCSAGControl(m_pocsagNetwork, m_display);
		pocsagTimer.start();
	}
	return pocsag;
}