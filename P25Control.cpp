/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "P25Control.h"
#include "P25Defines.h"
#include "Sync.h"
#include "Log.h"
#include "Utils.h"

#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Control::CP25Control(unsigned int nac, CDisplay* display, unsigned int timeout, bool duplex, CDMRLookup* lookup, int rssiMultiplier, int rssiOffset) :
m_nac(nac),
m_display(display),
m_duplex(duplex),
m_lookup(lookup),
m_rssiMultiplier(rssiMultiplier),
m_rssiOffset(rssiOffset),
m_queue(1000U, "P25 Control"),
m_rfState(RS_RF_LISTENING),
m_netState(RS_NET_IDLE),
m_rfTimeout(1000U, timeout),
m_netTimeout(1000U, timeout),
m_rfFrames(0U),
m_rfBits(0U),
m_rfErrs(0U),
m_nid(),
m_audio(),
m_rfData(),
m_netData()
{
	assert(display != NULL);
	assert(lookup != NULL);
}

CP25Control::~CP25Control()
{
}

bool CP25Control::writeModem(unsigned char* data, unsigned int len)
{
	assert(data != NULL);

	CUtils::dump(1U, "P25 Data", data, len);

	bool sync = data[1U] == 0x01U;

	if (data[0U] == TAG_LOST && m_rfState == RS_RF_LISTENING)
		return false;

	if (data[0U] == TAG_LOST) {
		LogMessage("P25, transmission lost, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));
		m_display->clearP25();
		m_rfState = RS_RF_LISTENING;
		m_rfTimeout.stop();
		m_rfData.reset();
		return false;
	}

	if (!sync && m_rfState == RS_RF_LISTENING)
		return false;

	// Regenerate the NID
	m_nid.process(data + 2U);
	unsigned char duid = m_nid.getDUID();
	unsigned int nac = m_nid.getNAC();

	LogDebug("P25, DUID=$%X NAC=$%03X", duid, nac);

	if (m_rfState == RS_RF_LISTENING && nac != m_nac)
		return false;

	if (data[0U] == TAG_HEADER) {
		m_rfData.reset();

		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Regenerate Enc Data
		m_rfData.processHeader(data + 2U);

		// Add busy bits
		addBusyBits(data + 2U, P25_HDR_FRAME_LENGTH_BITS, false, true);

		m_rfFrames = 0U;
		m_rfErrs   = 0U;
		m_rfBits   = 1U;
		m_rfTimeout.start();

		if (m_duplex) {
			data[0U] = TAG_HEADER;
			data[1U] = 0x00U;
			writeQueueRF(data, P25_HDR_FRAME_LENGTH_BYTES + 2U);
		}

		LogMessage("P25, received RF header");
	} else if (duid == P25_DUID_LDU1) {
		if (m_rfState == RS_RF_LISTENING) {
			m_rfFrames = 0U;
			m_rfErrs   = 0U;
			m_rfBits   = 1U;
			m_rfTimeout.start();
		}

		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Regenerate LDU1 Data
		m_rfData.processLDU1(data + 2U);

		// Regenerate Audio
		unsigned int errors = m_audio.process(data + 2U);
		LogDebug("P25, LDU1 audio, errs: %u/1233", errors);

		m_rfBits += 1233U;
		m_rfErrs += errors;
		m_rfFrames++;

		// Add busy bits
		addBusyBits(data + 2U, P25_LDU_FRAME_LENGTH_BITS, false, true);

		if (m_duplex) {
			data[0U] = TAG_DATA;
			data[1U] = 0x00U;
			writeQueueRF(data, P25_LDU_FRAME_LENGTH_BYTES + 2U);
		}

		if (m_rfState == RS_RF_LISTENING) {
			unsigned int src = m_rfData.getSource();
			bool         grp = m_rfData.getGroup();
			unsigned int dst = m_rfData.getDest();
			std::string source = m_lookup->find(src);
			LogMessage("P25, received RF from %s to %s%u", source.c_str(), grp ? "TG" : "", dst);
			m_display->writeP25(source.c_str(), grp, dst, "R");
			m_rfState = RS_RF_AUDIO;
		}
	} else if (duid == P25_DUID_LDU2) {
		if (m_rfState == RS_RF_LISTENING)
			return false;

		// Decode LDU2
		m_rfData.processLDU2(data + 2U);

		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Regenerate LDU2 Data

		// Regenerate Audio
		unsigned int errors = m_audio.process(data + 2U);
		LogDebug("P25, LDU2 audio, errs: %u/1233", errors);

		m_rfBits += 1233U;
		m_rfErrs += errors;
		m_rfFrames++;

		// Add busy bits
		addBusyBits(data + 2U, P25_LDU_FRAME_LENGTH_BITS, false, true);

		if (m_duplex) {
			data[0U] = TAG_DATA;
			data[1U] = 0x00U;
			writeQueueRF(data, P25_LDU_FRAME_LENGTH_BYTES + 2U);
		}
	} else if (duid == P25_DUID_TERM_LC) {
		if (m_rfState == RS_RF_LISTENING)
			return false;

		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Regenerate LDU1 Data
		m_rfData.processTerminator(data + 2U);

		// Add busy bits
		addBusyBits(data + 2U, P25_TERMLC_FRAME_LENGTH_BITS, false, true);

		m_rfState = RS_RF_LISTENING;
		m_rfTimeout.stop();
		m_rfData.reset();

		LogMessage("P25, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));
		m_display->clearP25();

		if (m_duplex) {
			data[0U] = TAG_EOT;
			data[1U] = 0x00U;
			writeQueueRF(data, P25_TERMLC_FRAME_LENGTH_BYTES + 2U);
		}
	} else if (duid == P25_DUID_TERM) {
		if (m_rfState == RS_RF_LISTENING)
			return false;

		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Add busy bits
		addBusyBits(data + 2U, P25_TERM_FRAME_LENGTH_BITS, false, true);

		m_rfState = RS_RF_LISTENING;
		m_rfTimeout.stop();
		m_rfData.reset();

		LogMessage("P25, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));
		m_display->clearP25();

		if (m_duplex) {
			data[0U] = TAG_EOT;
			data[1U] = 0x00U;
			writeQueueRF(data, P25_TERM_FRAME_LENGTH_BYTES + 2U);
		}
	} else {
		return false;
	}

	return true;
}

unsigned int CP25Control::readModem(unsigned char* data)
{
	assert(data != NULL);

	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CP25Control::clock(unsigned int ms)
{
	m_rfTimeout.clock(ms);
	m_netTimeout.clock(ms);
}

void CP25Control::writeQueueRF(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	if (m_rfTimeout.isRunning() && m_rfTimeout.hasExpired())
		return;

	unsigned int space = m_queue.freeSpace();
	if (space < (length + 1U)) {
		LogError("P25, overflow in the P25 RF queue");
		return;
	}

	unsigned char len = length;
	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CP25Control::addBusyBits(unsigned char* data, unsigned int length, bool b1, bool b2)
{
	assert(data != NULL);

	for (unsigned int ss0Pos = P25_SS0_START; ss0Pos < length; ss0Pos += P25_SS_INCREMENT) {
		unsigned int ss1Pos = ss0Pos + 1U;
		WRITE_BIT(data, ss0Pos, b1);
		WRITE_BIT(data, ss1Pos, b2);
	}
}
