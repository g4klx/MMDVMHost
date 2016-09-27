/*
 *	Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
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
#include "Defines.h"
#include "DMRCSBK.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <algorithm>

CDMRControl::CDMRControl(unsigned int id, unsigned int colorCode, unsigned int callHang, bool selfOnly, const std::vector<unsigned int>& prefixes, const std::vector<unsigned int>& blackList, const std::vector<unsigned int>& DstIdBlacklistSlot1RF, const std::vector<unsigned int>& DstIdWhitelistSlot1RF, const std::vector<unsigned int>& DstIdBlacklistSlot2RF, const std::vector<unsigned int>& DstIdWhitelistSlot2RF, const std::vector<unsigned int>& DstIdBlacklistSlot1NET, const std::vector<unsigned int>& DstIdWhitelistSlot1NET, const std::vector<unsigned int>& DstIdBlacklistSlot2NET, const std::vector<unsigned int>& DstIdWhitelistSlot2NET, unsigned int timeout, CModem* modem, CDMRNetwork* network, CDisplay* display, bool duplex, CDMRLookup* lookup, int rssiMultiplier, int rssiOffset, unsigned int jitter, bool TGRewriteSlot1, bool TGRewriteSlot2, bool BMAutoRewrite, bool BMRewriteReflectorVoicePrompts) :
m_id(id),
m_colorCode(colorCode),
m_selfOnly(selfOnly),
m_prefixes(prefixes),
m_blackList(blackList),
m_modem(modem),
m_network(network),
m_slot1(1U, timeout),
m_slot2(2U, timeout),
m_lookup(lookup)
{
	assert(modem != NULL);
	assert(display != NULL);
	assert(lookup != NULL);

	CDMRSlot::init(id, colorCode, callHang, selfOnly, prefixes, blackList, DstIdBlacklistSlot1RF, DstIdWhitelistSlot1RF, DstIdBlacklistSlot2RF, DstIdWhitelistSlot2RF, DstIdBlacklistSlot1NET, DstIdWhitelistSlot1NET, DstIdBlacklistSlot2NET, DstIdWhitelistSlot2NET, modem, network, display, duplex, m_lookup, rssiMultiplier, rssiOffset, jitter, TGRewriteSlot1, TGRewriteSlot2, BMAutoRewrite, BMRewriteReflectorVoicePrompts);
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
	unsigned int bsId  = csbk.getBSId();

	std::string src = m_lookup->find(srcId);

	if (m_selfOnly) {
		if (srcId != m_id) {
			LogMessage("Invalid CSBK BS_Dwn_Act received from %s", src.c_str());
			return false;
		}
	} else {
		if (std::find(m_blackList.begin(), m_blackList.end(), srcId) != m_blackList.end()) {
			LogMessage("Invalid CSBK BS_Dwn_Act received from %s", src.c_str());
			return false;
		}

		unsigned int prefix = srcId / 10000U;
		if (prefix == 0U || prefix > 999U) {
			LogMessage("Invalid CSBK BS_Dwn_Act received from %s", src.c_str());
			return false;
		}

		if (m_prefixes.size() > 0U) {
			if (std::find(m_prefixes.begin(), m_prefixes.end(), prefix) == m_prefixes.end()) {
				LogMessage("Invalid CSBK BS_Dwn_Act received from %s", src.c_str());
				return false;
			}
		}
	}

	if (bsId == 0xFFFFFFU) {
		LogMessage("CSBK BS_Dwn_Act for ANY received from %s", src.c_str());
		return true;
	} else if (bsId == m_id) {
		LogMessage("CSBK BS_Dwn_Act for %u received from %s", bsId, src.c_str());
		return true;
	}

	return false;
}

void CDMRControl::writeModemSlot1(unsigned char *data, unsigned int len)
{
	assert(data != NULL);

	m_slot1.writeModem(data, len);
}

void CDMRControl::writeModemSlot2(unsigned char *data, unsigned int len)
{
	assert(data != NULL);

	m_slot2.writeModem(data, len);
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
