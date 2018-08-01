/*
*   Copyright (C) 2018 by shawn.chain@gmail.com BG5HHP
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

#include "MMDVMTask.h"
#include "MMDVMHost.h"
#include "RSSIInterpolator.h"
#include "Conf.h"
#include "DMRNetwork.h"
#include "DMRControl.h"
#include "YSFNetwork.h"
#include "YSFControl.h"
#include "P25Network.h"
#include "P25Control.h"

extern const char* VERSION;

#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <ctime>

//---------------------------------------------------------
// Abstract Task Implementation
//---------------------------------------------------------
CMMDVMTask::CMMDVMTask(CMMDVMHost* host) :
m_host(host),
m_stopWatch()
{
    assert(host);
    m_stopWatch.start();
}

CMMDVMTask::~CMMDVMTask()
{
}

//---------------------------------------------------------
// DMR Task Implementation
//---------------------------------------------------------
CDMRTask::CDMRTask(CMMDVMHost* host) : 
CMMDVMTask(host),
m_dmrControl(NULL),
m_dmrNetwork(NULL),
m_dmrBeaconDurationTimer(1000U),
m_dmrBeaconIntervalTimer(1000U),
m_dmrTXTimer(1000U),
m_dmrRFModeHang(10U),
m_dmrNetModeHang(3U)
{
}

CDMRTask::~CDMRTask()
{
    if (m_dmrNetwork)
    {
        m_dmrNetwork->close();
        delete m_dmrNetwork;
    }

    if (m_dmrControl)
    {
        delete m_dmrControl;
    }
}

CDMRTask* CDMRTask::create(CMMDVMHost* host, CRSSIInterpolator* rssi)
{
    assert(host);
    assert(rssi);
    bool dmrEnabled = host->m_conf.getDMREnabled();
    if (!dmrEnabled)
        return NULL;

    CDMRTask *task = new CDMRTask(host);
    bool dmrNetEnabled = host->m_conf.getDMRNetworkEnabled();
    if (dmrNetEnabled)
        task->createDMRNetwork(); // create network always returns true
    task->createDMRControl(rssi); // create controller always returns true

    return task;
}

bool CDMRTask::createDMRNetwork()
{
    CMMDVMHost* host = m_host;
    assert(host);

	std::string address  = host->m_conf.getDMRNetworkAddress();
	unsigned int port    = host->m_conf.getDMRNetworkPort();
	unsigned int local   = host->m_conf.getDMRNetworkLocal();
	unsigned int id      = host->m_conf.getDMRId();
	std::string password = host->m_conf.getDMRNetworkPassword();
	bool debug           = host->m_conf.getDMRNetworkDebug();
	unsigned int jitter  = host->m_conf.getDMRNetworkJitter();
	bool slot1           = host->m_conf.getDMRNetworkSlot1();
	bool slot2           = host->m_conf.getDMRNetworkSlot2();
	HW_TYPE hwType       = host->m_modem->getHWType();
	m_dmrNetModeHang     = host->m_conf.getDMRNetworkModeHang();

	LogInfo("DMR Network Parameters");
	LogInfo("    Address: %s", address.c_str());
	LogInfo("    Port: %u", port);
	if (local > 0U)
		LogInfo("    Local: %u", local);
	else
		LogInfo("    Local: random");
	LogInfo("    Jitter: %ums", jitter);
	LogInfo("    Slot 1: %s", slot1 ? "enabled" : "disabled");
	LogInfo("    Slot 2: %s", slot2 ? "enabled" : "disabled");
	LogInfo("    Mode Hang: %us", m_dmrNetModeHang);

	m_dmrNetwork = new CDMRNetwork(address, port, local, id, password, host->m_duplex, VERSION, debug, slot1, slot2, hwType);

	std::string options = host->m_conf.getDMRNetworkOptions();
	if (!options.empty()) {
		LogInfo("    Options: %s", options.c_str());
		m_dmrNetwork->setOptions(options);
	}

	unsigned int rxFrequency = host->m_conf.getRXFrequency();
	unsigned int txFrequency = host->m_conf.getTXFrequency();
	unsigned int power       = host->m_conf.getPower();
	unsigned int colorCode   = host->m_conf.getDMRColorCode();
	float latitude           = host->m_conf.getLatitude();
	float longitude          = host->m_conf.getLongitude();
	int height               = host->m_conf.getHeight();
	std::string location     = host->m_conf.getLocation();
	std::string description  = host->m_conf.getDescription();
	std::string url          = host->m_conf.getURL();

	LogInfo("Info Parameters");
	LogInfo("    Callsign: %s", host->m_callsign.c_str());
	LogInfo("    RX Frequency: %uHz", rxFrequency);
	LogInfo("    TX Frequency: %uHz", txFrequency);
	LogInfo("    Power: %uW", power);
	LogInfo("    Latitude: %fdeg N", latitude);
	LogInfo("    Longitude: %fdeg E", longitude);
	LogInfo("    Height: %um", height);
	LogInfo("    Location: \"%s\"", location.c_str());
	LogInfo("    Description: \"%s\"", description.c_str());
	LogInfo("    URL: \"%s\"", url.c_str());

	m_dmrNetwork->setConfig(host->m_callsign, rxFrequency, txFrequency, power, colorCode, latitude, longitude, height, location, description, url);

	bool ret = m_dmrNetwork->open();
	if (!ret) {
		delete m_dmrNetwork;
		m_dmrNetwork = NULL;
		return false;
	}

	m_dmrNetwork->enable(true);

    return true;
}

bool CDMRTask::createDMRControl(CRSSIInterpolator* rssi)
{
    CMMDVMHost* host = m_host;
    assert(host);

    unsigned int id             = host->m_conf.getDMRId();
    unsigned int colorCode      = host->m_conf.getDMRColorCode();
    bool selfOnly               = host->m_conf.getDMRSelfOnly();
    bool embeddedLCOnly         = host->m_conf.getDMREmbeddedLCOnly();
    bool dumpTAData             = host->m_conf.getDMRDumpTAData();
    std::vector<unsigned int> prefixes  = host->m_conf.getDMRPrefixes();
    std::vector<unsigned int> blackList = host->m_conf.getDMRBlackList();
    std::vector<unsigned int> whiteList = host->m_conf.getDMRWhiteList();
    std::vector<unsigned int> slot1TGWhiteList = host->m_conf.getDMRSlot1TGWhiteList();
    std::vector<unsigned int> slot2TGWhiteList = host->m_conf.getDMRSlot2TGWhiteList();
    unsigned int callHang       = host->m_conf.getDMRCallHang();
    unsigned int txHang         = host->m_conf.getDMRTXHang();
    unsigned int jitter         = host->m_conf.getDMRNetworkJitter();
    m_dmrRFModeHang             = host->m_conf.getDMRModeHang();
    bool dmrBeacons             = host->m_conf.getDMRBeacons();

    if (txHang > m_dmrRFModeHang)
        txHang = m_dmrRFModeHang;

    if (host->m_conf.getDMRNetworkEnabled()) {
        unsigned dmrNetModeHang = host->m_conf.getDMRNetworkModeHang();
        if (txHang > dmrNetModeHang)
            txHang = dmrNetModeHang;
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

    if (dmrBeacons) {
        unsigned int dmrBeaconInterval = host->m_conf.getDMRBeaconInterval();
        unsigned int dmrBeaconDuration = host->m_conf.getDMRBeaconDuration();

        LogInfo("    DMR Roaming Beacon Interval: %us", dmrBeaconInterval);
        LogInfo("    DMR Roaming Beacon Duration: %us", dmrBeaconDuration);

        m_dmrBeaconDurationTimer.setTimeout(dmrBeaconDuration);

        m_dmrBeaconIntervalTimer.setTimeout(dmrBeaconInterval);
        m_dmrBeaconIntervalTimer.start();
    }

    m_dmrControl = new CDMRControl(id, colorCode, callHang, selfOnly, embeddedLCOnly, dumpTAData, 
            prefixes, blackList, whiteList, slot1TGWhiteList, slot2TGWhiteList, 
            host->m_timeout, host->m_modem, m_dmrNetwork, host->m_display, host->m_duplex, host->m_dmrLookup, rssi, jitter);

    m_dmrTXTimer.setTimeout(txHang);
    return true;
}

bool CDMRTask::run(CMMDVMTaskContext* ctx)
{
    if(!m_dmrControl)
        return true;

    assert(ctx->host);
    CMMDVMHost *host = ctx->host;

    unsigned char* data = (unsigned char*)ctx->data;

    // RF -> NET SLOT-1
    unsigned int len = host->m_modem->readDMRData1(data);
    if (len > 0U) {
        if (host->m_mode == MODE_IDLE) {
            if (host->m_duplex) {
                bool ret = m_dmrControl->processWakeup(data);
                if (ret) {
                    host->m_modeTimer.setTimeout(m_dmrRFModeHang);
                    host->setMode(MODE_DMR);
                    m_dmrBeaconDurationTimer.stop();
                }
            } else {
                host->m_modeTimer.setTimeout(m_dmrRFModeHang);
                host->setMode(MODE_DMR);
                m_dmrControl->writeModemSlot1(data, len);
                m_dmrBeaconDurationTimer.stop();
            }
        } else if (host->m_mode == MODE_DMR) {
            if (host->m_duplex && !host->m_modem->hasTX()) {
                bool ret = m_dmrControl->processWakeup(data);
                if (ret) {
                    host->m_modem->writeDMRStart(true);
                    m_dmrTXTimer.start();
                }
            } else {
                bool ret = m_dmrControl->writeModemSlot1(data, len);
                if (ret) {
                    m_dmrBeaconDurationTimer.stop();
                    host->m_modeTimer.start();
                    if (host->m_duplex)
                        m_dmrTXTimer.start();
                }
            }
        } else if (host->m_mode != MODE_LOCKOUT) {
            LogWarning("DMR modem data received when in mode %u", host->m_mode);
        }
    }

    // RF -> NET SLOT-2
    len = host->m_modem->readDMRData2(data);
    if (len > 0U) {
        if (host->m_mode == MODE_IDLE) {
            if (host->m_duplex) {
                bool ret = m_dmrControl->processWakeup(data);
                if (ret) {
                    host->m_modeTimer.setTimeout(m_dmrRFModeHang);
                    host->setMode(MODE_DMR);
                    m_dmrBeaconDurationTimer.stop();
                }
            } else {
                host->m_modeTimer.setTimeout(m_dmrRFModeHang);
                host->setMode(MODE_DMR);
                m_dmrControl->writeModemSlot2(data, len);
                m_dmrBeaconDurationTimer.stop();
            }
        } else if (host->m_mode == MODE_DMR) {
            if (host->m_duplex && !host->m_modem->hasTX()) {
                bool ret = m_dmrControl->processWakeup(data);
                if (ret) {
                    host->m_modem->writeDMRStart(true);
                    m_dmrTXTimer.start();
                }
            } else {
                bool ret = m_dmrControl->writeModemSlot2(data, len);
                if (ret) {
                    m_dmrBeaconDurationTimer.stop();
                    host->m_modeTimer.start();
                    if (host->m_duplex)
                        m_dmrTXTimer.start();
                }
            }
        } else if (host->m_mode != MODE_LOCKOUT) {
            LogWarning("DMR modem data received when in mode %u", host->m_mode);
        }
    }

    // NET -> RF SLOT 1
    if (host->m_modem->hasDMRSpace1() /* check if modem has buffer space to tx */) {
        len = m_dmrControl->readModemSlot1(data);
        if (len > 0U) {
            if (host->m_mode == MODE_IDLE) {
                host->m_modeTimer.setTimeout(m_dmrNetModeHang);
                host->setMode(MODE_DMR);
            }
            if (host->m_mode == MODE_DMR) {
                if (host->m_duplex) {
                    host->m_modem->writeDMRStart(true);
                    m_dmrTXTimer.start();
                }
                host->m_modem->writeDMRData1(data, len);
                m_dmrBeaconDurationTimer.stop();
                host->m_modeTimer.start();
            } else if (host->m_mode != MODE_LOCKOUT) {
                LogWarning("DMR data received when in mode %u", host->m_mode);
            }
        }
    }

    // NET -> RF SLOT 2
    if (host->m_modem->hasDMRSpace2()) {
        len = m_dmrControl->readModemSlot2(data);
        if (len > 0U) {
            if (host->m_mode == MODE_IDLE) {
                host->m_modeTimer.setTimeout(m_dmrNetModeHang);
                host->setMode(MODE_DMR);
            }
            if (host->m_mode == MODE_DMR) {
                if (host->m_duplex) {
                    host->m_modem->writeDMRStart(true);
                    m_dmrTXTimer.start();
                }
                host->m_modem->writeDMRData2(data, len);
                m_dmrBeaconDurationTimer.stop();
                host->m_modeTimer.start();
            } else if (host->m_mode != MODE_LOCKOUT) {
                LogWarning("DMR data received when in mode %u", host->m_mode);
            }
        }
    }

    unsigned int ms = m_stopWatch.elapsed();
	m_stopWatch.start();

    m_dmrControl->clock();
    if (m_dmrNetwork)
        m_dmrNetwork->clock(ms);

    m_dmrBeaconIntervalTimer.clock(ms);
    if (m_dmrBeaconIntervalTimer.isRunning() && m_dmrBeaconIntervalTimer.hasExpired()) {
    	if (host->m_mode == MODE_IDLE && !host->m_modem->hasTX()) {
    		host->setMode(MODE_DMR);
    		m_dmrBeaconIntervalTimer.start();
    		m_dmrBeaconDurationTimer.start();
    	}
    }

    m_dmrBeaconDurationTimer.clock(ms);
    if (m_dmrBeaconDurationTimer.isRunning() && m_dmrBeaconDurationTimer.hasExpired()) {
    	host->setMode(MODE_IDLE);
    	m_dmrBeaconDurationTimer.stop();
    }

    m_dmrTXTimer.clock(ms);
    if (m_dmrTXTimer.isRunning() && m_dmrTXTimer.hasExpired()) {
    	host->m_modem->writeDMRStart(false);
    	m_dmrTXTimer.stop();
    }

    return true;
}

