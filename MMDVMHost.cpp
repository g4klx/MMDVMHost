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
#include "DStarControl.h"
#include "DMRControl.h"
#include "TFTSerial.h"
#include "NullDisplay.h"
#include "YSFControl.h"
#include "Nextion.h"
#include "Thread.h"

#if defined(HD44780)
#include "HD44780.h"
#endif

#if defined(OLED)
#include "OLED.h"
#endif

#include <cstdio>
#include <vector>

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
const char* HEADER4 = "Copyright(C) 2015, 2016 by Jonathan Naylor, G4KLX and others";

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "MMDVMHost version %s\n", VERSION);
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
  ::signal(SIGTERM, sigHandler);
  ::signal(SIGHUP,  sigHandler);
#endif

  int ret = 0;

  do {
	  m_signal = 0;

	  CMMDVMHost* host = new CMMDVMHost(std::string(iniFile));
	  ret = host->run();

	  delete host;

	  if (m_signal == 15)
		  ::LogInfo("Caught SIGTERM, exiting");

	  if (m_signal == 1)
		  ::LogInfo("Caught SIGHUP, restarting");
  } while (m_signal == 1);

  ::LogFinalise();

  return ret;
}

CMMDVMHost::CMMDVMHost(const std::string& confFile) :
m_conf(confFile),
m_modem(NULL),
m_dstarNetwork(NULL),
m_dmrNetwork(NULL),
m_ysfNetwork(NULL),
m_display(NULL),
m_mode(MODE_IDLE),
m_rfModeHang(10U),
m_netModeHang(3U),
m_modeTimer(1000U),
m_dmrTXTimer(1000U),
m_cwIdTimer(1000U),
m_duplex(false),
m_timeout(180U),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_ysfEnabled(false),
m_callsign()
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

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), m_conf.getLogDisplayLevel());
	if (!ret) {
		::fprintf(stderr, "MMDVMHost: unable to open the log file\n");
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			    ::LogWarning("Couldn't fork() , exiting");
			    return -1;
		    }
		else if (pid != 0)
			exit(EXIT_SUCCESS);

		// Create new session and process group
		if (::setsid() == -1){
			    ::LogWarning("Couldn't setsid(), exiting");
			    return -1;
		    }

		// Set the working directory to the root directory
		if (::chdir("/") == -1){
			    ::LogWarning("Couldn't cd /, exiting");
			    return -1;
		    }

		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
#if !defined(HD44780)
		//If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::LogError("Could not get the mmdvm user, exiting");
				return -1;
			}
			
			uid_t mmdvm_uid = user->pw_uid;
		    gid_t mmdvm_gid = user->pw_gid;

		    //Set user and group ID's to mmdvm:mmdvm
		    if (setgid(mmdvm_gid) != 0) {
			    ::LogWarning("Could not set mmdvm GID, exiting");
			    return -1;
		    }

			if (setuid(mmdvm_uid) != 0) {
			    ::LogWarning("Could not set mmdvm UID, exiting");
			    return -1;
		    }
		    
		    //Double check it worked (AKA Paranoia) 
		    if (setuid(0) != -1){
			    ::LogWarning("It's possible to regain root - something is wrong!, exiting");
			    return -1;
		    }
		
		}
	}
#else
	::LogWarning("Dropping root permissions in daemon mode is disabled with HD44780 display");
	}
