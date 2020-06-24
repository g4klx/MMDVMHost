/*
 *	Copyright (C) 2015-2019 Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "DMRControl.h"
#include "DMRAccessControl.h"
#include "Defines.h"
#include "DMRCSBK.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <algorithm>

CDMRControl::CDMRControl(unsigned int id, unsigned int colorCode, unsigned int callHang, bool selfOnly, bool embeddedLCOnly, bool dumpTAData, const std::vector<unsigned int>& prefixes, const std::vector<unsigned int>& blacklist, const std::vector<unsigned int>& whitelist, const std::vector<unsigned int>& slot1TGWhitelist, const std::vector<unsigned int>& slot2TGWhitelist, unsigned int timeout, CModem* modem, CDMRNetwork* network, CDisplay* display, bool duplex, CDMRLookup* lookup, CRSSIInterpolator* rssi, unsigned int jitter, DMR_OVCM_TYPES ovcm) :
m_colorCode(colorCode),
m_modem(modem),
m_network(network),
m_slot1(1U, timeout),
m_slot2(2U, timeout),
m_lookup(lookup)
{
	assert(id != 0U);
	assert(modem != NULL);
	assert(display != NULL);
	assert(lookup != NULL);
	assert(rssi != NULL);

	// Load black and white lists to DMRAccessControl
	CDMRAccessControl::init(blacklist, whitelist, slot1TGWhitelist, slot2TGWhitelist, selfOnly, prefixes, id);

	CDMRSlot::init(colorCode, embeddedLCOnly, dumpTAData, callHang, modem, network, display, duplex, m_lookup, rssi, jitter, ovcm);
}

CDMRControl::~CDMRControl()
{
}

bool CDMRControl::processWakeup(const unsigned char* data)
{
	assert(data != NULL);

	// Wakeups always come in on slot 1
	if (data[0U] != TAG_DATA || data[1U] != (DMR_IDLE_RX | DMR_SYNC_DATA | DT_CSBK))
		return false;

	CDMRCSBK csbk;
	bool valid = csbk.put(data + 2U);
	if (!valid)
		return false;

	CSBKO csbko = csbk.getCSBKO();
	if (csbko != CSBKO_BSDWNACT)
		return false;

	unsigned int srcId = csbk.getSrcId();
	std::string src = m_lookup->find(srcId);

	bool ret = CDMRAccessControl::validateSrcId(srcId);
	if (!ret) {
		LogMessage("Invalid Downlink Activate received from %s", src.c_str());
		return false;
	}

	LogMessage("Downlink Activate received from %s", src.c_str());

	return true;
}

bool CDMRControl::writeModemSlot1(unsigned char *data, unsigned int len)
{
	assert(data != NULL);

	return m_slot1.writeModem(data, len);
}

bool CDMRControl::writeModemSlot2(unsigned char *data, unsigned int len)
{
	assert(data != NULL);

	return m_slot2.writeModem(data, len);
}

unsigned int CDMRControl::readModemSlot1(unsigned char *data)
{
	assert(data != NULL);

	return m_slot1.readModem(data);
}

unsigned int CDMRControl::readModemSlot2(unsigned char *data)
{
	assert(data != NULL);

	return m_slot2.readModem(data);
}

void CDMRControl::clock()
{
	if (m_network != NULL) {
		CDMRData data;
		bool ret = m_network->read(data);
		if (ret) {
			unsigned int slotNo = data.getSlotNo();
			switch (slotNo) {
				case 1U: m_slot1.writeNetwork(data); break;
				case 2U: m_slot2.writeNetwork(data); break;
				default: LogError("Invalid slot no %u", slotNo); break;
			}
		}
	}

	m_slot1.clock();
	m_slot2.clock();
}

bool CDMRControl::isBusy() const
{
	if (m_slot1.isBusy())
		return true;

	return m_slot2.isBusy();
}

void CDMRControl::enable(bool enabled)
{
	m_slot1.enable(enabled);
	m_slot2.enable(enabled);
}