void CDMRTask::enableNetwork(bool enabled){
    if (m_dmrNetwork != NULL)
        m_dmrNetwork->enable(enabled);    
}

void CDMRTask::startDMR(bool started)
{
    if (started){
        m_host->m_modem->writeDMRStart(true);
        m_dmrTXTimer.start();
    }else{
        m_host->m_modem->writeDMRStart(false);
	    m_dmrTXTimer.stop();
    }
}


//---------------------------------------------------------
// YSF Task Implementation
//---------------------------------------------------------
CYSFTask::CYSFTask(CMMDVMHost* host) : 
CMMDVMTask(host),
m_ysfControl(NULL),
m_ysfNetwork(NULL),
m_ysfRFModeHang(10U),
m_ysfNetModeHang(3U)
{
}

CYSFTask::~CYSFTask()
{
    if (m_ysfNetwork){
        m_ysfNetwork->close();
        delete m_ysfNetwork;
    }

    if (m_ysfControl){
        delete m_ysfControl;
    }
}

CYSFTask* CYSFTask::create(CMMDVMHost* host, CRSSIInterpolator* rssi)
{
    assert(host);
    assert(rssi);
    bool ysfEnabled = host->m_conf.getFusionEnabled();
    if (!ysfEnabled)
        return NULL;

    CYSFTask *task = new CYSFTask(host);
    bool ysfNetEnabled = host->m_conf.getFusionNetworkEnabled();
    if (ysfNetEnabled)
        task->createYSFNetwork(); // always returns true
    task->createYSFControl(rssi);     // always returns true

    return task;
}

