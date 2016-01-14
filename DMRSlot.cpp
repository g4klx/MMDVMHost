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

#include "SlotType.h"
#include "ShortLC.h"
#include "DMRSlot.h"
#include "Defines.h"
#include "DMRSync.h"
#include "FullLC.h"
#include "CSBK.h"
#include "Utils.h"
#include "EMB.h"
#include "CRC.h"
#include "Log.h"

#include <cassert>
#include <ctime>

unsigned int      CDMRSlot::m_colorCode = 0U;
CModem*           CDMRSlot::m_modem = NULL;
CHomebrewDMRIPSC* CDMRSlot::m_network = NULL;
IDisplay*         CDMRSlot::m_display = NULL;

unsigned char*    CDMRSlot::m_idle = NULL;

FLCO              CDMRSlot::m_flco1;
unsigned char     CDMRSlot::m_id1 = 0U;
FLCO              CDMRSlot::m_flco2;
unsigned char     CDMRSlot::m_id2 = 0U;

// #define	DUMP_DMR

CDMRSlot::CDMRSlot(unsigned int slotNo, unsigned int timeout) :
m_slotNo(slotNo),
m_radioQueue(1000U),
m_networkQueue(1000U),
m_state(SS_LISTENING),
m_embeddedLC(),
m_lc(NULL),
m_seqNo(0U),
m_n(0U),
m_playoutTimer(1000U, 0U, 500U),
m_networkWatchdog(1000U, 2U),
m_timeoutTimer(1000U, timeout),
m_fp(NULL)
{
}

CDMRSlot::~CDMRSlot()
{
}

