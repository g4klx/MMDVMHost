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

#include <cassert>

CDMRControl::CDMRControl(unsigned int id, unsigned int colorCode, unsigned int timeout, CModem* modem, CDMRIPSC* network, IDisplay* display, bool duplex) :
m_id(id),
m_colorCode(colorCode),
m_modem(modem),
m_network(network),
m_slot1(1U, timeout),
m_slot2(2U, timeout)
{
	assert(modem != NULL);
	assert(display != NULL);

	CDMRSlot::init(colorCode, modem, network, display, duplex);
}

CDMRControl::~CDMRControl()
{
}

bool CDMRControl::processWakeup(const unsigned char* data)
{
	// Wakeups always come in on slot 1
	if (data[0U] != TAG_DATA || data[1U] != (DMR_IDLE_RX | DMR_SYNC_DATA | DT_CSBK))
		return false;

	CDMRCSBK csbk(data + 2U);

	if (!csbk.isValid())
		return false;

	CSBKO csbko = csbk.getCSBKO();
	if (csbko != CSBKO_BSDWNACT)
		return false;

	unsigned int bsId = csbk.getBSId();
	if (bsId == 0xFFFFFFU) {
		LogMessage("CSBK BS_Dwn_Act for any received from %u", csbk.getSrcId());
		return true;
	} else if (bsId == m_id) {
		LogMessage("CSBK BS_Dwn_Act for %u received from %u", bsId, csbk.getSrcId());
		return true;
	}

	return false;
}

void CDMRControl::writeModemSlot1(unsigned char *data)
{
	m_slot1.writeModem(data);
}

void CDMRControl::writeModemSlot2(unsigned char *data)
{
	m_slot2.writeModem(data);
}

unsigned int CDMRControl::readModemSlot1(unsigned char *data)
{
	return m_slot1.readModem(data);
}

unsigned int CDMRControl::readModemSlot2(unsigned char *data)
{
	return m_slot2.readModem(data);
}

void CDMRControl::clock(unsigned int ms)
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

	m_slot1.clock(ms);
	m_slot2.clock(ms);
}