bool CYSFTask::createYSFControl(CRSSIInterpolator* rssi)
{
    CMMDVMHost* host = m_host;
    assert(host);

    bool lowDeviation   = host->m_conf.getFusionLowDeviation();
    bool remoteGateway  = host->m_conf.getFusionRemoteGateway();
    unsigned int txHang = host->m_conf.getFusionTXHang();
    bool selfOnly       = host->m_conf.getFusionSelfOnly();
    bool sqlEnabled     = host->m_conf.getFusionSQLEnabled();
    unsigned char sql   = host->m_conf.getFusionSQL();
    m_ysfRFModeHang     = host->m_conf.getFusionModeHang();

    LogInfo("YSF RF Parameters");
    LogInfo("    Low Deviation: %s", lowDeviation ? "yes" : "no");
    LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
    LogInfo("    TX Hang: %us", txHang);
    LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
    LogInfo("    DSQ: %s", sqlEnabled ? "yes" : "no");
    if (sqlEnabled)
        LogInfo("    DSQ Value: %u", sql);
    LogInfo("    Mode Hang: %us", m_ysfRFModeHang);

    m_ysfControl = new CYSFControl(host->m_callsign, selfOnly, m_ysfNetwork, host->m_display, host->m_timeout, host->m_duplex, lowDeviation, remoteGateway, rssi);
    m_ysfControl->setSQL(sqlEnabled, sql);

    return true;
}