void CDMRSlot::writeModem(unsigned char *data)
{
	if (data[0U] == TAG_LOST && m_state == SS_RELAYING_RF) {
		LogMessage("DMR Slot %u, transmission lost", m_slotNo);
		writeEndOfTransmission();
		return;
	}

	if (data[0U] == TAG_LOST && m_state == SS_LATE_ENTRY) {
		m_state = SS_LISTENING;
		return;
	}

	if (m_state == SS_RELAYING_NETWORK)
		return;

	bool dataSync  = (data[1U] & DMR_SYNC_DATA)  == DMR_SYNC_DATA;
	bool audioSync = (data[1U] & DMR_SYNC_AUDIO) == DMR_SYNC_AUDIO;

	if (dataSync) {
		CSlotType slotType;
		slotType.putData(data + 2U);

		unsigned char colorCode = slotType.getColorCode();
		unsigned char dataType  = slotType.getDataType();

		if (colorCode != m_colorCode)
			return;

		if (dataType == DT_VOICE_LC_HEADER) {
			if (m_state != SS_RELAYING_RF) {
				CFullLC fullLC;
				m_lc = fullLC.decode(data + 2U, DT_VOICE_LC_HEADER);
				if (m_lc == NULL) {
					LogMessage("DMR Slot %u: unable to decode the LC", m_slotNo);
					return;
				}

				// Regenerate the LC
				fullLC.encode(*m_lc, data + 2U, DT_VOICE_LC_HEADER);

				// Regenerate the Slot Type
				slotType.getData(data + 2U);

				// Convert the Data Sync to be from the BS
				CDMRSync sync;
				sync.addSync(data + 2U, DST_BS_DATA);

				data[0U] = TAG_DATA;
				data[1U] = 0x00U;
				m_n = 0U;

				m_networkWatchdog.stop();
				m_timeoutTimer.start();

				m_seqNo = 0U;

				for (unsigned i = 0U; i < 3U; i++) {
					writeNetwork(data, DT_VOICE_LC_HEADER);
					writeRadioQueue(data);
				}

				m_state = SS_RELAYING_RF;
				setShortLC(m_slotNo, m_lc->getDstId(), m_lc->getFLCO());

				m_display->writeDMR(m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP, m_lc->getDstId());

				LogMessage("DMR Slot %u, received RF header from %u to %s%u", m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP ? "TG " : "", m_lc->getDstId());
			}
		} else if (dataType == DT_VOICE_PI_HEADER) {
			if (m_state == SS_RELAYING_RF) {
				// Regenerate the Slot Type
				slotType.getData(data + 2U);

				// Convert the Data Sync to be from the BS
				CDMRSync sync;
				sync.addSync(data + 2U, DST_BS_DATA);

				data[0U] = TAG_DATA;
				data[1U] = 0x00U;
				m_n = 0U;

				writeNetwork(data, DT_VOICE_PI_HEADER);
				writeRadioQueue(data);

				LogMessage("DMR Slot %u, received PI header", m_slotNo);
			} else {
				// Should save the PI header for after we have a valid LC
			}
		} else {
			// Ignore wakeup CSBKs
			if (dataType == DT_CSBK) {
				CCSBK csbk(data + 2U);
				CSBKO csbko = csbk.getCSBKO();
				if (csbko == CSBKO_BSDWNACT)
					return;
			}

			if (m_state == SS_RELAYING_RF) {
				unsigned char end[DMR_FRAME_LENGTH_BYTES + 2U];

				// Generate the LC
				CFullLC fullLC;
				fullLC.encode(*m_lc, end + 2U, DT_TERMINATOR_WITH_LC);

				// Generate the Slot Type
				CSlotType slotType;
				slotType.setColorCode(m_colorCode);
				slotType.setDataType(DT_TERMINATOR_WITH_LC);
				slotType.getData(end + 2U);

				// Set the Data Sync to be from the BS
				CDMRSync sync;
				sync.addSync(end + 2U, DST_BS_DATA);

				end[0U] = TAG_EOT;
				end[1U] = 0x00U;

				writeNetwork(end, DT_TERMINATOR_WITH_LC);
				writeRadioQueue(end);

				LogMessage("DMR Slot %u, received RF end of transmission", m_slotNo);

				// 480ms of idle to space things out
				for (unsigned int i = 0U; i < 8U; i++)
					writeRadioQueue(m_idle);

				writeEndOfTransmission();

				if (dataType == DT_TERMINATOR_WITH_LC)
					return;
			}

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_DATA);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, dataType);
			writeRadioQueue(data);
		}
	} else if (audioSync) {
		if (m_state == SS_RELAYING_RF) {
			// Convert the Audio Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_AUDIO);

			unsigned char fid = m_lc->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA)
				; // AMBE FEC

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;
			m_n = 0U;

			writeRadioQueue(data);
			writeNetwork(data, DT_VOICE_SYNC);
		} else if (m_state == SS_LISTENING) {
			m_state = SS_LATE_ENTRY;
		}
	} else {
		if (m_state == SS_RELAYING_RF) {
			// Regenerate the EMB
			CEMB emb;
			emb.putData(data + 2U);
			emb.setColorCode(m_colorCode);
			emb.getData(data + 2U);

			unsigned char fid = m_lc->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA)
				; // AMBE FEC

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;
			m_n++;

			writeRadioQueue(data);
			writeNetwork(data, DT_VOICE);
		} else if (m_state == SS_LATE_ENTRY) {
			// If we haven't received an LC yet, then be strict on the color code
			CEMB emb;
			emb.putData(data + 2U);
			unsigned char colorCode = emb.getColorCode();
			if (colorCode != m_colorCode)
				return;

			m_lc = m_embeddedLC.addData(data + 2U, emb.getLCSS());
			if (m_lc != NULL) {
				// Create a dummy start frame to replace the received frame
				unsigned char start[DMR_FRAME_LENGTH_BYTES + 2U];

				CDMRSync sync;
				sync.addSync(start + 2U, DST_BS_DATA);

				CFullLC fullLC;
				fullLC.encode(*m_lc, start + 2U, DT_VOICE_LC_HEADER);

				CSlotType slotType;
				slotType.setColorCode(m_colorCode);
				slotType.setDataType(DT_VOICE_LC_HEADER);
				slotType.getData(start + 2U);

				start[0U] = TAG_DATA;
				start[1U] = 0x00U;
				m_n = 0U;

				m_networkWatchdog.stop();
				m_timeoutTimer.start();

				m_seqNo = 0U;

				for (unsigned int i = 0U; i < 3U; i++) {
					writeNetwork(start, DT_VOICE_LC_HEADER);
					writeRadioQueue(start);
				}

				// Send the original audio frame out
				unsigned char fid = m_lc->getFID();
				if (fid == FID_ETSI || fid == FID_DMRA)
					; // AMBE FEC

				data[0U] = TAG_DATA;
				data[1U] = 0x00U;
				m_n++;

				writeRadioQueue(data);
				writeNetwork(data, DT_VOICE);

				m_state = SS_RELAYING_RF;

				setShortLC(m_slotNo, m_lc->getDstId(), m_lc->getFLCO());

				m_display->writeDMR(m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP, m_lc->getDstId());

				LogMessage("DMR Slot %u, received RF late entry from %u to %s%u", m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP ? "TG " : "", m_lc->getDstId());
			}
		}
	}
}

