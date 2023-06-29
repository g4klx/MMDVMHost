/*
 *   Copyright (C) 2015-2023 by Jonathan Naylor G4KLX
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
#include "NXDNKenwoodNetwork.h"
#include "NXDNIcomNetwork.h"
#include "RSSIInterpolator.h"
#include "NullController.h"
#include "UARTController.h"
#if defined(__linux__)
#include "I2CController.h"
#endif
#include "UDPController.h"
#include "MQTTConnection.h"
#include "DStarDefines.h"
#include "Version.h"
#include "StopWatch.h"
#include "Thread.h"
#include "Utils.h"
#include "Log.h"
#include "GitVersion.h"

#include <cstdio>
#include <vector>

#include <cstdlib>

#include <nlohmann/json.hpp>

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
static bool m_reload = false;

// In Log.cpp
extern CMQTTConnection* m_mqtt;

#if !defined(_WIN32) && !defined(_WIN64)
static void sigHandler1(int signum)
{
	m_killed = true;
	m_signal = signum;
}

static void sigHandler2(int signum)
{
	m_reload = true;
}
#endif

static CMMDVMHost* host = NULL;

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2015-2023 by Jonathan Naylor, G4KLX and others";

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
	::signal(SIGINT,  sigHandler1);
	::signal(SIGTERM, sigHandler1);
	::signal(SIGHUP,  sigHandler1);
	::signal(SIGUSR1, sigHandler2);
#endif

	int ret = 0;

	do {
		m_signal = 0;

		host = new CMMDVMHost(std::string(iniFile));
		ret = host->run();

		delete host;
		host = NULL;

		switch (m_signal) {
			case 2:
				::LogInfo("MMDVMHost-%s exited on receipt of SIGINT", VERSION);
				break;
			case 15:
				::LogInfo("MMDVMHost-%s exited on receipt of SIGTERM", VERSION);
				break;
			case 1:
				::LogInfo("MMDVMHost-%s exited on receipt of SIGHUP", VERSION);
				break;
			case 10:
				::LogInfo("MMDVMHost-%s is restarting on receipt of SIGUSR1", VERSION);
				break;
			default:
				::LogInfo("MMDVMHost-%s exited on receipt of an unknown signal", VERSION);
				break;
		}
	} while (m_signal == 10);

	::LogFinalise();

	return ret;
}

CMMDVMHost::CMMDVMHost(const std::string& confFile) :
m_conf(confFile),
m_modem(NULL),
#if defined(USE_DSTAR)
m_dstar(NULL),
#endif
m_dmr(NULL),
m_ysf(NULL),
m_p25(NULL),
m_nxdn(NULL),
m_m17(NULL),
#if defined(USE_POCSAG)
m_pocsag(NULL),
#endif
#if defined(USE_FM)
m_fm(NULL),
#endif
#if defined(USE_AX25)
m_ax25(NULL),
#endif
#if defined(USE_DSTAR)
m_dstarNetwork(NULL),
#endif
m_dmrNetwork(NULL),
m_ysfNetwork(NULL),
m_p25Network(NULL),
m_nxdnNetwork(NULL),
m_m17Network(NULL),
#if defined(USE_POCSAG)
m_pocsagNetwork(NULL),
#endif
#if defined(USE_FM)
m_fmNetwork(NULL),
#endif
#if defined(USE_AX25)
m_ax25Network(NULL),
#endif
m_mode(MODE_IDLE),
#if defined(USE_DSTAR)
m_dstarRFModeHang(10U),
#endif
m_dmrRFModeHang(10U),
m_ysfRFModeHang(10U),
m_p25RFModeHang(10U),
m_nxdnRFModeHang(10U),
m_m17RFModeHang(10U),
#if defined(USE_FM)
m_fmRFModeHang(10U),
#endif
#if defined(USE_DSTAR)
m_dstarNetModeHang(3U),
#endif
m_dmrNetModeHang(3U),
m_ysfNetModeHang(3U),
m_p25NetModeHang(3U),
m_nxdnNetModeHang(3U),
m_m17NetModeHang(3U),
#if defined(USE_POCSAG)
m_pocsagNetModeHang(3U),
#endif
#if defined(USE_FM)
m_fmNetModeHang(3U),
#endif
m_modeTimer(1000U),
m_dmrTXTimer(1000U),
m_cwIdTimer(1000U),
m_duplex(false),
m_timeout(180U),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_ysfEnabled(false),
m_p25Enabled(false),
m_nxdnEnabled(false),
m_m17Enabled(false),
m_pocsagEnabled(false),
m_fmEnabled(false),
m_ax25Enabled(false),
m_cwIdTime(0U),
m_dmrLookup(NULL),
m_nxdnLookup(NULL),
m_callsign(),
m_id(0U),
m_cwCallsign(),
m_lockFileEnabled(false),
m_lockFileName(),
m_remoteControl(NULL),
m_fixedMode(false)
{
	CUDPSocket::startup();
}

CMMDVMHost::~CMMDVMHost()
{
	CUDPSocket::shutdown();
}

int CMMDVMHost::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "MMDVMHost: cannot read the .ini file\n");
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return -1;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1){
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return -1;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1){
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return -1;
		}

#if !defined(HD44780) && !defined(OLED) && !defined(_OPENWRT)
		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return -1;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (::setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return -1;
			}

			if (::setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return -1;
			}

			// Double check it worked (AKA Paranoia)
			if (::setuid(0) != -1){
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return -1;
			}
		}
	}
#else
	::fprintf(stderr, "Dropping root permissions in daemon mode is disabled.\n");
	}
#endif
#endif

	::LogInitialise(m_conf.getLogDisplayLevel(), m_conf.getLogMQTTLevel());

	std::vector<std::pair<std::string, void (*)(const std::string&)>> subscriptions;
	subscriptions.push_back(std::make_pair("display", CMMDVMHost::onDisplay));
	subscriptions.push_back(std::make_pair("command", CMMDVMHost::onCommand));

	m_mqtt = new CMQTTConnection(m_conf.getMQTTHost(), m_conf.getMQTTPort(), m_conf.getMQTTName(), subscriptions, m_conf.getMQTTKeepalive());
	ret = m_mqtt->open();
	if (!ret) {
		::fprintf(stderr, "MMDVMHost: unable to start the MQTT Publisher\n");
		delete m_mqtt;
		m_mqtt = NULL;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
	}
#endif

	LogInfo(HEADER1);
	LogInfo(HEADER2);
	LogInfo(HEADER3);
	LogInfo(HEADER4);

	LogInfo("MMDVMHost-%s is starting", VERSION);
	LogInfo("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	readParams();

	ret = createModem();
	if (!ret)
		return 1;

#if defined(USE_DSTAR)
	if (m_dstarEnabled && !m_modem->hasDStar()) {
		LogWarning("D-Star enabled in the host but not in the modem firmware, disabling");
		m_dstarEnabled = false;
	}
#endif

	if (m_dmrEnabled && !m_modem->hasDMR()) {
		LogWarning("DMR enabled in the host but not in the modem firmware, disabling");
		m_dmrEnabled = false;
	}

	if (m_ysfEnabled && !m_modem->hasYSF()) {
		LogWarning("YSF enabled in the host but not in the modem firmware, disabling");
		m_ysfEnabled = false;
	}

	if (m_p25Enabled && !m_modem->hasP25()) {
		LogWarning("P25 enabled in the host but not in the modem firmware, disabling");
		m_p25Enabled = false;
	}

	if (m_nxdnEnabled && !m_modem->hasNXDN()) {
		LogWarning("NXDN enabled in the host but not in the modem firmware, disabling");
		m_nxdnEnabled = false;
	}

	if (m_m17Enabled && !m_modem->hasM17()) {
		LogWarning("M17 enabled in the host but not in the modem firmware, disabling");
		m_m17Enabled = false;
	}

#if defined(USE_FM)
	if (m_fmEnabled && !m_modem->hasFM()) {
		LogWarning("FM enabled in the host but not in the modem firmware, disabling");
		m_fmEnabled = false;
	}
#endif

#if defined(USE_POCSAG)
	if (m_pocsagEnabled && !m_modem->hasPOCSAG()) {
		LogWarning("POCSAG enabled in the host but not in the modem firmware, disabling");
		m_pocsagEnabled = false;
	}
#endif

#if defined(USE_AX25)
	if (m_ax25Enabled && !m_modem->hasAX25()) {
		LogWarning("AX.25 enabled in the host but not in the modem firmware, disabling");
		m_ax25Enabled = false;
	}
#endif

	LogInfo("Opening network connections");
	writeJSONMessage("Opening network connections");

#if defined(USE_DSTAR)
	if (m_dstarEnabled && m_conf.getDStarNetworkEnabled()) {
		ret = createDStarNetwork();
		if (!ret)
			return 1;
	}
#endif

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

	if (m_p25Enabled && m_conf.getP25NetworkEnabled()) {
		ret = createP25Network();
		if (!ret)
			return 1;
	}

	if (m_nxdnEnabled && m_conf.getNXDNNetworkEnabled()) {
		ret = createNXDNNetwork();
		if (!ret)
			return 1;
	}

	if (m_m17Enabled && m_conf.getM17NetworkEnabled()) {
		ret = createM17Network();
		if (!ret)
			return 1;
	}

#if defined(USE_POCSAG)
	if (m_pocsagEnabled && m_conf.getPOCSAGNetworkEnabled()) {
		ret = createPOCSAGNetwork();
		if (!ret)
			return 1;
	}
#endif

#if defined(USE_FM)
	if (m_fmEnabled && m_conf.getFMNetworkEnabled()) {
		ret = createFMNetwork();
		if (!ret)
			return 1;
	}
#endif

#if defined(USE_AX25)
	if (m_ax25Enabled && m_conf.getAX25NetworkEnabled()) {
		ret = createAX25Network();
		if (!ret)
			return 1;
	}
#endif

	sockaddr_storage transparentAddress;
	unsigned int transparentAddrLen;
	CUDPSocket* transparentSocket = NULL;

	unsigned int sendFrameType = 0U;
	if (m_conf.getTransparentEnabled()) {
		std::string remoteAddress = m_conf.getTransparentRemoteAddress();
		unsigned short remotePort = m_conf.getTransparentRemotePort();
		unsigned short localPort  = m_conf.getTransparentLocalPort();
		sendFrameType             = m_conf.getTransparentSendFrameType();

		LogInfo("Transparent Data");
		LogInfo("    Remote Address: %s", remoteAddress.c_str());
		LogInfo("    Remote Port: %hu", remotePort);
		LogInfo("    Local Port: %hu", localPort);
		LogInfo("    Send Frame Type: %u", sendFrameType);

		if (CUDPSocket::lookup(remoteAddress, remotePort, transparentAddress, transparentAddrLen) != 0) {
			LogError("Unable to resolve the address of the Transparent Data source");
			return 1;
		}

		transparentSocket = new CUDPSocket(localPort);
		ret = transparentSocket->open(transparentAddress);
		if (!ret) {
			LogWarning("Could not open the Transparent data socket, disabling");
			delete transparentSocket;
			transparentSocket = NULL;
			sendFrameType=0;
		}
		m_modem->setTransparentDataParams(sendFrameType);
	}

	if (m_conf.getLockFileEnabled()) {
		m_lockFileEnabled = true;
		m_lockFileName    = m_conf.getLockFileName();

		LogInfo("Lock File Parameters");
		LogInfo("    Name: %s", m_lockFileName.c_str());

		removeLockFile();
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

	LogInfo("Starting protocol handlers");
	writeJSONMessage("Starting protocol handlers");

	CStopWatch stopWatch;
	stopWatch.start();

#if defined(USE_DSTAR)
	if (m_dstarEnabled) {
		std::string module                 = m_conf.getDStarModule();
		bool selfOnly                      = m_conf.getDStarSelfOnly();
		std::vector<std::string> blackList = m_conf.getDStarBlackList();
		std::vector<std::string> whiteList = m_conf.getDStarWhiteList();
		bool ackReply                      = m_conf.getDStarAckReply();
		unsigned int ackTime               = m_conf.getDStarAckTime();
		DSTAR_ACK_MESSAGE ackMessage       = m_conf.getDStarAckMessage();
		bool errorReply                    = m_conf.getDStarErrorReply();
		bool remoteGateway                 = m_conf.getDStarRemoteGateway();
		m_dstarRFModeHang                  = m_conf.getDStarModeHang();

		LogInfo("D-Star RF Parameters");
		LogInfo("    Module: %s", module.c_str());
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Ack Reply: %s", ackReply ? "yes" : "no");
		LogInfo("    Ack message: %s", ackMessage == DSTAR_ACK_RSSI? "RSSI" : (ackMessage == DSTAR_ACK_SMETER ? "SMETER" : "BER"));
		LogInfo("    Ack Time: %ums", ackTime);
		LogInfo("    Error Reply: %s", errorReply ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    Mode Hang: %us", m_dstarRFModeHang);

		if (blackList.size() > 0U)
			LogInfo("    Black List: %u", blackList.size());
		if (whiteList.size() > 0U)
			LogInfo("    White List: %u", whiteList.size());

		m_dstar = new CDStarControl(m_callsign, module, selfOnly, ackReply, ackTime, ackMessage, errorReply, blackList, whiteList, m_dstarNetwork, m_timeout, m_duplex, remoteGateway, rssi);
	}
#endif

	DMR_BEACONS dmrBeacons = DMR_BEACONS_OFF;
	CTimer dmrBeaconIntervalTimer(1000U);
	CTimer dmrBeaconDurationTimer(1000U);

	if (m_dmrEnabled) {
		unsigned int id             = m_conf.getDMRId();
		unsigned int colorCode      = m_conf.getDMRColorCode();
		bool selfOnly               = m_conf.getDMRSelfOnly();
		bool embeddedLCOnly         = m_conf.getDMREmbeddedLCOnly();
		bool dumpTAData             = m_conf.getDMRDumpTAData();
		std::vector<unsigned int> prefixes  = m_conf.getDMRPrefixes();
		std::vector<unsigned int> blackList = m_conf.getDMRBlackList();
		std::vector<unsigned int> whiteList = m_conf.getDMRWhiteList();
		std::vector<unsigned int> slot1TGWhiteList = m_conf.getDMRSlot1TGWhiteList();
		std::vector<unsigned int> slot2TGWhiteList = m_conf.getDMRSlot2TGWhiteList();
		unsigned int callHang       = m_conf.getDMRCallHang();
		unsigned int txHang         = m_conf.getDMRTXHang();
		unsigned int jitter         = m_conf.getDMRNetworkJitter();
		m_dmrRFModeHang             = m_conf.getDMRModeHang();
		dmrBeacons                  = m_conf.getDMRBeacons();
		DMR_OVCM_TYPES ovcm         = m_conf.getDMROVCM();

		if (txHang > m_dmrRFModeHang)
			txHang = m_dmrRFModeHang;

		if (m_conf.getDMRNetworkEnabled()) {
			if (txHang > m_dmrNetModeHang)
				txHang = m_dmrNetModeHang;
		}

		if (callHang > txHang)
			callHang = txHang;

		LogInfo("DMR RF Parameters");
		LogInfo("    Id: %u", id);
		LogInfo("    Color Code: %u", colorCode);
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Embedded LC Only: %s", embeddedLCOnly ? "yes" : "no");
		LogInfo("    Dump Talker Alias Data: %s", dumpTAData ? "yes" : "no");
		LogInfo("    Prefixes: %u", prefixes.size());

		if (blackList.size() > 0U)
			LogInfo("    Source ID Black List: %u", blackList.size());
		if (whiteList.size() > 0U)
			LogInfo("    Source ID White List: %u", whiteList.size());
		if (slot1TGWhiteList.size() > 0U)
			LogInfo("    Slot 1 TG White List: %u", slot1TGWhiteList.size());
		if (slot2TGWhiteList.size() > 0U)
			LogInfo("    Slot 2 TG White List: %u", slot2TGWhiteList.size());

		LogInfo("    Call Hang: %us", callHang);
		LogInfo("    TX Hang: %us", txHang);
		LogInfo("    Mode Hang: %us", m_dmrRFModeHang);
		if (ovcm == DMR_OVCM_OFF)
			LogInfo("    OVCM: off");
		else if (ovcm == DMR_OVCM_RX_ON)
			LogInfo("    OVCM: on(rx only)");
		else if (ovcm == DMR_OVCM_TX_ON)
			LogInfo("    OVCM: on(tx only)");
		else if (ovcm == DMR_OVCM_ON)
			LogInfo("    OVCM: on");
		else if (ovcm == DMR_OVCM_FORCE_OFF)
			LogInfo("    OVCM: off (forced)");

		switch (dmrBeacons) {
			case DMR_BEACONS_NETWORK: {
					unsigned int dmrBeaconDuration = m_conf.getDMRBeaconDuration();

					LogInfo("    DMR Roaming Beacons Type: network");
					LogInfo("    DMR Roaming Beacons Duration: %us", dmrBeaconDuration);

					dmrBeaconDurationTimer.setTimeout(dmrBeaconDuration);
				}
				break;
			case DMR_BEACONS_TIMED: {
					unsigned int dmrBeaconInterval = m_conf.getDMRBeaconInterval();
					unsigned int dmrBeaconDuration = m_conf.getDMRBeaconDuration();

					LogInfo("    DMR Roaming Beacons Type: timed");
					LogInfo("    DMR Roaming Beacons Interval: %us", dmrBeaconInterval);
					LogInfo("    DMR Roaming Beacons Duration: %us", dmrBeaconDuration);

					dmrBeaconDurationTimer.setTimeout(dmrBeaconDuration);

					dmrBeaconIntervalTimer.setTimeout(dmrBeaconInterval);
					dmrBeaconIntervalTimer.start();
				}
				break;
			default:
				LogInfo("    DMR Roaming Beacons Type: off");
				break;
		}

		m_dmr = new CDMRControl(id, colorCode, callHang, selfOnly, embeddedLCOnly, dumpTAData, prefixes, blackList, whiteList, slot1TGWhiteList, slot2TGWhiteList, m_timeout, m_modem, m_dmrNetwork, m_duplex, m_dmrLookup, rssi, jitter, ovcm);

		m_dmrTXTimer.setTimeout(txHang);
	}

	if (m_ysfEnabled) {
		bool lowDeviation   = m_conf.getFusionLowDeviation();
		bool remoteGateway  = m_conf.getFusionRemoteGateway();
		unsigned int txHang = m_conf.getFusionTXHang();
		bool selfOnly       = m_conf.getFusionSelfOnly();
		m_ysfRFModeHang     = m_conf.getFusionModeHang();

		LogInfo("YSF RF Parameters");
		LogInfo("    Low Deviation: %s", lowDeviation ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    TX Hang: %us", txHang);
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Mode Hang: %us", m_ysfRFModeHang);

		m_ysf = new CYSFControl(m_callsign, selfOnly, m_ysfNetwork, m_timeout, m_duplex, lowDeviation, remoteGateway, rssi);
	}

	if (m_p25Enabled) {
		unsigned int id     = m_conf.getP25Id();
		unsigned int nac    = m_conf.getP25NAC();
		unsigned int txHang = m_conf.getP25TXHang();
		bool uidOverride    = m_conf.getP25OverrideUID();
		bool selfOnly       = m_conf.getP25SelfOnly();
		bool remoteGateway  = m_conf.getP25RemoteGateway();
		m_p25RFModeHang     = m_conf.getP25ModeHang();

		LogInfo("P25 RF Parameters");
		LogInfo("    Id: %u", id);
		LogInfo("    NAC: $%03X", nac);
		LogInfo("    UID Override: %s", uidOverride ? "yes" : "no");
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    TX Hang: %us", txHang);
		LogInfo("    Mode Hang: %us", m_p25RFModeHang);

		m_p25 = new CP25Control(nac, id, selfOnly, uidOverride, m_p25Network, m_timeout, m_duplex, m_dmrLookup, remoteGateway, rssi);
	}

	if (m_nxdnEnabled) {
		std::string lookupFile  = m_conf.getNXDNIdLookupFile();
		unsigned int reloadTime = m_conf.getNXDNIdLookupTime();

		LogInfo("NXDN Id Lookups");
		LogInfo("    File: %s", lookupFile.length() > 0U ? lookupFile.c_str() : "None");
		if (reloadTime > 0U)
			LogInfo("    Reload: %u hours", reloadTime);

		m_nxdnLookup = new CNXDNLookup(lookupFile, reloadTime);
		m_nxdnLookup->read();

		unsigned int id     = m_conf.getNXDNId();
		unsigned int ran    = m_conf.getNXDNRAN();
		bool selfOnly       = m_conf.getNXDNSelfOnly();
		bool remoteGateway  = m_conf.getNXDNRemoteGateway();
		unsigned int txHang = m_conf.getNXDNTXHang();
		m_nxdnRFModeHang    = m_conf.getNXDNModeHang();

		LogInfo("NXDN RF Parameters");
		LogInfo("    Id: %u", id);
		LogInfo("    RAN: %u", ran);
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
		LogInfo("    TX Hang: %us", txHang);
		LogInfo("    Mode Hang: %us", m_nxdnRFModeHang);

		m_nxdn = new CNXDNControl(ran, id, selfOnly, m_nxdnNetwork, m_timeout, m_duplex, remoteGateway, m_nxdnLookup, rssi);
	}

	if (m_m17Enabled) {
		bool selfOnly          = m_conf.getM17SelfOnly();
		unsigned int can       = m_conf.getM17CAN();
		bool allowEncryption   = m_conf.getM17AllowEncryption();
		unsigned int txHang    = m_conf.getM17TXHang();
		m_m17RFModeHang        = m_conf.getM17ModeHang();

		LogInfo("M17 RF Parameters");
		LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
		LogInfo("    CAN: %u", can);
		LogInfo("    Allow Encryption: %s", allowEncryption ? "yes" : "no");
		LogInfo("    TX Hang: %us", txHang);
		LogInfo("    Mode Hang: %us", m_m17RFModeHang);

		m_m17 = new CM17Control(m_callsign, can, selfOnly, allowEncryption, m_m17Network, m_timeout, m_duplex, rssi);
	}

#if defined(USE_POCSAG)
	CTimer pocsagTimer(1000U, 30U);

	if (m_pocsagEnabled) {
		unsigned int frequency = m_conf.getPOCSAGFrequency();

		LogInfo("POCSAG RF Parameters");
		LogInfo("    Frequency: %uHz", frequency);

		m_pocsag = new CPOCSAGControl(m_pocsagNetwork);

		if (m_pocsagNetwork != NULL)
			pocsagTimer.start();
	}
#endif

#if defined(USE_AX25)
	if (m_ax25Enabled) {
		unsigned int txDelay  = m_conf.getAX25TXDelay();
		int  rxTwist          = m_conf.getAX25RXTwist();
		unsigned int slotTime = m_conf.getAX25SlotTime();
		unsigned int pPersist = m_conf.getAX25PPersist();
		bool trace            = m_conf.getAX25Trace();

		LogInfo("AX.25 RF Parameters");
		LogInfo("    TX Delay: %ums", txDelay);
		LogInfo("    RX Twist: %d", rxTwist);
		LogInfo("    Slot Time: %ums", slotTime);
		LogInfo("    P-Persist: %u", pPersist);
		LogInfo("    Trace: %s", trace ? "yes" : "no");

		m_ax25 = new CAX25Control(m_ax25Network, trace);
	}
#endif

#if defined(USE_FM)
	if (m_fmEnabled) {
		bool preEmphasis  = m_conf.getFMPreEmphasis();
		bool deEmphasis   = m_conf.getFMDeEmphasis();
		float txAudioGain = m_conf.getFMTXAudioGain();
		float rxAudioGain = m_conf.getFMRXAudioGain();
		m_fmRFModeHang    = m_conf.getFMModeHang();

		m_fm = new CFMControl(m_fmNetwork, txAudioGain, rxAudioGain, preEmphasis, deEmphasis);
	}
#endif

	bool remoteControlEnabled = m_conf.getRemoteControlEnabled();
	if (remoteControlEnabled)
		m_remoteControl = new CRemoteControl(this, m_mqtt);

	setMode(MODE_IDLE);

	LogInfo("MMDVMHost-%s is running", VERSION);
	writeJSONMessage("MMDVMHost is running");

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

		unsigned char data[500U];
		unsigned int len;
		bool ret;

#if defined(USE_DSTAR)
		len = m_modem->readDStarData(data);
		if (m_dstar != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = m_dstar->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_dstarRFModeHang);
					setMode(MODE_DSTAR);
				}
			} else if (m_mode == MODE_DSTAR) {
				bool ret = m_dstar->writeModem(data, len);
				if (ret)
					m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("D-Star modem data received when in mode %u", m_mode);
			}
		}
#endif

		len = m_modem->readDMRData1(data);
		if (m_dmr != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				if (m_duplex) {
					bool ret = m_dmr->processWakeup(data);
					if (ret) {
						m_modeTimer.setTimeout(m_dmrRFModeHang);
						setMode(MODE_DMR);
						dmrBeaconDurationTimer.stop();
					}
				} else {
					m_modeTimer.setTimeout(m_dmrRFModeHang);
					setMode(MODE_DMR);
					m_dmr->writeModemSlot1(data, len);
					dmrBeaconDurationTimer.stop();
				}
			} else if (m_mode == MODE_DMR) {
				if (m_duplex && !m_modem->hasTX()) {
					bool ret = m_dmr->processWakeup(data);
					if (ret) {
						m_modem->writeDMRStart(true);
						m_dmrTXTimer.start();
					}
				} else {
					bool ret = m_dmr->writeModemSlot1(data, len);
					if (ret) {
						dmrBeaconDurationTimer.stop();
						m_modeTimer.start();
						if (m_duplex)
							m_dmrTXTimer.start();
					}
				}
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("DMR modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readDMRData2(data);
		if (m_dmr != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				if (m_duplex) {
					bool ret = m_dmr->processWakeup(data);
					if (ret) {
						m_modeTimer.setTimeout(m_dmrRFModeHang);
						setMode(MODE_DMR);
						dmrBeaconDurationTimer.stop();
					}
				} else {
					m_modeTimer.setTimeout(m_dmrRFModeHang);
					setMode(MODE_DMR);
					m_dmr->writeModemSlot2(data, len);
					dmrBeaconDurationTimer.stop();
				}
			} else if (m_mode == MODE_DMR) {
				if (m_duplex && !m_modem->hasTX()) {
					bool ret = m_dmr->processWakeup(data);
					if (ret) {
						m_modem->writeDMRStart(true);
						m_dmrTXTimer.start();
					}
				} else {
					bool ret = m_dmr->writeModemSlot2(data, len);
					if (ret) {
						dmrBeaconDurationTimer.stop();
						m_modeTimer.start();
						if (m_duplex)
							m_dmrTXTimer.start();
					}
				}
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("DMR modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readYSFData(data);
		if (m_ysf != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = m_ysf->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_ysfRFModeHang);
					setMode(MODE_YSF);
				}
			} else if (m_mode == MODE_YSF) {
				bool ret = m_ysf->writeModem(data, len);
				if (ret)
					m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("System Fusion modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readP25Data(data);
		if (m_p25 != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = m_p25->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_p25RFModeHang);
					setMode(MODE_P25);
				}
			} else if (m_mode == MODE_P25) {
				bool ret = m_p25->writeModem(data, len);
				if (ret)
					m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("P25 modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readNXDNData(data);
		if (m_nxdn != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = m_nxdn->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_nxdnRFModeHang);
					setMode(MODE_NXDN);
				}
			} else if (m_mode == MODE_NXDN) {
				bool ret = m_nxdn->writeModem(data, len);
				if (ret)
					m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("NXDN modem data received when in mode %u", m_mode);
			}
		}

		len = m_modem->readM17Data(data);
		if (m_m17 != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = m_m17->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_m17RFModeHang);
					setMode(MODE_M17);
				}
			} else if (m_mode == MODE_M17) {
				bool ret = m_m17->writeModem(data, len);
				if (ret)
					m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("M17 modem data received when in mode %u", m_mode);
			}
		}

#if defined(USE_FM)
		len = m_modem->readFMData(data);
		if (m_fm != NULL && len > 0U) {
			if (m_mode == MODE_IDLE) {
				bool ret = m_fm->writeModem(data, len);
				if (ret) {
					m_modeTimer.setTimeout(m_fmRFModeHang);
					setMode(MODE_FM);
				}
			} else if (m_mode == MODE_FM) {
				bool ret = m_fm->writeModem(data, len);
				if (ret)
					m_modeTimer.start();
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("FM modem data received when in mode %u", m_mode);
			}
		}
#endif

#if defined(USE_AX25)
		len = m_modem->readAX25Data(data);
		if (m_ax25 != NULL && len > 0U) {
			if (m_mode == MODE_IDLE || m_mode == MODE_FM) {
				m_ax25->writeModem(data, len);
			} else if (m_mode != MODE_LOCKOUT) {
				LogWarning("NXDN modem data received when in mode %u", m_mode);
			}
		}
#endif

		len = m_modem->readTransparentData(data);
		if (transparentSocket != NULL && len > 0U)
			transparentSocket->write(data, len, transparentAddress, transparentAddrLen);

		if (!m_fixedMode) {
			if (m_modeTimer.isRunning() && m_modeTimer.hasExpired())
				setMode(MODE_IDLE);
		}

#if defined(USE_DSTAR)
		if (m_dstar != NULL) {
			ret = m_modem->hasDStarSpace();
			if (ret) {
				len = m_dstar->readModem(data);
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
#endif

		if (m_dmr != NULL) {
			ret = m_modem->hasDMRSpace1();
			if (ret) {
				len = m_dmr->readModemSlot1(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_dmrNetModeHang);
						setMode(MODE_DMR);
					}
					if (m_mode == MODE_DMR) {
						if (m_duplex) {
							m_modem->writeDMRStart(true);
							m_dmrTXTimer.start();
						}
						m_modem->writeDMRData1(data, len);
						dmrBeaconDurationTimer.stop();
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("DMR data received when in mode %u", m_mode);
					}
				}
			}

			ret = m_modem->hasDMRSpace2();
			if (ret) {
				len = m_dmr->readModemSlot2(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_dmrNetModeHang);
						setMode(MODE_DMR);
					}
					if (m_mode == MODE_DMR) {
						if (m_duplex) {
							m_modem->writeDMRStart(true);
							m_dmrTXTimer.start();
						}
						m_modem->writeDMRData2(data, len);
						dmrBeaconDurationTimer.stop();
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("DMR data received when in mode %u", m_mode);
					}
				}
			}
		}

		if (m_ysf != NULL) {
			ret = m_modem->hasYSFSpace();
			if (ret) {
				len = m_ysf->readModem(data);
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

		if (m_p25 != NULL) {
			ret = m_modem->hasP25Space();
			if (ret) {
				len = m_p25->readModem(data);
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

		if (m_nxdn != NULL) {
			ret = m_modem->hasNXDNSpace();
			if (ret) {
				len = m_nxdn->readModem(data);
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

		if (m_m17 != NULL) {
			ret = m_modem->hasM17Space();
			if (ret) {
				len = m_m17->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_m17NetModeHang);
						setMode(MODE_M17);
					}
					if (m_mode == MODE_M17) {
						m_modem->writeM17Data(data, len);
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("M17 data received when in mode %u", m_mode);
					}
				}
			}
		}

#if defined(USE_POCSAG)
		if (m_pocsag != NULL) {
			ret = m_modem->hasPOCSAGSpace();
			if (ret) {
				len = m_pocsag->readModem(data);
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
#endif

#if defined(USE_FM)
		if (m_fm != NULL) {
			unsigned int space = m_modem->getFMSpace();
			if (space > 0U) {
				len = m_fm->readModem(data, space);
				if (len > 0U) {
					if (m_mode == MODE_IDLE) {
						m_modeTimer.setTimeout(m_fmNetModeHang);
						setMode(MODE_FM);
					}
					if (m_mode == MODE_FM) {
						m_modem->writeFMData(data, len);
						m_modeTimer.start();
					} else if (m_mode != MODE_LOCKOUT) {
						LogWarning("FM data received when in mode %u", m_mode);
					}
				}
			}
		}
#endif

#if defined(USE_AX25)
		if (m_ax25 != NULL) {
			ret = m_modem->hasAX25Space();
			if (ret) {
				len = m_ax25->readModem(data);
				if (len > 0U) {
					if (m_mode == MODE_IDLE || m_mode == MODE_FM) {
						m_modem->writeAX25Data(data, len);
					}
					else if (m_mode != MODE_LOCKOUT) {
						LogWarning("AX.25 data received when in mode %u", m_mode);
					}
				}
			}
		}
#endif

		if (transparentSocket != NULL) {
			sockaddr_storage address;
			unsigned int addrlen;
			len = transparentSocket->read(data, 200U, address, addrlen);
			if (len > 0U)
				m_modem->writeTransparentData(data, len);
		}

		len = m_modem->readSerialData(data);
		if (len > 0U)
			m_mqtt->publish("display", data, len);

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_modem->clock(ms);

		if (!m_fixedMode)
			m_modeTimer.clock(ms);

		if (m_reload) {
			if (m_dmrLookup != NULL)
				m_dmrLookup->reload();

			if (m_nxdnLookup != NULL)
				m_nxdnLookup->reload();

			m_reload = false;
		}

#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->clock();
#endif
		if (m_dmr != NULL)
			m_dmr->clock();
		if (m_ysf != NULL)
			m_ysf->clock(ms);
		if (m_p25 != NULL)
			m_p25->clock(ms);
		if (m_nxdn != NULL)
			m_nxdn->clock(ms);
		if (m_m17 != NULL)
			m_m17->clock(ms);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->clock(ms);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->clock(ms);
#endif

#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->clock(ms);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->clock(ms);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->clock(ms);
		if (m_p25Network != NULL)
			m_p25Network->clock(ms);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->clock(ms);
		if (m_m17Network != NULL)
			m_m17Network->clock(ms);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->clock(ms);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->clock(ms);
#endif

		m_cwIdTimer.clock(ms);
		if (m_cwIdTimer.isRunning() && m_cwIdTimer.hasExpired()) {
			if (!m_modem->hasTX()){
				LogDebug("sending CW ID");
				m_modem->sendCWId(m_cwCallsign);

				m_cwIdTimer.setTimeout(m_cwIdTime);
				m_cwIdTimer.start();
			}
		}

		switch (dmrBeacons) {
			case DMR_BEACONS_TIMED:
				dmrBeaconIntervalTimer.clock(ms);
				if (dmrBeaconIntervalTimer.isRunning() && dmrBeaconIntervalTimer.hasExpired()) {
					if ((m_mode == MODE_IDLE || m_mode == MODE_DMR) && !m_modem->hasTX()) {
						if (!m_fixedMode && m_mode == MODE_IDLE)
							setMode(MODE_DMR);
						dmrBeaconIntervalTimer.start();
						dmrBeaconDurationTimer.start();
						if (m_duplex)
							LogDebug("sending DMR Roaming Beacon (timed mode)");
					}
				}
				break;
			case DMR_BEACONS_NETWORK:
				if (m_dmrNetwork != NULL) {
					bool beacon = m_dmrNetwork->wantsBeacon();
					if (beacon) {
						if ((m_mode == MODE_IDLE || m_mode == MODE_DMR) && !m_modem->hasTX()) {
							if (!m_fixedMode && m_mode == MODE_IDLE)
								setMode(MODE_DMR);
							dmrBeaconDurationTimer.start();
							if (m_duplex)
								LogDebug("sending DMR Roaming Beacon (network mode)");
						}
					}
				}
				break;
			default:
				break;
		}

		dmrBeaconDurationTimer.clock(ms);
		if (dmrBeaconDurationTimer.isRunning() && dmrBeaconDurationTimer.hasExpired()) {
			if (!m_fixedMode)
				setMode(MODE_IDLE);
			dmrBeaconDurationTimer.stop();
		}

		m_dmrTXTimer.clock(ms);
		if (m_dmrTXTimer.isRunning() && m_dmrTXTimer.hasExpired()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}

#if defined(USE_POCSAG)
		pocsagTimer.clock(ms);
		if (pocsagTimer.isRunning() && pocsagTimer.hasExpired()) {
			assert(m_pocsagNetwork != NULL);
			m_pocsagNetwork->enable(m_mode == MODE_IDLE || m_mode == MODE_POCSAG);
			pocsagTimer.start();
		}
#endif

		if (ms < 5U)
			CThread::sleep(5U);
	}

	setMode(MODE_QUIT);

	if (m_dmrLookup != NULL)
		m_dmrLookup->stop();

	if (m_nxdnLookup != NULL)
		m_nxdnLookup->stop();

	LogInfo("Closing network connections");
	writeJSONMessage("Closing network connections");

#if defined(USE_DSTAR)
	if (m_dstarNetwork != NULL) {
		m_dstarNetwork->close();
		delete m_dstarNetwork;
	}
#endif

	if (m_dmrNetwork != NULL) {
		m_dmrNetwork->close(true);
		delete m_dmrNetwork;
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

	if (m_m17Network != NULL) {
		m_m17Network->close();
		delete m_m17Network;
	}

#if defined(USE_POCSAG)
	if (m_pocsagNetwork != NULL) {
		m_pocsagNetwork->close();
		delete m_pocsagNetwork;
	}
#endif

#if defined(USE_FM)
	if (m_fmNetwork != NULL) {
		m_fmNetwork->close();
		delete m_fmNetwork;
	}
#endif

#if defined(USE_AX25)
	if (m_ax25Network != NULL) {
		m_ax25Network->close();
		delete m_ax25Network;
	}
#endif

	if (transparentSocket != NULL) {
		transparentSocket->close();
		delete transparentSocket;
	}

	delete m_remoteControl;

	LogInfo("Stopping protocol handlers");
	writeJSONMessage("Stopping protocol handlers");

#if defined(USE_DSTAR)
	delete m_dstar;
#endif
	delete m_dmr;
	delete m_ysf;
	delete m_p25;
	delete m_nxdn;
	delete m_m17;
#if defined(USE_POCSAG)
	delete m_pocsag;
#endif
#if defined(USE_FM)
	delete m_fm;
#endif
#if defined(USE_AX25)
	delete m_ax25;
#endif

	LogInfo("MMDVMHost-%s has stopped", VERSION);
	writeJSONMessage("MMDVMHost has stopped");

	m_modem->close();
	delete m_modem;

	if (m_mqtt != NULL) {
		m_mqtt->close();
		delete m_mqtt;
	}

	return 0;
}

bool CMMDVMHost::createModem()
{
	std::string protocol         = m_conf.getModemProtocol();
	std::string uartPort         = m_conf.getModemUARTPort();
	unsigned int uartSpeed       = m_conf.getModemUARTSpeed();
	std::string i2cPort          = m_conf.getModemI2CPort();
	unsigned int i2cAddress      = m_conf.getModemI2CAddress();
	std::string modemAddress     = m_conf.getModemModemAddress();
	unsigned short modemPort     = m_conf.getModemModemPort();
	std::string localAddress     = m_conf.getModemLocalAddress();
	unsigned short localPort     = m_conf.getModemLocalPort();
	bool rxInvert                = m_conf.getModemRXInvert();
	bool txInvert                = m_conf.getModemTXInvert();
	bool pttInvert               = m_conf.getModemPTTInvert();
	unsigned int txDelay         = m_conf.getModemTXDelay();
	unsigned int dmrDelay        = m_conf.getModemDMRDelay();
	float rxLevel                = m_conf.getModemRXLevel();
	float cwIdTXLevel            = m_conf.getModemCWIdTXLevel();
#if defined(USE_DSTAR)
	float dstarTXLevel           = m_conf.getModemDStarTXLevel();
#else
	float dstarTXLevel           = 0.0F;
#endif
	float dmrTXLevel             = m_conf.getModemDMRTXLevel();
	float ysfTXLevel             = m_conf.getModemYSFTXLevel();
	float p25TXLevel             = m_conf.getModemP25TXLevel();
	float nxdnTXLevel            = m_conf.getModemNXDNTXLevel();
	float m17TXLevel             = m_conf.getModemM17TXLevel();
#if defined(USE_POCSAG)
	float pocsagTXLevel          = m_conf.getModemPOCSAGTXLevel();
#else
	float pocsagTXLevel          = 0.0F;
#endif
#if defined(USE_FM)
	float fmTXLevel              = m_conf.getModemFMTXLevel();
#else
	float fmTXLevel              = 0.0F;
#endif
#if defined(USE_AX25)
	float ax25TXLevel            = m_conf.getModemAX25TXLevel();
#else
	float ax25TXLevel            = 0.0F;
#endif
	bool trace                   = m_conf.getModemTrace();
	bool debug                   = m_conf.getModemDebug();
	unsigned int colorCode       = m_conf.getDMRColorCode();
	bool lowDeviation            = m_conf.getFusionLowDeviation();
	unsigned int ysfTXHang       = m_conf.getFusionTXHang();
	unsigned int p25TXHang       = m_conf.getP25TXHang();
	unsigned int nxdnTXHang      = m_conf.getNXDNTXHang();
	unsigned int m17TXHang       = m_conf.getM17TXHang();
	unsigned int rxFrequency     = m_conf.getRXFrequency();
	unsigned int txFrequency     = m_conf.getTXFrequency();
#if defined(USE_POCSAG)
	unsigned int pocsagFrequency = m_conf.getPOCSAGFrequency();
#else
	unsigned int pocsagFrequency = txFrequency;
#endif
	int rxOffset                 = m_conf.getModemRXOffset();
	int txOffset                 = m_conf.getModemTXOffset();
	int rxDCOffset               = m_conf.getModemRXDCOffset();
	int txDCOffset               = m_conf.getModemTXDCOffset();
	float rfLevel                = m_conf.getModemRFLevel();
#if defined(USE_AX25)
	int rxTwist                  = m_conf.getAX25RXTwist();
	unsigned int ax25TXDelay     = m_conf.getAX25TXDelay();
	unsigned int ax25SlotTime    = m_conf.getAX25SlotTime();
	unsigned int ax25PPersist    = m_conf.getAX25PPersist();
#endif
	bool useCOSAsLockout         = m_conf.getModemUseCOSAsLockout();

	LogInfo("Modem Parameters");
	LogInfo("    Protocol: %s", protocol.c_str());

	if (protocol == "uart") {
		LogInfo("    UART Port: %s", uartPort.c_str());
		LogInfo("    UART Speed: %u", uartSpeed);
	} else if (protocol == "udp") {
		LogInfo("    Modem Address: %s", modemAddress.c_str());
		LogInfo("    Modem Port: %hu", modemPort);
		LogInfo("    Local Address: %s", localAddress.c_str());
		LogInfo("    Local Port: %hu", localPort);
	}
#if defined(__linux__)
	else if (protocol == "i2c") {
		LogInfo("    I2C Port: %s", i2cPort.c_str());
		LogInfo("    I2C Address: %02X", i2cAddress);
	}
#endif

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
#if defined(USE_DSTAR)
	LogInfo("    D-Star TX Level: %.1f%%", dstarTXLevel);
#endif
	LogInfo("    DMR TX Level: %.1f%%", dmrTXLevel);
	LogInfo("    YSF TX Level: %.1f%%", ysfTXLevel);
	LogInfo("    P25 TX Level: %.1f%%", p25TXLevel);
	LogInfo("    NXDN TX Level: %.1f%%", nxdnTXLevel);
	LogInfo("    M17 TX Level: %.1f%%", m17TXLevel);
#if defined(USE_POCSAG)
	LogInfo("    POCSAG TX Level: %.1f%%", pocsagTXLevel);
#endif
#if defined(USE_FM)
	LogInfo("    FM TX Level: %.1f%%", fmTXLevel);
#endif
#if defined(USE_AX25)
	LogInfo("    AX.25 TX Level: %.1f%%", ax25TXLevel);
#endif
	LogInfo("    TX Frequency: %uHz (%uHz)", txFrequency, txFrequency + txOffset);
	LogInfo("    Use COS as Lockout: %s", useCOSAsLockout ? "yes" : "no");

	m_modem = new CModem(m_duplex, rxInvert, txInvert, pttInvert, txDelay, dmrDelay, useCOSAsLockout, trace, debug);

	IModemPort* port = NULL;
	if (protocol == "uart")
		port = new CUARTController(uartPort, uartSpeed, true);
	else if (protocol == "udp")
		port = new CUDPController(modemAddress, modemPort, localAddress, localPort);
#if defined(__linux__)
	else if (protocol == "i2c")
		port = new CI2CController(i2cPort, i2cAddress);
#endif
	else if (protocol == "null")
		port = new CNullController;
	else
		return false;

	m_modem->setPort(port);
	m_modem->setModeParams(m_dstarEnabled, m_dmrEnabled, m_ysfEnabled, m_p25Enabled, m_nxdnEnabled, m_m17Enabled, m_pocsagEnabled, m_fmEnabled, m_ax25Enabled);
	m_modem->setLevels(rxLevel, cwIdTXLevel, dstarTXLevel, dmrTXLevel, ysfTXLevel, p25TXLevel, nxdnTXLevel, m17TXLevel, pocsagTXLevel, fmTXLevel, ax25TXLevel);
	m_modem->setRFParams(rxFrequency, rxOffset, txFrequency, txOffset, txDCOffset, rxDCOffset, rfLevel, pocsagFrequency);
	m_modem->setDMRParams(colorCode);
	m_modem->setYSFParams(lowDeviation, ysfTXHang);
	m_modem->setP25Params(p25TXHang);
	m_modem->setNXDNParams(nxdnTXHang);
	m_modem->setM17Params(m17TXHang);
#if defined(USE_AX25)
	m_modem->setAX25Params(rxTwist, ax25TXDelay, ax25SlotTime, ax25PPersist);
#endif

#if defined(USE_FM)
	if (m_fmEnabled) {
		std::string  callsign             = m_conf.getFMCallsign();
		unsigned int callsignSpeed        = m_conf.getFMCallsignSpeed();
		unsigned int callsignFrequency    = m_conf.getFMCallsignFrequency();
		unsigned int callsignTime         = m_conf.getFMCallsignTime();
		unsigned int callsignHoldoff      = m_conf.getFMCallsignHoldoff();
		float        callsignHighLevel    = m_conf.getFMCallsignHighLevel();
		float        callsignLowLevel     = m_conf.getFMCallsignLowLevel();
		bool         callsignAtStart      = m_conf.getFMCallsignAtStart();
		bool         callsignAtEnd        = m_conf.getFMCallsignAtEnd();
		bool         callsignAtLatch      = m_conf.getFMCallsignAtLatch();
		std::string  rfAck                = m_conf.getFMRFAck();
		unsigned int ackSpeed             = m_conf.getFMAckSpeed();
		unsigned int ackFrequency         = m_conf.getFMAckFrequency();
		unsigned int ackMinTime           = m_conf.getFMAckMinTime();
		unsigned int ackDelay             = m_conf.getFMAckDelay();
		float        ackLevel             = m_conf.getFMAckLevel();
		unsigned int timeout              = m_conf.getFMTimeout();
		float        timeoutLevel         = m_conf.getFMTimeoutLevel();
		float        ctcssFrequency       = m_conf.getFMCTCSSFrequency();
		unsigned int ctcssHighThreshold   = m_conf.getFMCTCSSHighThreshold();
		unsigned int ctcssLowThreshold    = m_conf.getFMCTCSSLowThreshold();
		float        ctcssLevel           = m_conf.getFMCTCSSLevel();
		unsigned int kerchunkTime         = m_conf.getFMKerchunkTime();
		unsigned int hangTime             = m_conf.getFMHangTime();
		unsigned int accessMode           = m_conf.getFMAccessMode();
		bool         linkMode             = m_conf.getFMLinkMode();
		bool         cosInvert            = m_conf.getFMCOSInvert();
		bool         noiseSquelch         = m_conf.getFMNoiseSquelch();
		unsigned int squelchHighThreshold = m_conf.getFMSquelchHighThreshold();
		unsigned int squelchLowThreshold  = m_conf.getFMSquelchLowThreshold();
		unsigned int rfAudioBoost         = m_conf.getFMRFAudioBoost();
		float        maxDevLevel          = m_conf.getFMMaxDevLevel();
		unsigned int modeHangTime         = m_conf.getFMModeHang();

		LogInfo("FM Parameters");
		LogInfo("    Callsign: %s", callsign.c_str());
		LogInfo("    Callsign Speed: %uWPM", callsignSpeed);
		LogInfo("    Callsign Frequency: %uHz", callsignFrequency);
		LogInfo("    Callsign Time: %umins", callsignTime);
		LogInfo("    Callsign Holdoff: 1/%u", callsignHoldoff);
		LogInfo("    Callsign High Level: %.1f%%", callsignHighLevel);
		LogInfo("    Callsign Low Level: %.1f%%", callsignLowLevel);
		LogInfo("    Callsign At Start: %s", callsignAtStart ? "yes" : "no");
		LogInfo("    Callsign At End: %s", callsignAtEnd ? "yes" : "no");
		LogInfo("    Callsign At Latch: %s", callsignAtLatch ? "yes" : "no");
		LogInfo("    RF Ack: %s", rfAck.c_str());
		LogInfo("    Ack Speed: %uWPM", ackSpeed);
		LogInfo("    Ack Frequency: %uHz", ackFrequency);
		LogInfo("    Ack Min Time: %us", ackMinTime);
		LogInfo("    Ack Delay: %ums", ackDelay);
		LogInfo("    Ack Level: %.1f%%", ackLevel);
		LogInfo("    Timeout: %us", timeout);
		LogInfo("    Timeout Level: %.1f%%", timeoutLevel);
		LogInfo("    CTCSS Frequency: %.1fHz", ctcssFrequency);
		LogInfo("    CTCSS High Threshold: %u", ctcssHighThreshold);
		LogInfo("    CTCSS Low Threshold: %u", ctcssLowThreshold);
		LogInfo("    CTCSS Level: %.1f%%", ctcssLevel);
		LogInfo("    Kerchunk Time: %us", kerchunkTime);
		LogInfo("    Hang Time: %us", hangTime);
		LogInfo("    Access Mode: %u", accessMode);
		LogInfo("    Link Mode: %s", linkMode ? "yes" : "no");
		LogInfo("    COS Invert: %s", cosInvert ? "yes" : "no");

		LogInfo("    Noise Squelch: %s", noiseSquelch ? "yes" : "no");
		if (noiseSquelch) {
			LogInfo("    Squelch High Threshold: %u", squelchHighThreshold);
			LogInfo("    Squelch Low Threshold: %u", squelchLowThreshold);
		}

		LogInfo("    RF Audio Boost: x%u", rfAudioBoost);
		LogInfo("    Max. Deviation Level: %.1f%%", maxDevLevel);
		LogInfo("    Mode Hang: %us", modeHangTime);

		m_modem->setFMCallsignParams(callsign, callsignSpeed, callsignFrequency, callsignTime, callsignHoldoff, callsignHighLevel, callsignLowLevel, callsignAtStart, callsignAtEnd, callsignAtLatch);
		m_modem->setFMAckParams(rfAck, ackSpeed, ackFrequency, ackMinTime, ackDelay, ackLevel);
		m_modem->setFMMiscParams(timeout, timeoutLevel, ctcssFrequency, ctcssHighThreshold, ctcssLowThreshold, ctcssLevel, kerchunkTime, hangTime, accessMode, linkMode, cosInvert, noiseSquelch, squelchHighThreshold, squelchLowThreshold, rfAudioBoost, maxDevLevel);

		if (m_conf.getFMNetworkEnabled()) {
			std::string  extAck        = m_conf.getFMExtAck();
			unsigned int extAudioBoost = m_conf.getFMExtAudioBoost();

			LogInfo("    Ext. Ack: %s", extAck.c_str());
			LogInfo("    Ext. Audio Boost: x%u", extAudioBoost);

			m_modem->setFMExtParams(extAck, extAudioBoost);
		}
	}
#endif

	bool ret = m_modem->open();
	if (!ret) {
		delete m_modem;
		m_modem = NULL;
		return false;
	}

	return true;
}

#if defined(USE_DSTAR)
bool CMMDVMHost::createDStarNetwork()
{
	std::string gatewayAddress = m_conf.getDStarGatewayAddress();
	unsigned short gatewayPort = m_conf.getDStarGatewayPort();
	std::string localAddress   = m_conf.getDStarLocalAddress();
	unsigned short localPort   = m_conf.getDStarLocalPort();
	bool debug                 = m_conf.getDStarNetworkDebug();
	m_dstarNetModeHang         = m_conf.getDStarNetworkModeHang();

	LogInfo("D-Star Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
	LogInfo("    Mode Hang: %us", m_dstarNetModeHang);

	m_dstarNetwork = new CDStarNetwork(gatewayAddress, gatewayPort, localAddress, localPort, m_duplex, VERSION, debug);

	bool ret = m_dstarNetwork->open();
	if (!ret) {
		delete m_dstarNetwork;
		m_dstarNetwork = NULL;
		return false;
	}

	m_dstarNetwork->enable(true);

	return true;
}
#endif

bool CMMDVMHost::createDMRNetwork()
{
	std::string gatewayAddress = m_conf.getDMRNetworkGatewayAddress();
	unsigned short gatewayPort = m_conf.getDMRNetworkGatewayPort();
	std::string localAddress   = m_conf.getDMRNetworkLocalAddress();
	unsigned short localPort   = m_conf.getDMRNetworkLocalPort();
	unsigned int id      = m_conf.getDMRId();
	bool debug           = m_conf.getDMRNetworkDebug();
	unsigned int jitter  = m_conf.getDMRNetworkJitter();
	bool slot1           = m_conf.getDMRNetworkSlot1();
	bool slot2           = m_conf.getDMRNetworkSlot2();
	HW_TYPE hwType       = m_modem->getHWType();
	m_dmrNetModeHang     = m_conf.getDMRNetworkModeHang();

	LogInfo("DMR Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
	LogInfo("    Jitter: %ums", jitter);
	LogInfo("    Slot 1: %s", slot1 ? "enabled" : "disabled");
	LogInfo("    Slot 2: %s", slot2 ? "enabled" : "disabled");
	LogInfo("    Mode Hang: %us", m_dmrNetModeHang);

	m_dmrNetwork = new CDMRNetwork(gatewayAddress, gatewayPort, localAddress, localPort, id, m_duplex, VERSION, slot1, slot2, hwType, debug);

	unsigned int rxFrequency = m_conf.getRXFrequency();
	unsigned int txFrequency = m_conf.getTXFrequency();
	unsigned int power       = m_conf.getPower();
	unsigned int colorCode   = m_conf.getDMRColorCode();

	LogInfo("Info Parameters");
	LogInfo("    Callsign: %s", m_callsign.c_str());
	LogInfo("    RX Frequency: %uHz", rxFrequency);
	LogInfo("    TX Frequency: %uHz", txFrequency);
	LogInfo("    Power: %uW", power);

	m_dmrNetwork->setConfig(m_callsign, rxFrequency, txFrequency, power, colorCode);
	
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
	std::string localAddress   = m_conf.getFusionNetworkLocalAddress();
	unsigned short localPort   = m_conf.getFusionNetworkLocalPort();
	std::string gatewayAddress = m_conf.getFusionNetworkGatewayAddress();
	unsigned short gatewayPort = m_conf.getFusionNetworkGatewayPort();
	m_ysfNetModeHang           = m_conf.getFusionNetworkModeHang();
	bool debug                 = m_conf.getFusionNetworkDebug();

	LogInfo("System Fusion Network Parameters");
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Mode Hang: %us", m_ysfNetModeHang);

	m_ysfNetwork = new CYSFNetwork(localAddress, localPort, gatewayAddress, gatewayPort, m_callsign, debug);

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
	std::string gatewayAddress = m_conf.getP25GatewayAddress();
	unsigned short gatewayPort = m_conf.getP25GatewayPort();
	std::string localAddress   = m_conf.getP25LocalAddress();
	unsigned short localPort   = m_conf.getP25LocalPort();
	m_p25NetModeHang           = m_conf.getP25NetworkModeHang();
	bool debug                 = m_conf.getP25NetworkDebug();

	LogInfo("P25 Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
	LogInfo("    Mode Hang: %us", m_p25NetModeHang);

	m_p25Network = new CP25Network(gatewayAddress, gatewayPort, localAddress, localPort, debug);

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
	std::string protocol       = m_conf.getNXDNNetworkProtocol();
	std::string gatewayAddress = m_conf.getNXDNGatewayAddress();
	unsigned short gatewayPort = m_conf.getNXDNGatewayPort();
	std::string localAddress   = m_conf.getNXDNLocalAddress();
	unsigned short localPort   = m_conf.getNXDNLocalPort();
	m_nxdnNetModeHang          = m_conf.getNXDNNetworkModeHang();
	bool debug                 = m_conf.getNXDNNetworkDebug();

	LogInfo("NXDN Network Parameters");
	LogInfo("    Protocol: %s", protocol.c_str());
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
	LogInfo("    Mode Hang: %us", m_nxdnNetModeHang);

	if (protocol == "Kenwood")
		m_nxdnNetwork = new CNXDNKenwoodNetwork(localAddress, localPort, gatewayAddress, gatewayPort, debug);
	else
		m_nxdnNetwork = new CNXDNIcomNetwork(localAddress, localPort, gatewayAddress, gatewayPort, debug);

	bool ret = m_nxdnNetwork->open();
	if (!ret) {
		delete m_nxdnNetwork;
		m_nxdnNetwork = NULL;
		return false;
	}

	m_nxdnNetwork->enable(true);

	return true;
}

bool CMMDVMHost::createM17Network()
{
	std::string gatewayAddress = m_conf.getM17GatewayAddress();
	unsigned short gatewayPort = m_conf.getM17GatewayPort();
	std::string localAddress   = m_conf.getM17LocalAddress();
	unsigned short localPort   = m_conf.getM17LocalPort();
	m_m17NetModeHang           = m_conf.getM17NetworkModeHang();
	bool debug                 = m_conf.getM17NetworkDebug();

	LogInfo("M17 Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
	LogInfo("    Mode Hang: %us", m_m17NetModeHang);

	m_m17Network = new CM17Network(localAddress, localPort, gatewayAddress, gatewayPort, debug);
	bool ret = m_m17Network->open();
	if (!ret) {
		delete m_m17Network;
		m_m17Network = NULL;
		return false;
	}

	m_m17Network->enable(true);

	return true;
}

#if defined(USE_POCSAG)
bool CMMDVMHost::createPOCSAGNetwork()
{
	std::string gatewayAddress = m_conf.getPOCSAGGatewayAddress();
	unsigned short gatewayPort = m_conf.getPOCSAGGatewayPort();
	std::string localAddress   = m_conf.getPOCSAGLocalAddress();
	unsigned short localPort   = m_conf.getPOCSAGLocalPort();
	m_pocsagNetModeHang        = m_conf.getPOCSAGNetworkModeHang();
	bool debug                 = m_conf.getPOCSAGNetworkDebug();

	LogInfo("POCSAG Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
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
#endif

#if defined(USE_FM)
bool CMMDVMHost::createFMNetwork()
{
	std::string callsign       = m_conf.getFMCallsign();
	std::string protocol       = m_conf.getFMNetworkProtocol();
	std::string gatewayAddress = m_conf.getFMGatewayAddress();
	unsigned short gatewayPort = m_conf.getFMGatewayPort();
	std::string localAddress   = m_conf.getFMLocalAddress();
	unsigned short localPort   = m_conf.getFMLocalPort();
	bool preEmphasis           = m_conf.getFMPreEmphasis();
	bool deEmphasis            = m_conf.getFMDeEmphasis();
	float txAudioGain          = m_conf.getFMTXAudioGain();
	float rxAudioGain          = m_conf.getFMRXAudioGain();
	m_fmNetModeHang            = m_conf.getFMNetworkModeHang();
	bool debug                 = m_conf.getFMNetworkDebug();

	LogInfo("FM Network Parameters");
	LogInfo("    Protocol: %s", protocol.c_str());
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %hu", gatewayPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %hu", localPort);
	LogInfo("    Pre-Emphasis: %s", preEmphasis ? "yes" : "no");
	LogInfo("    De-Emphasis: %s", deEmphasis ? "yes" : "no");
	LogInfo("    TX Audio Gain: %.2f", txAudioGain);
	LogInfo("    RX Audio Gain: %.2f", rxAudioGain);
	LogInfo("    Mode Hang: %us", m_fmNetModeHang);

	m_fmNetwork = new CFMNetwork(callsign, protocol, localAddress, localPort, gatewayAddress, gatewayPort, debug);

	bool ret = m_fmNetwork->open();
	if (!ret) {
		delete m_fmNetwork;
		m_fmNetwork = NULL;
		return false;
	}

	m_fmNetwork->enable(true);

	return true;
}
#endif

#if defined(USE_AX25)
bool CMMDVMHost::createAX25Network()
{
	std::string  port = m_conf.getAX25NetworkPort();
	unsigned int speed = m_conf.getAX25NetworkSpeed();
	bool debug = m_conf.getAX25NetworkDebug();

	LogInfo("AX.25 Network Parameters");
	LogInfo("    Port: %s", port.c_str());
	LogInfo("    Speed: %u", speed);

	m_ax25Network = new CAX25Network(port, speed, debug);

	bool ret = m_ax25Network->open();
	if (!ret) {
		delete m_ax25Network;
		m_ax25Network = NULL;
		return false;
	}

	m_ax25Network->enable(true);

	return true;
}
#endif

void CMMDVMHost::readParams()
{
#if defined(USE_DSTAR)
	m_dstarEnabled  = m_conf.getDStarEnabled();
#endif
	m_dmrEnabled    = m_conf.getDMREnabled();
	m_ysfEnabled    = m_conf.getFusionEnabled();
	m_p25Enabled    = m_conf.getP25Enabled();
	m_nxdnEnabled   = m_conf.getNXDNEnabled();
	m_m17Enabled    = m_conf.getM17Enabled();
#if defined(USE_POCSAG)
	m_pocsagEnabled = m_conf.getPOCSAGEnabled();
#endif
#if defined(USE_FM)
	m_fmEnabled     = m_conf.getFMEnabled();
#endif
#if defined(USE_AX25)
	m_ax25Enabled   = m_conf.getAX25Enabled();
#endif
	m_duplex        = m_conf.getDuplex();
	m_callsign      = m_conf.getCallsign();
	m_id            = m_conf.getId();
	m_timeout       = m_conf.getTimeout();

	LogInfo("General Parameters");
	LogInfo("    Callsign: %s", m_callsign.c_str());
	LogInfo("    Id: %u", m_id);
	LogInfo("    Duplex: %s", m_duplex ? "yes" : "no");
	LogInfo("    Timeout: %us", m_timeout);
#if defined(USE_DSTAR)
	LogInfo("    D-Star: %s", m_dstarEnabled ? "enabled" : "disabled");
#endif
	LogInfo("    DMR: %s", m_dmrEnabled ? "enabled" : "disabled");
	LogInfo("    YSF: %s", m_ysfEnabled ? "enabled" : "disabled");
	LogInfo("    P25: %s", m_p25Enabled ? "enabled" : "disabled");
	LogInfo("    NXDN: %s", m_nxdnEnabled ? "enabled" : "disabled");
	LogInfo("    M17: %s", m_m17Enabled ? "enabled" : "disabled");
#if defined(USE_POCSAG)
	LogInfo("    POCSAG: %s", m_pocsagEnabled ? "enabled" : "disabled");
#endif
#if defined(USE_FM)
	LogInfo("    FM: %s", m_fmEnabled ? "enabled" : "disabled");
#endif
#if defined(USE_AX25)
	LogInfo("    AX.25: %s", m_ax25Enabled ? "enabled" : "disabled");
#endif
}

void CMMDVMHost::setMode(unsigned char mode)
{
	assert(m_modem != NULL);

	switch (mode) {
	case MODE_DSTAR:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(true);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(true);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		m_modem->setMode(MODE_DSTAR);
		m_mode = MODE_DSTAR;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("D-Star");
		writeJSONMode("D-Star");
		break;

	case MODE_DMR:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(true);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(true);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		m_modem->setMode(MODE_DMR);
		if (m_duplex) {
			m_modem->writeDMRStart(true);
			m_dmrTXTimer.start();
		}
		m_mode = MODE_DMR;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("DMR");
		writeJSONMode("DMR");
		break;

	case MODE_YSF:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(true);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(true);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		m_modem->setMode(MODE_YSF);
		m_mode = MODE_YSF;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("System Fusion");
		writeJSONMode("YSF");
		break;

	case MODE_P25:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(true);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(true);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		m_modem->setMode(MODE_P25);
		m_mode = MODE_P25;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("P25");
		writeJSONMode("P25");
		break;

	case MODE_NXDN:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(true);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(true);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		m_modem->setMode(MODE_NXDN);
		m_mode = MODE_NXDN;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("NXDN");
		writeJSONMode("NXDN");
		break;

	case MODE_M17:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(true);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(true);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		m_modem->setMode(MODE_M17);
		m_mode = MODE_M17;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("M17");
		writeJSONMode("M17");
		break;

	case MODE_POCSAG:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(true);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(true);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		m_modem->setMode(MODE_POCSAG);
		m_mode = MODE_POCSAG;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("POCSAG");
		writeJSONMode("POCSAG");
		break;

	case MODE_FM:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(true);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(true);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(true);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(true);
#endif
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}
		m_modem->setMode(MODE_FM);
		m_mode = MODE_FM;
		m_modeTimer.start();
		m_cwIdTimer.stop();
		createLockFile("FM");
		writeJSONMode("FM");
		break;

	case MODE_LOCKOUT:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}
		m_modem->setMode(MODE_IDLE);
		m_mode = MODE_LOCKOUT;
		m_modeTimer.stop();
		m_cwIdTimer.stop();
		removeLockFile();
		writeJSONMode("lockout");
		break;

	case MODE_ERROR:
		LogMessage("Mode set to Error");
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(false);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(false);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(false);
		if (m_p25Network != NULL)
			m_p25Network->enable(false);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(false);
		if (m_m17Network != NULL)
			m_m17Network->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(false);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(false);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(false);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(false);
		if (m_ysf != NULL)
			m_ysf->enable(false);
		if (m_p25 != NULL)
			m_p25->enable(false);
		if (m_nxdn != NULL)
			m_nxdn->enable(false);
		if (m_m17 != NULL)
			m_m17->enable(false);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(false);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(false);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(false);
#endif
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}
		m_mode = MODE_ERROR;
		m_modeTimer.stop();
		m_cwIdTimer.stop();
		removeLockFile();
		writeJSONMode("error");
		break;

	default:
#if defined(USE_DSTAR)
		if (m_dstarNetwork != NULL)
			m_dstarNetwork->enable(true);
#endif
		if (m_dmrNetwork != NULL)
			m_dmrNetwork->enable(true);
		if (m_ysfNetwork != NULL)
			m_ysfNetwork->enable(true);
		if (m_p25Network != NULL)
			m_p25Network->enable(true);
		if (m_nxdnNetwork != NULL)
			m_nxdnNetwork->enable(true);
		if (m_m17Network != NULL)
			m_m17Network->enable(true);
#if defined(USE_POCSAG)
		if (m_pocsagNetwork != NULL)
			m_pocsagNetwork->enable(true);
#endif
#if defined(USE_FM)
		if (m_fmNetwork != NULL)
			m_fmNetwork->enable(true);
#endif
#if defined(USE_AX25)
		if (m_ax25Network != NULL)
			m_ax25Network->enable(true);
#endif
#if defined(USE_DSTAR)
		if (m_dstar != NULL)
			m_dstar->enable(true);
#endif
		if (m_dmr != NULL)
			m_dmr->enable(true);
		if (m_ysf != NULL)
			m_ysf->enable(true);
		if (m_p25 != NULL)
			m_p25->enable(true);
		if (m_nxdn != NULL)
			m_nxdn->enable(true);
		if (m_m17 != NULL)
			m_m17->enable(true);
#if defined(USE_POCSAG)
		if (m_pocsag != NULL)
			m_pocsag->enable(true);
#endif
#if defined(USE_FM)
		if (m_fm != NULL)
			m_fm->enable(true);
#endif
#if defined(USE_AX25)
		if (m_ax25 != NULL)
			m_ax25->enable(true);
#endif
		if (m_mode == MODE_DMR && m_duplex && m_modem->hasTX()) {
			m_modem->writeDMRStart(false);
			m_dmrTXTimer.stop();
		}
		m_modem->setMode(MODE_IDLE);
		if (m_mode == MODE_ERROR) {
			m_modem->sendCWId(m_callsign);
			m_cwIdTimer.setTimeout(m_cwIdTime);
			m_cwIdTimer.start();
		} else {
			m_cwIdTimer.setTimeout(m_cwIdTime / 4U);
			m_cwIdTimer.start();
		}
		m_mode = MODE_IDLE;
		m_modeTimer.stop();
		removeLockFile();
		writeJSONMode("idle");
		break;
	}
}

void CMMDVMHost::createLockFile(const char* mode) const
{
	if (m_lockFileEnabled) {
		FILE* fp = ::fopen(m_lockFileName.c_str(), "wt");
		if (fp != NULL) {
			::fprintf(fp, "%s\n", mode);
			::fclose(fp);
		}
	}
}

void CMMDVMHost::removeLockFile() const
{
	if (m_lockFileEnabled)
		::remove(m_lockFileName.c_str());
}

void CMMDVMHost::remoteControl(const std::string& commandString)
{
	if (m_remoteControl == NULL)
		return;

	REMOTE_COMMAND command = m_remoteControl->getCommand(commandString);
	switch (command) {
		case RCD_MODE_IDLE:
			m_fixedMode = false;
			setMode(MODE_IDLE);
			break;
		case RCD_MODE_LOCKOUT:
			m_fixedMode = false;
			setMode(MODE_LOCKOUT);
			break;
#if defined(USE_DSTAR)
		case RCD_MODE_DSTAR:
			if (m_dstar != NULL)
				processModeCommand(MODE_DSTAR, m_dstarRFModeHang);
			break;
#endif
		case RCD_MODE_DMR:
			if (m_dmr != NULL)
				processModeCommand(MODE_DMR, m_dmrRFModeHang);
			break;
		case RCD_MODE_YSF:
			if (m_ysf != NULL)
				processModeCommand(MODE_YSF, m_ysfRFModeHang);
			break;
		case RCD_MODE_P25:
			if (m_p25 != NULL)
				processModeCommand(MODE_P25, m_p25RFModeHang);
			break;
		case RCD_MODE_NXDN:
			if (m_nxdn != NULL)
				processModeCommand(MODE_NXDN, m_nxdnRFModeHang);
			break;
		case RCD_MODE_M17:
			if (m_m17 != NULL)
				processModeCommand(MODE_M17, m_m17RFModeHang);
			break;
#if defined(USE_FM)
		case RCD_MODE_FM:
			if (m_fmEnabled)
				processModeCommand(MODE_FM, 0);
			break;
#endif
#if defined(USE_DSTAR)
		case RCD_ENABLE_DSTAR:
			if (m_dstar != NULL && !m_dstarEnabled)
				processEnableCommand(m_dstarEnabled, true);
			if (m_dstarNetwork != NULL)
				m_dstarNetwork->enable(true);
			break;
#endif
		case RCD_ENABLE_DMR:
			if (m_dmr != NULL && !m_dmrEnabled)
				processEnableCommand(m_dmrEnabled, true);
			if (m_dmrNetwork != NULL)
				m_dmrNetwork->enable(true);
			break;
		case RCD_ENABLE_YSF:
			if (m_ysf != NULL && !m_ysfEnabled)
				processEnableCommand(m_ysfEnabled, true);
			if (m_ysfNetwork != NULL)
				m_ysfNetwork->enable(true);
			break;
		case RCD_ENABLE_P25:
			if (m_p25 != NULL && !m_p25Enabled)
				processEnableCommand(m_p25Enabled, true);
			if (m_p25Network != NULL)
				m_p25Network->enable(true);
			break;
		case RCD_ENABLE_NXDN:
			if (m_nxdn != NULL && !m_nxdnEnabled)
				processEnableCommand(m_nxdnEnabled, true);
			if (m_nxdnNetwork != NULL)
				m_nxdnNetwork->enable(true);
			break;
		case RCD_ENABLE_M17:
			if (m_m17 != NULL && !m_m17Enabled)
				processEnableCommand(m_m17Enabled, true);
			if (m_m17Network != NULL)
				m_m17Network->enable(true);
			break;
#if defined(USE_FM)
		case RCD_ENABLE_FM:
			if (!m_fmEnabled)
				processEnableCommand(m_fmEnabled, true);
			break;
#endif
#if defined(USE_AX25)
		case RCD_ENABLE_AX25:
			if (!m_ax25Enabled)
				processEnableCommand(m_ax25Enabled, true);
			break;
#endif
#if defined(USE_DSTAR)
		case RCD_DISABLE_DSTAR:
			if (m_dstar != NULL && m_dstarEnabled)
				processEnableCommand(m_dstarEnabled, false);
			if (m_dstarNetwork != NULL)
				m_dstarNetwork->enable(false);
			break;
#endif
		case RCD_DISABLE_DMR:
			if (m_dmr != NULL && m_dmrEnabled)
				processEnableCommand(m_dmrEnabled, false);
			if (m_dmrNetwork != NULL)
				m_dmrNetwork->enable(false);
			break;
		case RCD_DISABLE_YSF:
			if (m_ysf != NULL && m_ysfEnabled)
				processEnableCommand(m_ysfEnabled, false);
			if (m_ysfNetwork != NULL)
				m_ysfNetwork->enable(false);
			break;
		case RCD_DISABLE_P25:
			if (m_p25 != NULL && m_p25Enabled)
				processEnableCommand(m_p25Enabled, false);
			if (m_p25Network != NULL)
				m_p25Network->enable(false);
			break;
		case RCD_DISABLE_NXDN:
			if (m_nxdn != NULL && m_nxdnEnabled)
				processEnableCommand(m_nxdnEnabled, false);
			if (m_nxdnNetwork != NULL)
				m_nxdnNetwork->enable(false);
			break;
		case RCD_DISABLE_M17:
			if (m_m17 != NULL && m_m17Enabled)
				processEnableCommand(m_m17Enabled, false);
			if (m_m17Network != NULL)
				m_m17Network->enable(false);
			break;
#if defined(USE_FM)
		case RCD_DISABLE_FM:
			if (m_fmEnabled)
				processEnableCommand(m_fmEnabled, false);
			break;
#endif
#if defined(USE_AX25)
		case RCD_DISABLE_AX25:
			if (m_ax25Enabled)
				processEnableCommand(m_ax25Enabled, false);
			break;
#endif
#if defined(USE_POCSAG)
		case RCD_PAGE:
			if (m_pocsag != NULL) {
				unsigned int ric = m_remoteControl->getArgUInt(0U);
				std::string text;
				for (unsigned int i = 1U; i < m_remoteControl->getArgCount(); i++) {
					if (i > 1U)
						text += " ";
					text += m_remoteControl->getArgString(i);
				}
				m_pocsag->sendPage(ric, text);
			}
			break;
		case RCD_PAGE_BCD:
			if (m_pocsag != NULL) {
				unsigned int ric = m_remoteControl->getArgUInt(0U);
				std::string text;
				for (unsigned int i = 1U; i < m_remoteControl->getArgCount(); i++) {
					if (i > 1U)
						text += " ";
					text += m_remoteControl->getArgString(i);
				}
				m_pocsag->sendPageBCD(ric, text);
			}
			break;
		case RCD_PAGE_A1:
			if (m_pocsag != NULL) {
				unsigned int ric = m_remoteControl->getArgUInt(0U);
				m_pocsag->sendPageAlert1(ric);
			}
			break;
		case RCD_PAGE_A2:
			if (m_pocsag != NULL) {
				unsigned int ric = m_remoteControl->getArgUInt(0U);
				std::string text;
				for (unsigned int i = 1U; i < m_remoteControl->getArgCount(); i++) {
					if (i > 1U)
						text += " ";
					text += m_remoteControl->getArgString(i);
				}
				m_pocsag->sendPageAlert2(ric, text);
			}
			break;
#endif
		case RCD_CW:
			setMode(MODE_IDLE); // Force the modem to go idle so that we can send the CW text.
			if (!m_modem->hasTX()) {
				std::string cwtext;
				for (unsigned int i = 0U; i < m_remoteControl->getArgCount(); i++) {
					if (i > 0U)
						cwtext += " ";
					cwtext += m_remoteControl->getArgString(i);
				}
				m_modem->sendCWId(cwtext);
			}
			break;
		case RCD_RELOAD:
			m_reload = true;
			break;
		default:
			break;
	}
}

void CMMDVMHost::processModeCommand(unsigned char mode, unsigned int timeout)
{
	m_fixedMode = false;
	m_modeTimer.setTimeout(timeout);

	if (m_remoteControl->getArgCount() > 0U) {
		if (m_remoteControl->getArgString(0U) == "fixed") {
			m_fixedMode = true;
		} else {
			unsigned int t = m_remoteControl->getArgUInt(0U);
			if (t > 0U)
				m_modeTimer.setTimeout(t);
		}
	}

	setMode(mode);
}

void CMMDVMHost::processEnableCommand(bool& mode, bool enabled)
{
	LogDebug("Setting mode current=%s new=%s", mode ? "true" : "false", enabled ? "true" : "false");

	mode = enabled;

	m_modem->setModeParams(m_dstarEnabled, m_dmrEnabled, m_ysfEnabled, m_p25Enabled, m_nxdnEnabled, m_m17Enabled, m_pocsagEnabled, m_fmEnabled, m_ax25Enabled);
	if (!m_modem->writeConfig())
		LogError("Cannot write Config to MMDVM");
}

void CMMDVMHost::buildNetworkStatusString(std::string &str)
{
	str = "";

#if defined(USE_DSTAR)
	str += std::string("dstar:") + (((m_dstarNetwork == NULL) || !m_dstarEnabled) ? "n/a" : (m_dstarNetwork->isConnected() ? "conn" : "disc"));
#endif
	str += std::string(" dmr:") + (((m_dmrNetwork == NULL) || !m_dmrEnabled) ? "n/a" : (m_dmrNetwork->isConnected() ? "conn" : "disc"));
	str += std::string(" ysf:") + (((m_ysfNetwork == NULL) || !m_ysfEnabled) ? "n/a" : (m_ysfNetwork->isConnected() ? "conn" : "disc"));
	str += std::string(" p25:") + (((m_p25Network == NULL) || !m_p25Enabled) ? "n/a" : (m_p25Network->isConnected() ? "conn" : "disc"));
	str += std::string(" nxdn:") + (((m_nxdnNetwork == NULL) || !m_nxdnEnabled) ? "n/a" : (m_nxdnNetwork->isConnected() ? "conn" : "disc"));
	str += std::string(" m17:") + (((m_m17Network == NULL) || !m_m17Enabled) ? "n/a" : (m_m17Network->isConnected() ? "conn" : "disc"));
#if defined(USE_FM)
	str += std::string(" fm:") + (m_fmEnabled ? "conn" : "n/a");
#endif
}

void CMMDVMHost::buildNetworkHostsString(std::string &str)
{
	str = "";

#if defined(USE_DSTAR)
	std::string dstarReflector;
	if (m_dstarEnabled && (m_dstarNetwork != NULL)) {
		unsigned char ref[DSTAR_LONG_CALLSIGN_LENGTH + 1];
		LINK_STATUS status;

		::memset(ref, 0, sizeof(ref));

		m_dstarNetwork->getStatus(status, &ref[0]);
		switch (status) {
			case LINK_STATUS::LS_LINKED_LOOPBACK:
			case LINK_STATUS::LS_LINKED_DEXTRA:
			case LINK_STATUS::LS_LINKED_DPLUS:
			case LINK_STATUS::LS_LINKED_DCS:
			case LINK_STATUS::LS_LINKED_CCS:
			     dstarReflector = std::string((char *)ref);
			     break;

			default:
			     break;
		}
	}

	str += std::string("dstar:\"") + ((dstarReflector.length() == 0) ? "NONE" : dstarReflector) + "\"";
#endif
	str += std::string(" dmr:\"") + ((m_dmrEnabled && (m_dmrNetwork != NULL)) ? m_conf.getDMRNetworkGatewayAddress() : "NONE") + "\"";
	str += std::string(" ysf:\"") + ((m_ysfEnabled && (m_ysfNetwork != NULL)) ? m_conf.getFusionNetworkGatewayAddress() : "NONE") + "\"";
	str += std::string(" p25:\"") + ((m_p25Enabled && (m_p25Network != NULL)) ? m_conf.getP25GatewayAddress() : "NONE") + "\"";
	str += std::string(" nxdn:\"") + ((m_nxdnEnabled && (m_nxdnNetwork != NULL)) ? m_conf.getNXDNGatewayAddress() : "NONE") + "\"";
	str += std::string(" m17:\"") + ((m_m17Enabled && (m_m17Network != NULL)) ? m_conf.getM17GatewayAddress() : "NONE") + "\"";
#if defined(USE_FM)
	str += std::string(" fm:\"") + ((m_fmEnabled && (m_fmNetwork != NULL)) ? m_conf.getFMGatewayAddress() : "NONE") + "\"";
#endif
}

void CMMDVMHost::writeJSONMode(const std::string& mode)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["mode"]      = mode;

	WriteJSON("MMDVM", json);
}

void CMMDVMHost::writeJSONMessage(const std::string& message)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["message"]   = message;

	WriteJSON("MMDVM", json);
}

void CMMDVMHost::writeSerial(const std::string& message)
{
	assert(m_modem != NULL);

	m_modem->writeSerialData((unsigned char*)message.c_str(), message.length());
}

void CMMDVMHost::onCommand(const std::string& command)
{
	assert(host != NULL);

	host->remoteControl(command);
}

void CMMDVMHost::onDisplay(const std::string& message)
{
	assert(host != NULL);

	host->writeSerial(message);	
}

