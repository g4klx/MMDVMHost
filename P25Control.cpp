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

#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Control::CP25Control(unsigned int id, CDisplay* display, unsigned int timeout, bool duplex, int rssiMultiplier, int rssiOffset) :
m_id(id),
m_display(display),
m_duplex(duplex),
m_rssiMultiplier(rssiMultiplier),
m_rssiOffset(rssiOffset),
m_queue(1000U, "P25 Control"),
m_rfState(RS_RF_LISTENING),
m_netState(RS_NET_IDLE),
m_rfTimeout(1000U, timeout),
m_netTimeout(1000U, timeout),
m_rfFrames(0U),
m_rfBits(0U),
m_rfErrs(0U)
{
	assert(display != NULL);
}

CP25Control::~CP25Control()
{
}

bool CP25Control::writeModem(unsigned char* data, unsigned int len)
{
	assert(data != NULL);

	bool sync = data[1U] == 0x01U;

	if (data[0U] == TAG_LOST && m_rfState == RS_RF_LISTENING)
		return false;

	if (data[0U] == TAG_LOST) {
		LogMessage("P25, transmission lost, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));
		m_rfState = RS_RF_LISTENING;
		m_rfTimeout.stop();
		return false;
	}

	if (!sync && m_rfState == RS_RF_LISTENING)
		return false;

	// Put into the NID decoder
	unsigned char duid = 0U;	// XXX

	if (data[0U] == TAG_HEADER) {
		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Regenerate NID

		// Regenerate Enc Data

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
			// Decode LDU1
		}

		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Regenerate NID

		// Regenerate LDU1 Data

		// Regenerate Audio
		unsigned int errors = 0U;	// XXX
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
			// LogMessage("P25, received RF LC from %8.8s/%4.4s to %8.8s", my1, my2, your);
			m_rfState = RS_RF_AUDIO;
		}
	} else if (duid == P25_DUID_LDU2) {
		if (m_rfState == RS_RF_LISTENING)
			return false;

		// Decode LDU2

		// Regenerate Sync
		CSync::addP25Sync(data + 2U);

		// Regenerate NID

		// Regenerate LDU2 Data

		// Regenerate Audio
		unsigned int errors = 0U;	// XXX
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

		// Regenerate NID

		// Regenerate LDU1 Data

		// Add busy bits
		addBusyBits(data + 2U, P25_TERMLC_FRAME_LENGTH_BITS, false, true);

		m_rfState = RS_RF_LISTENING;
		m_rfTimeout.stop();

		LogMessage("P25, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));

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

		// Regenerate NID

		// Add busy bits
		addBusyBits(data + 2U, P25_TERM_FRAME_LENGTH_BITS, false, true);

		m_rfState = RS_RF_LISTENING;
		m_rfTimeout.stop();

		LogMessage("P25, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));

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

	for (unsigned int i = 0U; i < length; i++) {
		if (i > 0U && (i % 70U) == 0U)
			WRITE_BIT(data, i, b1);
		if (i > 0U && (i % 71U) == 0U)
			WRITE_BIT(data, i, b2);
	}
}