#endif
#endif

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

	if (m_ysfEnabled && m_conf.getFusionNetworkEnabled()) {
		ret = createYSFNetwork();
		if (!ret)
			return 1;
	}

	if (m_conf.getCWIdEnabled()) {
		unsigned int time = m_conf.getCWIdTime();

		LogInfo("CW Id Parameters");
		LogInfo("    Time: %u mins", time);

		m_cwIdTimer.setTimeout(time * 60U);
		m_cwIdTimer.start();
	}

	CTimer dmrBeaconTimer(1000U, 4U);
	bool dmrBeaconsEnabled = m_dmrEnabled && m_conf.getDMRBeacons();

	CStopWatch stopWatch;
	stopWatch.start();

	CDStarControl* dstar = NULL;
	if (m_dstarEnabled) {
		std::string module = m_conf.getDStarModule();
		bool selfOnly      = m_conf.getDStarSelfOnly();
		std::vector<std::string> blackList = m_conf.getDStarBlackList();

		LogInfo("D-Star Parameters");
		LogInfo("    Module: %s", module.c_str());
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		if (blackList.size() > 0U)
			LogInfo("    Black List: %u", blackList.size());

		dstar = new CDStarControl(m_callsign, module, selfOnly, blackList, m_dstarNetwork, m_display, m_timeout, m_duplex);
	}

	CDMRControl* dmr = NULL;
	if (m_dmrEnabled) {
		unsigned int id        = m_conf.getDMRId();
		unsigned int colorCode = m_conf.getDMRColorCode();
		bool selfOnly          = m_conf.getDMRSelfOnly();
		std::vector<unsigned int> prefixes = m_conf.getDMRPrefixes();
		std::vector<unsigned int> blackList = m_conf.getDMRBlackList();
		std::vector<unsigned int> dstIDBlackListSlot1 = m_conf.getDMRDstIdBlacklistSlot1();
		std::vector<unsigned int> dstIDBlackListSlot2 = m_conf.getDMRDstIdBlacklistSlot2();
		std::vector<unsigned int> dstIDWhiteListSlot1 = m_conf.getDMRDstIdWhitelistSlot1();
		std::vector<unsigned int> dstIDWhiteListSlot2 = m_conf.getDMRDstIdWhitelistSlot2();
		std::string lookupFile = m_conf.getDMRLookupFile();
		unsigned int callHang  = m_conf.getDMRCallHang();
		unsigned int txHang    = m_conf.getDMRTXHang();

		if (txHang > m_rfModeHang)
			txHang = m_rfModeHang;
		if (txHang > m_netModeHang)
			txHang = m_netModeHang;

		if (callHang > txHang)
			callHang = txHang;

		LogInfo("DMR Parameters");
		LogInfo("    Id: %u", id);
		LogInfo("    Color Code: %u", colorCode);
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Prefixes: %u", prefixes.size());
		
		if (blackList.size() > 0U)
			LogInfo("    Black List: %u", blackList.size());
		if (dstIDBlackListSlot1.size() > 0U)
			LogInfo("    Slot 1 Destination ID Black List: %u entries", dstIDBlackListSlot1.size());
		if (dstIDBlackListSlot2.size() > 0U)
			LogInfo("    Slot 2 Destination ID Black List: %u entries", dstIDBlackListSlot2.size());
		if (dstIDWhiteListSlot1.size() > 0U)
			LogInfo("    Slot 1 Destination ID White List: %u entries", dstIDWhiteListSlot1.size());
		if (dstIDWhiteListSlot2.size() > 0U)
			LogInfo("    Slot 2 Destination ID White List: %u entries", dstIDWhiteListSlot2.size());
		
		LogInfo("    Lookup File: %s", lookupFile.length() > 0U ? lookupFile.c_str() : "None");
		LogInfo("    Call Hang: %us", callHang);
		LogInfo("    TX Hang: %us", txHang);

		dmr = new CDMRControl(id, colorCode, callHang, selfOnly, prefixes, blackList,dstIDBlackListSlot1,dstIDWhiteListSlot1, dstIDBlackListSlot2, dstIDWhiteListSlot2, m_timeout, m_modem, m_dmrNetwork, m_display, m_duplex, lookupFile);

		m_dmrTXTimer.setTimeout(txHang);
	}

	CYSFControl* ysf = NULL;
	if (m_ysfEnabled)
		ysf = new CYSFControl(m_callsign, m_ysfNetwork, m_display, m_timeout, m_duplex);

	setMode(MODE_IDLE);

	LogMessage("MMDVMHost-%s is running", VERSION);

	while (!m_killed) {
		bool lockout = m_modem->hasLockout();
		if (lockout && m_mode != MODE_LOCKOUT)
			setMode(MODE_LOCKOUT);
		else if (!lockout && m_mode == MODE_LOCKOUT)
			setMode(MODE_IDLE);

		bool error = m_modem->hasError();
		if (error && m_mode != MODE_ERROR)
			setMode(MODE_ERROR);
		else if (!error && m_mode == MODE_ERROR)
			setMode(MODE_IDLE);

		unsigned char data[200U];
		unsigned int len;
		bool ret;

		len = m_modem->readDStarData(data);
		if (dstar != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = dstar->writeModem(data);
				if (ret) {
					m_modeTimer.setTimeout(m_rfModeHang);
					setMode(MODE_DSTAR);
				}
			} else if (m_mode == MODE_DSTAR) {
				dstar->writeModem(data);
				m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("D-Star modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readDMRData1(data);
		if (dmr != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				if (m_duplex) {
					bool ret = dmr->processWakeup(data);
					if (ret) {
						m_modeTimer.setTimeout(m_rfModeHang);
						setMode(MODE_DMR);
						dmrBeaconTimer.stop();
					}
				} else {
					m_modeTimer.setTimeout(m_rfModeHang);
					setMode(MODE_DMR);
					dmr->writeModemSlot1(data);
					dmrBeaconTimer.stop();
				}
			} else if (m_mode == MODE_DMR) {
				if (m_duplex && !m_modem->hasTX()) {
					bool ret = dmr->processWakeup(data);
					if (ret) {
						m_modem->writeDMRStart(true);
						m_dmrTXTimer.start();
					}
				} else {
					dmr->writeModemSlot1(data);
					dmrBeaconTimer.stop();
					m_modeTimer.start();
					if (m_duplex)
						m_dmrTXTimer.start();
				}
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("DMR modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readDMRData2(data);
		if (dmr != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				if (m_duplex) {
					bool ret = dmr->processWakeup(data);
					if (ret) {
						m_modeTimer.setTimeout(m_rfModeHang);
						setMode(MODE_DMR);
						dmrBeaconTimer.stop();
					}
				} else {
					m_modeTimer.setTimeout(m_rfModeHang);
					setMode(MODE_DMR);
					dmr->writeModemSlot2(data);
					dmrBeaconTimer.stop();
				}
			} else if (m_mode == MODE_DMR) {
				if (m_duplex && !m_modem->hasTX()) {
					bool ret = dmr->processWakeup(data);
					if (ret) {
						m_modem->writeDMRStart(true);
						m_dmrTXTimer.start();
					}
				} else {
					dmr->writeModemSlot2(data);
					dmrBeaconTimer.stop();
					m_modeTimer.start();
					if (m_duplex)
						m_dmrTXTimer.start();
				}
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("DMR modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readYSFData(data);
		if (ysf != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = ysf->writeModem(data);
				if (ret) {
					m_modeTimer.setTimeout(m_rfModeHang);
					setMode(MODE_YSF);
				}
			} else if (m_mode == MODE_YSF) {
				ysf->writeModem(data);
				m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("System Fusion modem data received when in mode %u", m_mode);
			}
		}

		if (m_modeTimer.isRunning() && m_modeTimer.hasExpired())
			setMode(MODE_IDLE);

		if (dstar != NULL) {
			ret = m_modem->hasDStarSpace();
			if (ret) {
				len = dstar->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_netModeHang);
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

		if (dmr != NULL) {
			ret = m_modem->hasDMRSpace1();
			if (ret) {
				len = dmr->readModemSlot1(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_netModeHang);
						setMode(MODE_DMR);
					}
					if (m_mode == MODE_DMR) {
						if (m_duplex) {
							m_modem->writeDMRStart(true);
							m_dmrTXTimer.start();
						}
						m_modem->writeDMRData1(data, len);
						dmrBeaconTimer.stop();
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("DMR data received when in mode %u", m_mode);
					}
				}
			}

			ret = m_modem->hasDMRSpace2();
			if (ret) {
				len = dmr->readModemSlot2(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_netModeHang);
						setMode(MODE_DMR);
					}
					if (m_mode == MODE_DMR) {
						if (m_duplex) {
							m_modem->writeDMRStart(true);
							m_dmrTXTimer.start();
						}
						m_modem->writeDMRData2(data, len);
						dmrBeaconTimer.stop();
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("DMR data received when in mode %u", m_mode);
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
						m_modeTimer.setTimeout(m_netModeHang);
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

		if (m_dmrNetwork != NULL) {
			bool run = m_dmrNetwork->wantsBeacon();
			if (dmrBeaconsEnabled && run && m_mode == MODE_IDLE && !m_modem->hasTX()) {
				setMode(MODE_DMR);
				dmrBeaconTimer.start();
			}
		}

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_display->clock(ms);

		m_modem->clock(ms);
		m_modeTimer.clock(ms);

		if (dstar != NULL)
			dstar->clock();
		if (dmr != NULL)
			dmr->clock();
		if (ysf != NULL)
			ysf->clock(ms);

		if (m_dstarNetwork != NULL)
			m_dstarNetwork->clock(ms);
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->clock(ms);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->clock(ms);

		m_cwIdTimer.clock(ms);
		if (m_cwIdTimer.isRunning() && m_cwIdTimer.hasExpired()) {
			if (m_mode == MODE_IDLE && !m_modem->hasTX()){
				LogDebug("sending CW ID");
				m_modem->sendCWId(m_callsign);

				m_cwIdTimer.start();   //reset only after sending ID, timer-overflow after 49 days doesnt matter
			}
		}

		dmrBeaconTimer.clock(ms);
		if (dmrBeaconTimer.isRunning() && dmrBeaconTimer.hasExpired()) {
			setMode(MODE_IDLE);
			dmrBeaconTimer.stop();
		}

		m_dmrTXTimer.clock(ms);
		if (m_dmrTXTimer.isRunning() && m_dmrTXTimer.hasExpired()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	LogMessage("MMDVMHost-%s is exiting on receipt of SIGHUP1", VERSION);

	setMode(MODE_IDLE);

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

	if (m_ysfNetwork != NULL) {
		m_ysfNetwork->close();
		delete m_ysfNetwork;
	}

	delete dstar;
	delete dmr;
	delete ysf;

	return 0;
}

bool CMMDVMHost::createModem()
{
    std::string port         = m_conf.getModemPort();
    bool rxInvert            = m_conf.getModemRXInvert();
    bool txInvert            = m_conf.getModemTXInvert();
    bool pttInvert           = m_conf.getModemPTTInvert();
    unsigned int txDelay     = m_conf.getModemTXDelay();
	unsigned int dmrDelay    = m_conf.getModemDMRDelay();
	unsigned int rxLevel     = m_conf.getModemRXLevel();
    unsigned int txLevel     = m_conf.getModemTXLevel();
    bool debug               = m_conf.getModemDebug();
	unsigned int colorCode   = m_conf.getDMRColorCode();
	unsigned int rxFrequency = m_conf.getRxFrequency();
	unsigned int txFrequency = m_conf.getTxFrequency();
	int dstarLevel           = m_conf.getModemDStarLevel();
	int dmrLevel1            = m_conf.getModemDMRLevel1();
	int dmrLevel3            = m_conf.getModemDMRLevel3();
	int ysfLevel1            = m_conf.getModemYSFLevel1();
	int ysfLevel3            = m_conf.getModemYSFLevel3();
	int dmrThreshold         = m_conf.getModemDMRThreshold();
	int ysfThreshold         = m_conf.getModemYSFThreshold();
	int oscOffset            = m_conf.getModemOscOffset();

	LogInfo("Modem Parameters");
	LogInfo("    Port: %s", port.c_str());
	LogInfo("    RX Invert: %s", rxInvert ? "yes" : "no");
	LogInfo("    TX Invert: %s", txInvert ? "yes" : "no");
	LogInfo("    PTT Invert: %s", pttInvert ? "yes" : "no");
	LogInfo("    TX Delay: %ums", txDelay);
	LogInfo("    DMR Delay: %u (%.1fms)", dmrDelay, float(dmrDelay) * 0.0416666F);
	LogInfo("    RX Level: %u%%", rxLevel);
	LogInfo("    TX Level: %u%%", txLevel);
	LogInfo("    RX Frequency: %uHz", rxFrequency);
	LogInfo("    TX Frequency: %uHz", txFrequency);

	if (dstarLevel != 0)
		LogInfo("    D-Star Level: %.1f%%", float(dstarLevel) / 10.0F);
	if (dmrLevel1 != 0)
		LogInfo("    DMR Level 1: %.1f%%", float(dmrLevel1) / 10.0F);
	if (dmrLevel3 != 0)
		LogInfo("    DMR Level 3: %.1f%%", float(dmrLevel3) / 10.0F);
	if (ysfLevel1 != 0)
		LogInfo("    YSF Level 1: %.1f%%", float(ysfLevel1) / 10.0F);
	if (ysfLevel3 != 0)
		LogInfo("    YSF Level 3: %.1f%%", float(ysfLevel3) / 10.0F);
	if (dmrThreshold != 0)
		LogInfo("    DMR Threshold: %.1f%%", float(dmrThreshold) / 10.0F);
	if (ysfThreshold != 0)
		LogInfo("    YSF Threshold: %.1f%%", float(ysfThreshold) / 10.0F);

	LogInfo("    Osc. Offset: %dppm", oscOffset);

	m_modem = new CModem(port, m_duplex, rxInvert, txInvert, pttInvert, txDelay, rxLevel, txLevel, dmrDelay, oscOffset, debug);
	m_modem->setModeParams(m_dstarEnabled, m_dmrEnabled, m_ysfEnabled);
	m_modem->setModeLevels(dstarLevel, dmrLevel1, dmrLevel3, ysfLevel1, ysfLevel3, dmrThreshold, ysfThreshold);
	m_modem->setRFParams(rxFrequency, txFrequency);
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

bool CMMDVMHost::createDMRNetwork()
{
	std::string address  = m_conf.getDMRNetworkAddress();
	unsigned int port    = m_conf.getDMRNetworkPort();
	unsigned int local   = m_conf.getDMRNetworkLocal();
	unsigned int id      = m_conf.getDMRId();
	std::string password = m_conf.getDMRNetworkPassword();
	bool debug           = m_conf.getDMRNetworkDebug();
	bool slot1           = m_conf.getDMRNetworkSlot1();
	bool slot2           = m_conf.getDMRNetworkSlot2();

	LogInfo("DMR Network Parameters");
	LogInfo("    Address: %s", address.c_str());
	LogInfo("    Port: %u", port);
	if (local > 0U)
		LogInfo("    Local: %u", local);
	else
		LogInfo("    Local: random");
	LogInfo("    Slot 1: %s", slot1 ? "enabled" : "disabled");
	LogInfo("    Slot 2: %s", slot2 ? "enabled" : "disabled");

	m_dmrNetwork = new CDMRIPSC(address, port, local, id, password, m_duplex, VERSION, debug, slot1, slot2);

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
	LogInfo("    Callsign: %s", m_callsign.c_str());
	LogInfo("    RX Frequency: %uHz", rxFrequency);
	LogInfo("    TX Frequency: %uHz", txFrequency);
	LogInfo("    Power: %uW", power);
	LogInfo("    Latitude: %fdeg N", latitude);
	LogInfo("    Longitude: %fdeg E", longitude);
	LogInfo("    Height: %um", height);
	LogInfo("    Location: \"%s\"", location.c_str());
	LogInfo("    Description: \"%s\"", description.c_str());
	LogInfo("    URL: \"%s\"", url.c_str());

	m_dmrNetwork->setConfig(m_callsign, rxFrequency, txFrequency, power, colorCode, latitude, longitude, height, location, description, url);

	bool ret = m_dmrNetwork->open();
	if (!ret) {
		delete m_dmrNetwork;
		m_dmrNetwork = NULL;
		return false;
	}

	m_dmrNetwork->enable(true);

	return true;
}

bool CMMDVMHost::createYSFNetwork()
{
	std::string myAddress  = m_conf.getFusionNetworkMyAddress();
	unsigned int myPort    = m_conf.getFusionNetworkMyPort();
	std::string gwyAddress = m_conf.getFusionNetworkGwyAddress();
	unsigned int gwyPort   = m_conf.getFusionNetworkGwyPort();
	bool debug             = m_conf.getFusionNetworkDebug();

	LogInfo("System Fusion Network Parameters");
	LogInfo("    Local Address: %s", myAddress.c_str());
	LogInfo("    Local Port: %u", myPort);
	LogInfo("    Gateway Address: %s", gwyAddress.c_str());
	LogInfo("    Gateway Port: %u", gwyPort);

	m_ysfNetwork = new CYSFNetwork(myAddress, myPort, gwyAddress, gwyPort, m_callsign, debug);

	bool ret = m_ysfNetwork->open();
	if (!ret) {
		delete m_ysfNetwork;
		m_ysfNetwork = NULL;
		return false;
	}

	m_ysfNetwork->enable(true);

	return true;
}

void CMMDVMHost::readParams()
{
	m_dstarEnabled = m_conf.getDStarEnabled();
	m_dmrEnabled   = m_conf.getDMREnabled();
	m_ysfEnabled   = m_conf.getFusionEnabled();
	m_duplex       = m_conf.getDuplex();
	m_callsign     = m_conf.getCallsign();
	m_timeout      = m_conf.getTimeout();

	m_rfModeHang  = m_conf.getRFModeHang();
	m_netModeHang = m_conf.getNetModeHang();

	LogInfo("General Parameters");
	LogInfo("    Callsign: %s", m_callsign.c_str());
	LogInfo("    Duplex: %s", m_duplex ? "yes" : "no");
	LogInfo("    Timeout: %us", m_timeout);
	LogInfo("    RF Mode Hang: %us", m_rfModeHang);
	LogInfo("    Net Mode Hang: %us", m_netModeHang);
	LogInfo("    D-Star: %s", m_dstarEnabled ? "enabled" : "disabled");
	LogInfo("    DMR: %s", m_dmrEnabled ? "enabled" : "disabled");
	LogInfo("    YSF: %s", m_ysfEnabled ? "enabled" : "disabled");
}

void CMMDVMHost::createDisplay()
{
	std::string type = m_conf.getDisplay();
	unsigned int dmrid = m_conf.getDMRId();

	LogInfo("Display Parameters");
	LogInfo("    Type: %s", type.c_str());

	if (type == "TFT Serial") {
		std::string port        = m_conf.getTFTSerialPort();
		unsigned int brightness = m_conf.getTFTSerialBrightness();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);

		m_display = new CTFTSerial(m_callsign, dmrid, port, brightness);
	} else if (type == "Nextion") {
		std::string port            = m_conf.getNextionPort();
		unsigned int brightness     = m_conf.getNextionBrightness();
		bool displayClock           = m_conf.getNextionDisplayClock();
		bool utc                    = m_conf.getNextionUTC();
		unsigned int idleBrightness = m_conf.getNextionIdleBrightness();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);
		LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");
		if (displayClock)
			LogInfo("    Display UTC: %s", utc ? "yes" : "no");
		LogInfo("    Idle Brightness: %u", idleBrightness);

		m_display = new CNextion(m_callsign, dmrid, port, brightness, displayClock, utc, idleBrightness);
#if defined(HD44780)
	} else if (type == "HD44780") {
		unsigned int rows              = m_conf.getHD44780Rows();
		unsigned int columns           = m_conf.getHD44780Columns();
		std::vector<unsigned int> pins = m_conf.getHD44780Pins();
		std::string i2cAddress         = m_conf.getHD44780i2cAddress();
		bool pwm                       = m_conf.getHD44780PWM();
		unsigned int pwmPin            = m_conf.getHD44780PWMPin();
		unsigned int pwmBright         = m_conf.getHD44780PWMBright();
		unsigned int pwmDim            = m_conf.getHD44780PWMDim();
		bool displayClock              = m_conf.getHD44780DisplayClock();
		bool utc                       = m_conf.getHD44780UTC();

		if (pins.size() == 6U) {
			LogInfo("    Rows: %u", rows);
			LogInfo("    Columns: %u", columns);
			LogInfo("    Pins: %u,%u,%u,%u,%u,%u", pins.at(0U), pins.at(1U), pins.at(2U), pins.at(3U), pins.at(4U), pins.at(5U));

			LogInfo("    PWM Backlight: %s", pwm ? "yes" : "no");
			if (pwm) {
				LogInfo("    PWM Pin: %u", pwmPin);
				LogInfo("    PWM Bright: %u", pwmBright);
				LogInfo("    PWM Dim: %u", pwmDim);
			}

			LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");
			if (displayClock)
				LogInfo("    Display UTC: %s", utc ? "yes" : "no");

			m_display = new CHD44780(rows, columns, m_callsign, dmrid, pins, i2cAddress, pwm, pwmPin, pwmBright, pwmDim, displayClock, utc, m_duplex);
		}
#endif
#if defined(OLED)
	} else if (type == "OLED") {
        unsigned char displayType       = m_conf.getOLEDType();
        unsigned char displayBrightness = m_conf.getOLEDBrightness();
        unsigned char displayInvert     = m_conf.getOLEDInvert();
		m_display = new COLED(displayType, displayBrightness, displayInvert);
#endif
	} else {
		m_display = new CNullDisplay;
	}

	bool ret = m_display->open();
	if (!ret) {
		delete m_display;
		m_display = new CNullDisplay;
	}
}

void CMMDVMHost::setMode(unsigned char mode)
{
	assert(m_modem != NULL);
	assert(m_display != NULL);

	switch (mode) {
	case MODE_DSTAR:
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		m_modem->setMode(MODE_DSTAR);
		m_mode = MODE_DSTAR;
		m_modeTimer.start();
		break;

	case MODE_DMR:
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		m_modem->setMode(MODE_DMR);
		if (m_duplex) {
			m_modem->writeDMRStart(true);
			m_dmrTXTimer.start();
		}
		m_mode = MODE_DMR;
		m_modeTimer.start();
		break;

	case MODE_YSF:
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		m_modem->setMode(MODE_YSF);
		m_mode = MODE_YSF;
		m_modeTimer.start();
		break;

	case MODE_LOCKOUT:
		LogMessage("Mode set to Lockout");
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}
		m_modem->setMode(MODE_IDLE);
		m_display->setLockout();
		m_mode = MODE_LOCKOUT;
		m_modeTimer.stop();
		m_cwIdTimer.stop();
		break;

	case MODE_ERROR:
		LogMessage("Mode set to Error");
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}
		m_display->setError("MODEM");
		m_mode = MODE_ERROR;
		m_modeTimer.stop();
		m_cwIdTimer.stop();
		break;

	default:
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(true);
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(true);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(true);
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}
		m_modem->setMode(MODE_IDLE);
		if (m_mode == MODE_ERROR || m_mode == MODE_LOCKOUT) {
			m_modem->sendCWId(m_callsign);
			m_cwIdTimer.start();
		}
		m_display->setIdle();
		m_mode = MODE_IDLE;
		m_modeTimer.stop();
		break;
	}
}