unsigned int CDMRSlot::readModem(unsigned char* data)
{
	if (m_radioQueue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_radioQueue.getData(&len, 1U);

	m_radioQueue.getData(data, len);

	return len;
}

void CDMRSlot::writeEndOfTransmission()
{
	m_state = SS_LISTENING;

	setShortLC(m_slotNo, 0U);

	m_display->clearDMR(m_slotNo);

	m_networkWatchdog.stop();
	m_timeoutTimer.stop();

	delete m_lc;
	m_lc = NULL;
}

void CDMRSlot::writeNetwork(const CDMRData& dmrData)
{
	if (m_state == SS_RELAYING_RF || m_state == SS_LATE_ENTRY)
		return;

	m_networkWatchdog.start();

	unsigned char dataType = dmrData.getDataType();

	unsigned char data[DMR_FRAME_LENGTH_BYTES + 2U];
	dmrData.getData(data + 2U);

	if (dataType == DT_VOICE_LC_HEADER) {
		if (m_state != SS_RELAYING_NETWORK) {
			CFullLC fullLC;
			m_lc = fullLC.decode(data + 2U, DT_VOICE_LC_HEADER);
			if (m_lc == NULL) {
				LogMessage("DMR Slot %u, bad LC received from the network", m_slotNo);
				return;
			}

			// Regenerate the LC
			fullLC.encode(*m_lc, data + 2U, DT_VOICE_LC_HEADER);

			// Regenerate the Slot Type
			CSlotType slotType;
			slotType.setColorCode(m_colorCode);
			slotType.setDataType(DT_VOICE_LC_HEADER);
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_DATA);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			m_playoutTimer.start();
			m_timeoutTimer.start();

			for (unsigned int i = 0U; i < 3U; i++)
				writeNetworkQueue(data);

			m_state = SS_RELAYING_NETWORK;

			setShortLC(m_slotNo, m_lc->getDstId(), m_lc->getFLCO());

			m_display->writeDMR(m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP, m_lc->getDstId());

#if defined(DUMP_DMR)
			openFile();
			writeFile(data);
#endif
			LogMessage("DMR Slot %u, received network header from %u to %s%u", m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP ? "TG " : "", m_lc->getDstId());
		}
	} else if (dataType == DT_VOICE_PI_HEADER) {
		if (m_state != SS_RELAYING_NETWORK)
			return;

		// Regenerate the Slot Type
		CSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_VOICE_PI_HEADER);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CDMRSync sync;
		sync.addSync(data + 2U, DST_BS_DATA);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		writeNetworkQueue(data);

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	} else if (dataType == DT_TERMINATOR_WITH_LC) {
		if (m_state != SS_RELAYING_NETWORK)
			return;

		// Regenerate the LC
		CFullLC fullLC;
		fullLC.encode(*m_lc, data + 2U, DT_TERMINATOR_WITH_LC);

		// Regenerate the Slot Type
		CSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_TERMINATOR_WITH_LC);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CDMRSync sync;
		sync.addSync(data + 2U, DST_BS_DATA);

		data[0U] = TAG_EOT;
		data[1U] = 0x00U;

		writeNetworkQueue(data);
		writeEndOfTransmission();