bool CYSFTask::createYSFNetwork()
{
    CMMDVMHost* host = m_host;
    assert(host);

	std::string myAddress  = host->m_conf.getFusionNetworkMyAddress();
	unsigned int myPort    = host->m_conf.getFusionNetworkMyPort();
	std::string gatewayAddress = host->m_conf.getFusionNetworkGatewayAddress();
	unsigned int gatewayPort   = host->m_conf.getFusionNetworkGatewayPort();
	m_ysfNetModeHang       = host->m_conf.getFusionNetworkModeHang();
	bool debug             = host->m_conf.getFusionNetworkDebug();

	LogInfo("System Fusion Network Parameters");
	LogInfo("    Local Address: %s", myAddress.c_str());
	LogInfo("    Local Port: %u", myPort);
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Mode Hang: %us", m_ysfNetModeHang);

	m_ysfNetwork = new CYSFNetwork(myAddress, myPort, gatewayAddress, gatewayPort, host->m_callsign, debug);

	bool ret = m_ysfNetwork->open();
	if (!ret) {
		delete m_ysfNetwork;
		m_ysfNetwork = NULL;
		return false;
	}

	m_ysfNetwork->enable(true);

	return true;
}

bool CYSFTask::run(CMMDVMTaskContext* ctx)
{
    if(!m_ysfControl)
        return true;

    assert(ctx->host);
    CMMDVMHost *host = ctx->host;

    unsigned char* data = (unsigned char*)ctx->data;
    bool ret;
    unsigned int len = host->m_modem->readYSFData(data);
    if (len > 0U) {
        if (host->m_mode == MODE_IDLE) {
            ret = m_ysfControl->writeModem(data, len);
            if (ret) {
                host->m_modeTimer.setTimeout(m_ysfRFModeHang);
                host->setMode(MODE_YSF);
            }
        } else if (host->m_mode == MODE_YSF) {
            m_ysfControl->writeModem(data, len);
            host->m_modeTimer.start();
        } else if (host->m_mode != MODE_LOCKOUT) {
            LogWarning("System Fusion modem data received when in mode %u", host->m_mode);
        }
    }

    ret = host->m_modem->hasYSFSpace();
    if (ret) {
        len = m_ysfControl->readModem(data);
        if (len > 0U) {
            if (host->m_mode == MODE_IDLE) {
                host->m_modeTimer.setTimeout(m_ysfNetModeHang);
                host->setMode(MODE_YSF);
            }
            if (host->m_mode == MODE_YSF) {
                host->m_modem->writeYSFData(data, len);
                host->m_modeTimer.start();
            } else if (host->m_mode != MODE_LOCKOUT) {
                LogWarning("System Fusion data received when in mode %u", host->m_mode);
            }
        }
    }

    unsigned int ms = m_stopWatch.elapsed();
	m_stopWatch.start();
    m_ysfControl->clock(ms);

    if (m_ysfNetwork != NULL)
        m_ysfNetwork->clock(ms);

    return true;
}

