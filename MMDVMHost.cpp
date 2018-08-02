/*
 *   Copyright (C) 2015,2016,2017,2018 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by BG5HHP
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
#include "Version.h"
#include "StopWatch.h"
#include "Defines.h"
#include "UDPSocket.h"
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
		m_killed = false;

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
m_dstarTask(NULL),
m_dmrTask(NULL),
m_ysfTask(NULL),
m_p25Task(NULL),
m_nxdnTask(NULL),
m_pocsagTask(NULL),
m_display(NULL),
m_ump(NULL),
m_mode(MODE_IDLE),
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

	// Map IDs to callsigns for NXDN - should be merged with dmrLookup.
	if (m_nxdnEnabled) {
		std::string lookupFile  = m_conf.getNXDNIdLookupFile();
    	unsigned int reloadTime = m_conf.getNXDNIdLookupTime();
		LogInfo("NXDN Id Lookups");
		LogInfo("    File: %s", lookupFile.length() > 0U ? lookupFile.c_str() : "None");
		if (reloadTime > 0U)
			LogInfo("    Reload: %u hours", reloadTime);

		m_nxdnLookup = new CNXDNLookup(lookupFile, reloadTime);
		m_nxdnLookup->read();
	}

	CStopWatch stopWatch;
	stopWatch.start();

	if (m_dstarEnabled){
		m_dstarTask = CDStarTask::create(this,rssi);
		assert(m_dstarTask);
	}
	if (m_dmrEnabled){
		m_dmrTask = CDMRTask::create(this,rssi);
		assert(m_dmrTask);
	}
	if (m_ysfEnabled){
		m_ysfTask = CYSFTask::create(this,rssi);
		assert(m_ysfTask);
	}
	if (m_p25Enabled){
		m_p25Task = CP25Task::create(this,rssi);
		assert(m_p25Task);
	}
	if (m_nxdnEnabled){
		m_nxdnTask = CNXDNTask::create(this,rssi);
		assert(m_nxdnTask);
	}
	if (m_pocsagEnabled){
		m_pocsagTask = CPOCSAGTask::create(this,rssi);
		assert(m_pocsagTask);
	}
	
	CPassThroughTask *transparentTask = NULL;
	if (m_conf.getTransparentEnabled())
		transparentTask = CPassThroughTask::create(this);

	setMode(MODE_IDLE);

	LogMessage("MMDVMHost-%s is running", VERSION);

	CMMDVMTaskContext ctx;
	ctx.host = this;

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
		ctx.data = data;

		if (m_dstarTask != NULL)
			m_dstarTask->run(&ctx);

		if (m_dmrTask != NULL)
			m_dmrTask->run(&ctx);
		
		if (m_ysfTask != NULL)
			m_ysfTask->run(&ctx);

		if (m_p25Task != NULL)
			m_p25Task->run(&ctx);

		if (m_nxdnTask != NULL)
			m_nxdnTask->run(&ctx);

		if (transparentTask != NULL)
			transparentTask->run(&ctx);

		if (m_modeTimer.isRunning() && m_modeTimer.hasExpired())
			setMode(MODE_IDLE);

		if (m_pocsagTask != NULL)
			m_pocsagTask->run(&ctx);

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_display->clock(ms);

		m_modem->clock(ms);
		m_modeTimer.clock(ms);

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

	if (m_dstarTask != NULL)
		delete m_dstarTask;

	if (m_dmrTask != NULL)
		delete m_dmrTask;

	if (m_ysfTask != NULL)
		delete m_ysfTask;

	if (m_p25Task != NULL)
		delete m_p25Task;

	if (m_nxdnTask != NULL)
		delete m_nxdnTask;

	if (m_pocsagTask != NULL)
		delete m_pocsagTask;

	if (transparentTask != NULL)
		delete transparentTask;

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

	if (m_dstarTask != NULL)
		m_dstarTask->enableNetwork(false);
	if (m_dmrTask != NULL)
		m_dmrTask->enableNetwork(false);
	if (m_ysfTask != NULL)
		m_ysfTask->enableNetwork(false);
	if (m_p25Task != NULL)
		m_p25Task->enableNetwork(false);
	if (m_nxdnTask != NULL)
		m_nxdnTask->enableNetwork(false);
	if (m_pocsagTask != NULL)
		m_pocsagTask->enableNetwork(false);

	switch (mode) {
	case MODE_DSTAR:
		if (m_dstarTask != NULL)
			m_dstarTask->enableNetwork(true);
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
		if (m_ysfTask != NULL)
			m_ysfTask->enableNetwork(true);
		m_modem->setMode(MODE_YSF);
		if (m_ump != NULL)
			m_ump->setMode(MODE_YSF);
		m_mode = MODE_YSF;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_P25:
		if (m_p25Task)
			m_p25Task->enableNetwork(true);
		m_modem->setMode(MODE_P25);
		if (m_ump != NULL)
			m_ump->setMode(MODE_P25);
		m_mode = MODE_P25;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_NXDN:
		if (m_nxdnTask != NULL)
			m_nxdnTask->enableNetwork(true);
		m_modem->setMode(MODE_NXDN);
		if (m_ump != NULL)
			m_ump->setMode(MODE_NXDN);
		m_mode = MODE_NXDN;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		break;

	case MODE_POCSAG:
		if (m_pocsagTask != NULL)
			m_pocsagTask->enableNetwork(false);
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
		if (m_dstarTask != NULL)
			m_dstarTask->enableNetwork(true);
		if (m_dmrTask != NULL)
			m_dmrTask->enableNetwork(true);
		if (m_ysfTask != NULL)
			m_ysfTask->enableNetwork(true);
		if (m_p25Task != NULL)
			m_p25Task->enableNetwork(true);
		if (m_nxdnTask != NULL)
			m_nxdnTask->enableNetwork(true);
		if (m_pocsagTask != NULL)
			m_pocsagTask->enableNetwork(true);
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