#if defined(DUMP_DMR)
		writeFile(data);
		closeFile();
#endif
		LogMessage("DMR Slot %u, received network end of transmission", m_slotNo);
	} else if (dataType == DT_VOICE_SYNC) {
		if (m_state != SS_RELAYING_NETWORK)
			return;

		// Convert the Audio Sync to be from the BS
		CDMRSync sync;
		sync.addSync(data + 2U, DST_BS_AUDIO);

		unsigned char fid = m_lc->getFID();
		if (fid == FID_ETSI || fid == FID_DMRA)
			; // AMBE FEC

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		writeNetworkQueue(data);

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	} else if (dataType == DT_VOICE) {
		if (m_state != SS_RELAYING_NETWORK)
			return;

		unsigned char fid = m_lc->getFID();
		if (fid == FID_ETSI || fid == FID_DMRA)
			; // AMBE FEC

		// Change the color code in the EMB
		CEMB emb;
		emb.putData(data + 2U);
		emb.setColorCode(m_colorCode);
		emb.getData(data + 2U);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		writeNetworkQueue(data);

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	} else {
		if (m_state != SS_RELAYING_NETWORK)
			return;

		// Change the Color Code of the Slot Type
		CSlotType slotType;
		slotType.putData(data + 2U);
		slotType.setColorCode(m_colorCode);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CDMRSync sync;
		sync.addSync(data + 2U, DST_BS_DATA);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		writeNetworkQueue(data);

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	}
}

void CDMRSlot::clock(unsigned int ms)
{
	m_timeoutTimer.clock(ms);

	m_playoutTimer.clock(ms);
	if (m_playoutTimer.isRunning() && m_playoutTimer.hasExpired()) {
		m_playoutTimer.stop();

		while (!m_networkQueue.isEmpty()) {
			unsigned char len = 0U;
			m_networkQueue.getData(&len, 1U);

			unsigned char buffer[100U];
			m_networkQueue.getData(buffer, len);

			m_radioQueue.addData(&len, 1U);
			m_radioQueue.addData(buffer, len);
		}
	}

	if (m_state == SS_RELAYING_NETWORK) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			LogMessage("DMR Slot %u, network watchdog has expired", m_slotNo);
			writeEndOfTransmission();
#if defined(DUMP_DMR)
			closeFile();
#endif
		}
	}
}

void CDMRSlot::writeRadioQueue(const unsigned char *data)
{
	unsigned char len = DMR_FRAME_LENGTH_BYTES + 2U;
	m_radioQueue.addData(&len, 1U);

	// If the timeout has expired, replace the audio with idles to keep the slot busy
	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired())
		m_radioQueue.addData(m_idle, len);
	else
		m_radioQueue.addData(data, len);
}

void CDMRSlot::writeNetworkQueue(const unsigned char *data)
{
	// If the timeout has expired, send nothing
	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired())
		return;

	if (m_playoutTimer.isRunning() && !m_playoutTimer.hasExpired()) {
		unsigned char len = DMR_FRAME_LENGTH_BYTES + 2U;
		m_networkQueue.addData(&len, 1U);
		m_networkQueue.addData(data, len);
	} else {
		unsigned char len = DMR_FRAME_LENGTH_BYTES + 2U;
		m_radioQueue.addData(&len, 1U);
		m_radioQueue.addData(data, len);
	}
}