void CYSFTask::enableNetwork(bool enabled)
{
    if (m_ysfNetwork != NULL)
        m_ysfNetwork->enable(enabled);
}

//---------------------------------------------------------
// P25 Task Implementation
//---------------------------------------------------------
CP25Task::CP25Task(CMMDVMHost* host) :
CMMDVMTask(host),
m_p25Control(NULL),
m_p25Network(NULL),
m_p25RFModeHang(10U),
m_p25NetModeHang(3U)
{
    LogDebug("construct p25 task");
}

CP25Task::~CP25Task()
{
    LogDebug("destruct p25 task");
    if (m_p25Network){
        m_p25Network->close();
        delete m_p25Network;
    }

    if (m_p25Control){
        delete m_p25Control;
    }
}

CP25Task* CP25Task::create(CMMDVMHost* host, CRSSIInterpolator* rssi)
{
    assert(host);
    assert(rssi);
    bool p25Enabled = host->m_conf.getP25Enabled();
    if (!p25Enabled)
        return NULL;

    CP25Task *task = new CP25Task(host);
    bool p25NetEnabled = host->m_conf.getP25NetworkEnabled();
    if (p25NetEnabled)
        task->createP25Network(); // always returns true

    task->createP25Control(rssi);     // always returns true
    return task;
}

