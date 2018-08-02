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
#include "DStarNetwork.h"
#include "DStarControl.h"
#include "NXDNNetwork.h"
#include "NXDNControl.h"

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
}

CP25Task::~CP25Task()
{
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


//---------------------------------------------------------
// DStar Task Implementation
//---------------------------------------------------------
CDStarTask::CDStarTask(CMMDVMHost* host) :
CMMDVMTask(host),
m_dstarControl(NULL),
m_dstarNetwork(NULL),
m_dstarRFModeHang(10U),
m_dstarNetModeHang(3U)
{
}

CDStarTask::~CDStarTask()
{
    if (m_dstarNetwork){
        m_dstarNetwork->close();
        delete m_dstarNetwork;
    }

    if (m_dstarControl){
        delete m_dstarControl;
    }
}

CDStarTask* CDStarTask::create(CMMDVMHost* host, CRSSIInterpolator* rssi)
{
    assert(host);
    assert(rssi);
    bool dstarEnabled = host->m_conf.getDStarEnabled();
    if (!dstarEnabled)
        return NULL;

    CDStarTask *task = new CDStarTask(host);
    bool dstarNetEnabled = host->m_conf.getDStarNetworkEnabled();
    if (dstarNetEnabled)
        task->createDStarNetwork();     // always returns true

    task->createDStarControl(rssi);     // always returns true
    return task;
}

bool CDStarTask::createDStarControl(CRSSIInterpolator* rssi)
{
    CMMDVMHost* host = m_host;
    assert(host);

    std::string module                 = host->m_conf.getDStarModule();
    bool selfOnly                      = host->m_conf.getDStarSelfOnly();
    std::vector<std::string> blackList = host->m_conf.getDStarBlackList();
    bool ackReply                      = host->m_conf.getDStarAckReply();
    unsigned int ackTime               = host->m_conf.getDStarAckTime();
    bool errorReply                    = host->m_conf.getDStarErrorReply();
    bool remoteGateway                 = host->m_conf.getDStarRemoteGateway();
    m_dstarRFModeHang                  = host->m_conf.getDStarModeHang();

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

    m_dstarControl = new CDStarControl(host->m_callsign, module, selfOnly, ackReply, ackTime, errorReply, blackList, m_dstarNetwork, host->m_display, host->m_timeout, host->m_duplex, remoteGateway, rssi);
    return true;
}

bool CDStarTask::createDStarNetwork()
{
    CMMDVMHost* host = m_host;
    assert(host);

	std::string gatewayAddress = host->m_conf.getDStarGatewayAddress();
	unsigned int gatewayPort   = host->m_conf.getDStarGatewayPort();
	unsigned int localPort     = host->m_conf.getDStarLocalPort();
	bool debug                 = host->m_conf.getDStarNetworkDebug();
	m_dstarNetModeHang         = host->m_conf.getDStarNetworkModeHang();

	LogInfo("D-Star Network Parameters");
	LogInfo("    Gateway Address: %s", gatewayAddress.c_str());
	LogInfo("    Gateway Port: %u", gatewayPort);
	LogInfo("    Local Port: %u", localPort);
	LogInfo("    Mode Hang: %us", m_dstarNetModeHang);

	m_dstarNetwork = new CDStarNetwork(gatewayAddress, gatewayPort, localPort, host->m_duplex, VERSION, debug);

	bool ret = m_dstarNetwork->open();
	if (!ret) {
		delete m_dstarNetwork;
		m_dstarNetwork = NULL;
		return false;
	}

	m_dstarNetwork->enable(true);

	return true;
}

bool CDStarTask::run(CMMDVMTaskContext* ctx)
{
    if(!m_dstarControl)
        return true;

    CMMDVMHost *host = ctx->host;
    assert(host);

    unsigned char* data = (unsigned char*)ctx->data;
    bool ret;
    unsigned int len;
    
    len = host->m_modem->readDStarData(data);
    if (len > 0U) {
        if (host->m_mode == MODE_IDLE) {
            ret = m_dstarControl->writeModem(data, len);
            if (ret) {
                host->m_modeTimer.setTimeout(m_dstarRFModeHang);
                host->setMode(MODE_DSTAR);
            }
        } else if (host->m_mode == MODE_DSTAR) {
            m_dstarControl->writeModem(data, len);
            host->m_modeTimer.start();
        } else if (host->m_mode != MODE_LOCKOUT) {
            LogWarning("D-Star modem data received when in mode %u", host->m_mode);
        }
    }

    ret = host->m_modem->hasDStarSpace();
    if (ret) {
        len = m_dstarControl->readModem(data);
        if (len > 0U) {
            if (host->m_mode == MODE_IDLE) {
                host->m_modeTimer.setTimeout(m_dstarNetModeHang);
                host->setMode(MODE_DSTAR);
            }
            if (host->m_mode == MODE_DSTAR) {
                host->m_modem->writeDStarData(data, len);
                host->m_modeTimer.start();
            } else if (host->m_mode != MODE_LOCKOUT) {
                LogWarning("D-Star data received when in mode %u", host->m_mode);
            }
        }
    }

    unsigned int ms = m_stopWatch.elapsed();
    m_stopWatch.start();
    
    m_dstarControl->clock();

    if (m_dstarNetwork){
        m_dstarNetwork->clock(ms);
    }

    return true;
}

void CDStarTask::enableNetwork(bool enabled)
{
    if (m_dstarNetwork != NULL)
        m_dstarNetwork->enable(enabled);
}


//---------------------------------------------------------
// NXDN Task Implementation
//---------------------------------------------------------
CNXDNTask::CNXDNTask(CMMDVMHost* host) :
CMMDVMTask(host),
m_nxdnControl(NULL),
m_nxdnNetwork(NULL),
m_nxdnRFModeHang(10U),
m_nxdnNetModeHang(3U)
{
}

CNXDNTask::~CNXDNTask()
{
    if (m_nxdnNetwork){
        m_nxdnNetwork->close();
        delete m_nxdnNetwork;
    }
    delete m_nxdnControl;
}

CNXDNTask* CNXDNTask::create(CMMDVMHost* host, CRSSIInterpolator* rssi)
{
    assert(host);
    assert(rssi);
    bool nxdnEnabled = host->m_conf.getNXDNEnabled();
    if (!nxdnEnabled)
        return NULL;

    CNXDNTask *task = new CNXDNTask(host);
    bool nxdnNetEnabled = host->m_conf.getNXDNNetworkEnabled();
    if (nxdnNetEnabled)
        task->createNXDNNetwork();     // always returns true

    task->createNXDNControl(rssi);     // always returns true
    return task;
}

bool CNXDNTask::createNXDNControl(CRSSIInterpolator* rssi)
{
    CMMDVMHost* host = m_host;
    assert(host);

    unsigned int id    = host->m_conf.getNXDNId();
    unsigned int ran   = host->m_conf.getNXDNRAN();
    bool selfOnly      = host->m_conf.getNXDNSelfOnly();
    bool remoteGateway = host->m_conf.getNXDNRemoteGateway();
    m_nxdnRFModeHang   = host->m_conf.getNXDNModeHang();

    LogInfo("NXDN RF Parameters");
    LogInfo("    Id: %u", id);
    LogInfo("    RAN: %u", ran);
    LogInfo("    Self Only: %s", selfOnly ? "yes" : "no");
    LogInfo("    Remote Gateway: %s", remoteGateway ? "yes" : "no");
    LogInfo("    Mode Hang: %us", m_nxdnRFModeHang);

    m_nxdnControl = new CNXDNControl(ran, id, selfOnly, m_nxdnNetwork, host->m_display, host->m_timeout, host->m_duplex, remoteGateway, host->m_nxdnLookup, rssi);
    return true;
}

bool CNXDNTask::createNXDNNetwork()
{
    CMMDVMHost* host = m_host;
    assert(host);

	std::string gatewayAddress = host->m_conf.getNXDNGatewayAddress();
	unsigned int gatewayPort   = host->m_conf.getNXDNGatewayPort();
	std::string localAddress   = host->m_conf.getNXDNLocalAddress();
	unsigned int localPort     = host->m_conf.getNXDNLocalPort();
	m_nxdnNetModeHang          = host->m_conf.getNXDNNetworkModeHang();
	bool debug                 = host->m_conf.getNXDNNetworkDebug();

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

bool CNXDNTask::run(CMMDVMTaskContext* ctx)
{
    if(!m_nxdnControl)
        return true;

    CMMDVMHost *host = ctx->host;
    assert(host);

    unsigned char* data = (unsigned char*)ctx->data;
    bool ret;
    unsigned int len;
    
    // polling modem and write to network
    len = host->m_modem->readNXDNData(data);
    if (len > 0U) {
        if (host->m_mode == MODE_IDLE) {
            ret = m_nxdnControl->writeModem(data, len);
            if (ret) {
                host->m_modeTimer.setTimeout(m_nxdnRFModeHang);
                host->setMode(MODE_NXDN);
            }
        } else if (host->m_mode == MODE_NXDN) {
            m_nxdnControl->writeModem(data, len);
            host->m_modeTimer.start();
        } else if (host->m_mode != MODE_LOCKOUT) {
            LogWarning("NXDN modem data received when in mode %u", host->m_mode);
        }
    }

    // polling network and write to modem
    ret = host->m_modem->hasNXDNSpace();
    if (ret) {
        len = m_nxdnControl->readModem(data);
        if (len > 0U) {
            if (host->m_mode == MODE_IDLE) {
                host->m_modeTimer.setTimeout(m_nxdnNetModeHang);
                host->setMode(MODE_NXDN);
            }
            if (host->m_mode == MODE_NXDN) {
                host->m_modem->writeNXDNData(data, len);
                host->m_modeTimer.start();
            } else if (host->m_mode != MODE_LOCKOUT) {
                LogWarning("NXDN data received when in mode %u", host->m_mode);
            }
        }
    }

    unsigned int ms = m_stopWatch.elapsed();
    m_stopWatch.start();

    m_nxdnControl->clock(ms);
    if (m_nxdnNetwork){
        m_nxdnNetwork->clock(ms);
    }

    return true;
}

void CNXDNTask::enableNetwork(bool enabled)
{
    if (m_nxdnNetwork != NULL)
        m_nxdnNetwork->enable(enabled);
}