void CDMRSlot::writeNetwork(const unsigned char* data, unsigned char dataType)
{
	assert(m_lc != NULL);

	if (m_network == NULL)
		return;

	// Don't send to the network if the timeout has expired
	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired())
		return;

	CDMRData dmrData;
	dmrData.setSlotNo(m_slotNo);
	dmrData.setDataType(dataType);
	dmrData.setSrcId(m_lc->getSrcId());
	dmrData.setDstId(m_lc->getDstId());
	dmrData.setFLCO(m_lc->getFLCO());
	dmrData.setN(m_n);
	dmrData.setSeqNo(m_seqNo);

	m_seqNo++;

	dmrData.setData(data + 2U);

	m_network->write(dmrData);
}

void CDMRSlot::init(unsigned int colorCode, CModem* modem, CHomebrewDMRIPSC* network, IDisplay* display)
{
	assert(modem != NULL);
	assert(display != NULL);

	m_colorCode = colorCode;
	m_modem     = modem;
	m_network   = network;
	m_display   = display;

	m_idle = new unsigned char[DMR_FRAME_LENGTH_BYTES + 2U];

	::memcpy(m_idle + 2U, IDLE_DATA, DMR_FRAME_LENGTH_BYTES);

	// Generate the Slot Type
	CSlotType slotType;
	slotType.setColorCode(colorCode);
	slotType.setDataType(DT_IDLE);
	slotType.getData(m_idle + 2U);

	m_idle[0U] = TAG_DATA;
	m_idle[1U] = 0x00U;
}

void CDMRSlot::setShortLC(unsigned int slotNo, unsigned int id, FLCO flco)
{
	assert(m_modem != NULL);

	switch (slotNo) {
		case 1U:
			m_id1   = 0U;
			m_flco1 = flco;
			if (id != 0U) {
				unsigned char buffer[3U];
				buffer[0U] = (id << 16) & 0xFFU;
				buffer[1U] = (id << 8)  & 0xFFU;
				buffer[2U] = (id << 0)  & 0xFFU;
				m_id1 = CCRC::crc8(buffer, 3U);
			}
			break;
		case 2U:
			m_id2   = 0U;
			m_flco2 = flco;
			if (id != 0U) {
				unsigned char buffer[3U];
				buffer[0U] = (id << 16) & 0xFFU;
				buffer[1U] = (id << 8)  & 0xFFU;
				buffer[2U] = (id << 0)  & 0xFFU;
				m_id2 = CCRC::crc8(buffer, 3U);
			}
			break;
		default:
			LogError("Invalid slot number passed to setShortLC - %u", slotNo);
			return;
	}

	unsigned char lc[5U];
	lc[0U] = 0x01U;
	lc[1U] = 0x00U;
	lc[2U] = 0x00U;
	lc[3U] = 0x00U;

	if (m_id1 != 0U) {
		lc[2U] = m_id1;
		if (m_flco1 == FLCO_GROUP)
			lc[1U] |= 0x80U;
		else
			lc[1U] |= 0x90U;
	}

	if (m_id2 != 0U) {
		lc[3U] = m_id2;
		if (m_flco2 == FLCO_GROUP)
			lc[1U] |= 0x08U;
		else
			lc[1U] |= 0x09U;
	}

	lc[4U] = CCRC::crc8(lc, 4U);

	unsigned char sLC[9U];

	CUtils::dump(1U, "Input Short LC", lc, 5U);

	CShortLC shortLC;
	shortLC.encode(lc, sLC);

	CUtils::dump(1U, "Output Short LC", sLC, 9U);

	m_modem->writeDMRShortLC(sLC);
}

bool CDMRSlot::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "DMR_%u_%04d%02d%02d_%02d%02d%02d.ambe", m_slotNo, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("DMR", 1U, 3U, m_fp);

	return true;
}

bool CDMRSlot::writeFile(const unsigned char* data)
{
	if (m_fp == NULL)
		return false;

	::fwrite(data, 1U, DMR_FRAME_LENGTH_BYTES + 2U, m_fp);

	return true;
}

void CDMRSlot::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}