bool CP25Task::createP25Control(CRSSIInterpolator* rssi)
{
    CMMDVMHost* host = m_host;
    assert(host);

    unsigned int id    = host->m_conf.getP25Id();
    unsigned int nac   = host->m_conf.getP25NAC();
    bool uidOverride   = host->m_conf.getP25OverrideUID();
    bool selfOnly      = host->m_conf.getP25SelfOnly();
    bool remoteGateway = host->m_conf.getP25RemoteGateway();
    m_p25RFModeHang    = host->m_conf.getP25ModeHang();

    LogInfo("P25 RF Parameters");
    LogInfo("    Id: %u", id);
    LogInfo("    NAC: $%03X", nac);
    LogInfo("    UID Override: %s", uidOverride ? "yes" : "no");
    LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
    LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
    LogInfo("    Mode Hang: %us", m_p25RFModeHang);

	m_p25Control = new CP25Control(nac, id, selfOnly, uidOverride, m_p25Network, host->m_display, host->m_timeout, host->m_duplex, host->m_dmrLookup, remoteGateway, rssi);
    return true;
}

bool CP25Task::createP25Network()
{
    CMMDVMHost* host = m_host;
    assert(host);

    std::string gatewayAddress = host->m_conf.getP25GatewayAddress();
	unsigned int gatewayPort   = host->m_conf.getP25GatewayPort();
	unsigned int localPort     = host->m_conf.getP25LocalPort();
	m_p25NetModeHang           = host->m_conf.getP25NetworkModeHang();
	bool debug                 = host->m_conf.getP25NetworkDebug();

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

bool CP25Task::run(CMMDVMTaskContext* ctx)
{
    if(!m_p25Control)
        return true;

    CMMDVMHost *host = ctx->host;
    assert(host);

    unsigned char* data = (unsigned char*)ctx->data;
    bool ret;

    unsigned int len = host->m_modem->readP25Data(data);
    if (len > 0U) {
        if (host->m_mode == MODE_IDLE) {
            ret = m_p25Control->writeModem(data, len);
            if (ret) {
                host->m_modeTimer.setTimeout(m_p25RFModeHang);
                host->setMode(MODE_P25);
            }
        } else if (host->m_mode == MODE_P25) {
            m_p25Control->writeModem(data, len);
            host->m_modeTimer.start();
        } else if (host->m_mode != MODE_LOCKOUT) {
            LogWarning("P25 modem data received when in mode %u", host->m_mode);
        }
    }

    ret = host->m_modem->hasP25Space();
    if (ret) {
        len = m_p25Control->readModem(data);
        if (len > 0U) {
            if (host->m_mode == MODE_IDLE) {
                host->m_modeTimer.setTimeout(m_p25NetModeHang);
                host->setMode(MODE_P25);
            }
            if (host->m_mode == MODE_P25) {
                host->m_modem->writeP25Data(data, len);
                host->m_modeTimer.start();
            } else if (host->m_mode != MODE_LOCKOUT) {
                LogWarning("P25 data received when in mode %u", host->m_mode);
            }
        }
    }

    unsigned int ms = m_stopWatch.elapsed();
    m_stopWatch.start();
    m_p25Control->clock(ms);

    if (m_p25Network)
    {
        m_p25Network->clock(ms);
    }
    return true;
}

void CP25Task::enableNetwork(bool enabled)
{
    if (m_p25Network != NULL)
        m_p25Network->enable(enabled);
}