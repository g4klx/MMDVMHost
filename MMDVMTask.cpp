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
extern const char* VERSION;

#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <ctime>

CMMDVMTask::CMMDVMTask() :
m_host(NULL),
m_stopWatch()
{
    m_stopWatch.start();
}

CMMDVMTask::~CMMDVMTask()
{
}

CDMRTask::CDMRTask() : 
CMMDVMTask(),
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
        delete m_dmrControl;
}

CDMRTask* CDMRTask::create(CMMDVMTaskContext& ctx)
{
    assert(ctx.host);
    assert(ctx.rssi);
    bool dmrEnabled = ctx.host->m_conf.getDMREnabled();
    if (!dmrEnabled)
        return NULL;

    CDMRTask *task = new CDMRTask();
    task->m_host = ctx.host;
    bool dmrNetEnabled = ctx.host->m_conf.getDMRNetworkEnabled();
    if (dmrNetEnabled)
        task->createDMRNetwork(ctx);

    task->createDMRControl(ctx);

    return task;
}

bool CDMRTask::createDMRNetwork(CMMDVMTaskContext& ctx)
{
	std::string address  = ctx.host->m_conf.getDMRNetworkAddress();
	unsigned int port    = ctx.host->m_conf.getDMRNetworkPort();
	unsigned int local   = ctx.host->m_conf.getDMRNetworkLocal();
	unsigned int id      = ctx.host->m_conf.getDMRId();
	std::string password = ctx.host->m_conf.getDMRNetworkPassword();
	bool debug           = ctx.host->m_conf.getDMRNetworkDebug();
	unsigned int jitter  = ctx.host->m_conf.getDMRNetworkJitter();
	bool slot1           = ctx.host->m_conf.getDMRNetworkSlot1();
	bool slot2           = ctx.host->m_conf.getDMRNetworkSlot2();
	HW_TYPE hwType       = ctx.host->m_modem->getHWType();
	m_dmrNetModeHang     = ctx.host->m_conf.getDMRNetworkModeHang();

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

	m_dmrNetwork = new CDMRNetwork(address, port, local, id, password, ctx.host->m_duplex, VERSION, debug, slot1, slot2, hwType);

	std::string options = ctx.host->m_conf.getDMRNetworkOptions();
	if (!options.empty()) {
		LogInfo("    Options: %s", options.c_str());
		m_dmrNetwork->setOptions(options);
	}

	unsigned int rxFrequency = ctx.host->m_conf.getRXFrequency();
	unsigned int txFrequency = ctx.host->m_conf.getTXFrequency();
	unsigned int power       = ctx.host->m_conf.getPower();
	unsigned int colorCode   = ctx.host->m_conf.getDMRColorCode();
	float latitude           = ctx.host->m_conf.getLatitude();
	float longitude          = ctx.host->m_conf.getLongitude();
	int height               = ctx.host->m_conf.getHeight();
	std::string location     = ctx.host->m_conf.getLocation();
	std::string description  = ctx.host->m_conf.getDescription();
	std::string url          = ctx.host->m_conf.getURL();

	LogInfo("Info Parameters");
	LogInfo("    Callsign: %s", ctx.host->m_callsign.c_str());
	LogInfo("    RX Frequency: %uHz", rxFrequency);
	LogInfo("    TX Frequency: %uHz", txFrequency);
	LogInfo("    Power: %uW", power);
	LogInfo("    Latitude: %fdeg N", latitude);
	LogInfo("    Longitude: %fdeg E", longitude);
	LogInfo("    Height: %um", height);
	LogInfo("    Location: \"%s\"", location.c_str());
	LogInfo("    Description: \"%s\"", description.c_str());
	LogInfo("    URL: \"%s\"", url.c_str());

	m_dmrNetwork->setConfig(ctx.host->m_callsign, rxFrequency, txFrequency, power, colorCode, latitude, longitude, height, location, description, url);

	bool ret = m_dmrNetwork->open();
	if (!ret) {
		delete m_dmrNetwork;
		m_dmrNetwork = NULL;
		return false;
	}

	m_dmrNetwork->enable(true);

    return true;
}

bool CDMRTask::createDMRControl(CMMDVMTaskContext& ctx)
{
    unsigned int id             = ctx.host->m_conf.getDMRId();
    unsigned int colorCode      = ctx.host->m_conf.getDMRColorCode();
    bool selfOnly               = ctx.host->m_conf.getDMRSelfOnly();
    bool embeddedLCOnly         = ctx.host->m_conf.getDMREmbeddedLCOnly();
    bool dumpTAData             = ctx.host->m_conf.getDMRDumpTAData();
    std::vector<unsigned int> prefixes  = ctx.host->m_conf.getDMRPrefixes();
    std::vector<unsigned int> blackList = ctx.host->m_conf.getDMRBlackList();
    std::vector<unsigned int> whiteList = ctx.host->m_conf.getDMRWhiteList();
    std::vector<unsigned int> slot1TGWhiteList = ctx.host->m_conf.getDMRSlot1TGWhiteList();
    std::vector<unsigned int> slot2TGWhiteList = ctx.host->m_conf.getDMRSlot2TGWhiteList();
    unsigned int callHang       = ctx.host->m_conf.getDMRCallHang();
    unsigned int txHang         = ctx.host->m_conf.getDMRTXHang();
    unsigned int jitter         = ctx.host->m_conf.getDMRNetworkJitter();
    m_dmrRFModeHang  = ctx.host->m_conf.getDMRModeHang();
    bool dmrBeacons             = ctx.host->m_conf.getDMRBeacons();

    if (txHang > m_dmrRFModeHang)
        txHang = m_dmrRFModeHang;

    if (ctx.host->m_conf.getDMRNetworkEnabled()) {
        unsigned dmrNetModeHang = ctx.host->m_conf.getDMRNetworkModeHang();
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
        unsigned int dmrBeaconInterval = ctx.host->m_conf.getDMRBeaconInterval();
        unsigned int dmrBeaconDuration = ctx.host->m_conf.getDMRBeaconDuration();

        LogInfo("    DMR Roaming Beacon Interval: %us", dmrBeaconInterval);
        LogInfo("    DMR Roaming Beacon Duration: %us", dmrBeaconDuration);

        m_dmrBeaconDurationTimer.setTimeout(dmrBeaconDuration);

        m_dmrBeaconIntervalTimer.setTimeout(dmrBeaconInterval);
        m_dmrBeaconIntervalTimer.start();
    }

    m_dmrControl = new CDMRControl(id, colorCode, callHang, selfOnly, embeddedLCOnly, dumpTAData, 
            prefixes, blackList, whiteList, slot1TGWhiteList, slot2TGWhiteList, 
            ctx.host->m_timeout, ctx.host->m_modem, m_dmrNetwork, ctx.host->m_display, ctx.host->m_duplex, ctx.host->m_dmrLookup, ctx.rssi, jitter);

    m_dmrTXTimer.setTimeout(txHang);
    return true;
}

bool CDMRTask::run(CMMDVMTaskContext* ctx)
{
    if(!m_dmrControl)
        return true;

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
    if (!m_dmrNetwork)
        return;

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
