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

#include "DMRDataHeader.h"
#include "SlotType.h"
#include "ShortLC.h"
#include "DMRSlot.h"
#include "DMRSync.h"
#include "FullLC.h"
#include "Utils.h"
#include "CSBK.h"
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
m_queue(1000U),
m_state(RS_LISTENING),
m_embeddedLC(),
m_lc(NULL),
m_seqNo(0U),
m_n(0U),
m_networkWatchdog(1000U, 0U, 1500U),
m_timeoutTimer(1000U, timeout),
m_packetTimer(1000U, 0U, 100U),
m_elapsed(),
m_frames(0U),
m_blocks(0U),
m_lost(0U),
m_fec(),
m_bits(0U),
m_errs(0U),
m_lastFrame(NULL),
m_lastEMB(),
m_fp(NULL)
{
	m_lastFrame = new unsigned char[DMR_FRAME_LENGTH_BYTES + 2U];
}

CDMRSlot::~CDMRSlot()
{
	delete[] m_lastFrame;
}

void CDMRSlot::writeModem(unsigned char *data)
{
	if (data[0U] == TAG_LOST && m_state == RS_RELAYING_RF_AUDIO) {
		LogMessage("DMR Slot %u, transmission lost, BER: %u%%", m_slotNo, (m_errs * 100U) / m_bits);
		writeEndOfTransmission();
		return;
	}

	if (data[0U] == TAG_LOST && m_state == RS_RELAYING_RF_DATA) {
		LogMessage("DMR Slot %u, transmission lost", m_slotNo);
		writeEndOfTransmission();
		return;
	}

	if (data[0U] == TAG_LOST && m_state == RS_LATE_ENTRY) {
		m_state = RS_LISTENING;
		return;
	}

	if (m_state == RS_RELAYING_NETWORK_AUDIO || m_state == RS_RELAYING_NETWORK_DATA)
		return;

	bool dataSync  = (data[1U] & DMR_SYNC_DATA)  == DMR_SYNC_DATA;
	bool audioSync = (data[1U] & DMR_SYNC_AUDIO) == DMR_SYNC_AUDIO;

	if (dataSync) {
		CSlotType slotType;
		slotType.putData(data + 2U);

		unsigned char dataType = slotType.getDataType();

		if (dataType == DT_VOICE_LC_HEADER) {
			if (m_state == RS_RELAYING_RF_AUDIO)
				return;

			CFullLC fullLC;
			m_lc = fullLC.decode(data + 2U, DT_VOICE_LC_HEADER);
			if (m_lc == NULL) {
				LogMessage("DMR Slot %u: unable to decode the LC", m_slotNo);
				return;
			}

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_DATA);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			m_networkWatchdog.stop();
			m_timeoutTimer.start();

			m_seqNo = 0U;
			m_n = 0U;

			m_bits = 1U;
			m_errs = 0U;

			for (unsigned i = 0U; i < 3U; i++) {
				writeNetwork(data, DT_VOICE_LC_HEADER);
				writeQueue(data);
			}

			m_state = RS_RELAYING_RF_AUDIO;
			setShortLC(m_slotNo, m_lc->getDstId(), m_lc->getFLCO());

			m_display->writeDMR(m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP, m_lc->getDstId());

			LogMessage("DMR Slot %u, received RF voice header from %u to %s%u", m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP ? "TG " : "", m_lc->getDstId());
		} else if (dataType == DT_VOICE_PI_HEADER) {
			if (m_state != RS_RELAYING_RF_AUDIO)
				return;

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_DATA);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			m_n = 0U;

			writeNetwork(data, DT_VOICE_PI_HEADER);
			writeQueue(data);
		} else if (dataType == DT_TERMINATOR_WITH_LC) {
			if (m_state != RS_RELAYING_RF_AUDIO)
				return;

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Set the Data Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_DATA);

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;

			for (unsigned int i = 0U; i < 2U; i++)
				writeNetwork(data, DT_TERMINATOR_WITH_LC);

			// 480ms of terminator to space things out
			for (unsigned int i = 0U; i < 8U; i++)
				writeQueue(data);

			LogMessage("DMR Slot %u, received RF end of voice transmission, BER: %u%%", m_slotNo, (m_errs * 100U) / m_bits);

			writeEndOfTransmission();
		} else if (dataType == DT_DATA_HEADER) {
			if (m_state == RS_RELAYING_RF_DATA)
				return;

			CDMRDataHeader dataHeader(data + 2U);
			// if (!dataHeader.isValid()) {
			//	LogMessage("DMR Slot %u: unable to decode the data header", m_slotNo);
			//	return;
			// }

			bool gi = dataHeader.getGI();
			unsigned int srcId = dataHeader.getSrcId();
			unsigned int dstId = dataHeader.getDstId();

			m_blocks = dataHeader.getBlocks();

			m_lc = new CLC(gi ? FLCO_GROUP : FLCO_USER_USER, srcId, dstId);

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_DATA);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			m_networkWatchdog.stop();

			m_seqNo = 0U;
			m_n = 0U;

			for (unsigned i = 0U; i < 3U; i++) {
				writeNetwork(data, DT_DATA_HEADER);
				writeQueue(data);
			}

			m_state = RS_RELAYING_RF_DATA;

			setShortLC(m_slotNo, m_lc->getDstId(), gi ? FLCO_GROUP : FLCO_USER_USER);

			m_display->writeDMR(m_slotNo, srcId, gi, dstId);

			LogMessage("DMR Slot %u, received RF data header from %u to %s%u, %u blocks", m_slotNo, srcId, gi ? "TG ": "", dstId, m_blocks);
		} else if (dataType == DT_CSBK) {
			CCSBK csbk(data + 2U);
			// if (!csbk.isValid()) {
			//	LogMessage("DMR Slot %u: unable to decode the CSBK", m_slotNo);
			//	return;
			// }

			CSBKO csbko = csbk.getCSBKO();
			switch (csbko) {
			case CSBKO_BSDWNACT:
				return;

			case CSBKO_UUVREQ:
			case CSBKO_UUANSRSP:
			case CSBKO_NACKRSP:
			case CSBKO_PRECCSBK: {
					// Regenerate the Slot Type
					slotType.getData(data + 2U);

					// Convert the Data Sync to be from the BS
					CDMRSync sync;
					sync.addSync(data + 2U, DST_BS_DATA);

					data[0U] = TAG_DATA;
					data[1U] = 0x00U;

					m_seqNo = 0U;

					writeNetwork(data, DT_CSBK, FLCO_USER_USER, csbk.getSrcId(), csbk.getDstId());
					writeQueue(data);

					LogMessage("DMR Slot %u, received RF CSBK from %u to %u", m_slotNo, csbk.getSrcId(), csbk.getDstId());
				}
				break;

			default:
				LogWarning("DMR Slot %u, unhandled RF CSBK type - 0x%02X", m_slotNo, csbko);
				break;
			}
		} else if (dataType == DT_RATE_12_DATA || dataType == DT_RATE_34_DATA || dataType == DT_RATE_1_DATA) {
			if (m_state != RS_RELAYING_RF_DATA)
				return;

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_DATA);

			m_blocks--;

			data[0U] = m_blocks == 0U ? TAG_EOT : TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, dataType);
			writeQueue(data);

			if (m_blocks == 0U) {
				LogMessage("DMR Slot %u, ended RF data transmission", m_slotNo);
				writeEndOfTransmission();
			}
		} else {
			// Unhandled data type
			LogWarning("DMR Slot %u, unhandled RF data type - 0x%02X", m_slotNo, dataType);
		}
	} else if (audioSync) {
		if (m_state == RS_RELAYING_RF_AUDIO) {
			// Convert the Audio Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_AUDIO);

			unsigned char fid = m_lc->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA)
				m_errs += m_fec.regenerateDMR(data + 2U);
			m_bits += 216U;

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			m_n = 0U;

			writeQueue(data);
			writeNetwork(data, DT_VOICE_SYNC);
		} else if (m_state == RS_LISTENING) {
			m_state = RS_LATE_ENTRY;
		}
	} else {
		CEMB emb;
		emb.putData(data + 2U);

		if (m_state == RS_RELAYING_RF_AUDIO) {
			// Regenerate the EMB
			emb.setColorCode(m_colorCode);
			emb.getData(data + 2U);

			unsigned char fid = m_lc->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA)
				m_errs += m_fec.regenerateDMR(data + 2U);
			m_bits += 216U;

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			m_n++;

			writeQueue(data);
			writeNetwork(data, DT_VOICE);
		} else if (m_state == RS_LATE_ENTRY) {
			// If we haven't received an LC yet, then be strict on the color code
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

				m_networkWatchdog.stop();
				m_timeoutTimer.start();

				m_seqNo = 0U;
				m_n     = 0U;

				m_bits = 1U;
				m_errs = 0U;

				for (unsigned int i = 0U; i < 3U; i++) {
					writeNetwork(start, DT_VOICE_LC_HEADER);
					writeQueue(start);
				}

				// Regenerate the EMB
				emb.getData(data + 2U);

				// Send the original audio frame out
				unsigned char fid = m_lc->getFID();
				if (fid == FID_ETSI || fid == FID_DMRA)
					m_errs += m_fec.regenerateDMR(data + 2U);
				m_bits += 216U;

				data[0U] = TAG_DATA;
				data[1U] = 0x00U;

				m_n++;

				writeQueue(data);
				writeNetwork(data, DT_VOICE);

				m_state = RS_RELAYING_RF_AUDIO;

				setShortLC(m_slotNo, m_lc->getDstId(), m_lc->getFLCO());

				m_display->writeDMR(m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP, m_lc->getDstId());

				LogMessage("DMR Slot %u, received RF late entry from %u to %s%u", m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP ? "TG " : "", m_lc->getDstId());
			}
		}
	}
}

unsigned int CDMRSlot::readModem(unsigned char* data)
{
	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CDMRSlot::writeEndOfTransmission()
{
	m_state = RS_LISTENING;

	setShortLC(m_slotNo, 0U);

	m_display->clearDMR(m_slotNo);

	m_networkWatchdog.stop();
	m_timeoutTimer.stop();
	m_packetTimer.stop();

	delete m_lc;
	m_lc = NULL;

#if defined(DUMP_DMR)
	closeFile();
#endif
}

void CDMRSlot::writeNetwork(const CDMRData& dmrData)
{
	if (m_state == RS_RELAYING_RF_AUDIO || m_state == RS_RELAYING_RF_DATA || m_state == RS_LATE_ENTRY)
		return;

	m_networkWatchdog.start();

	unsigned char dataType = dmrData.getDataType();

	unsigned char data[DMR_FRAME_LENGTH_BYTES + 2U];
	dmrData.getData(data + 2U);

	if (dataType == DT_VOICE_LC_HEADER) {
		if (m_state == RS_RELAYING_NETWORK_AUDIO)
			return;

		CFullLC fullLC;
		m_lc = fullLC.decode(data + 2U, DT_VOICE_LC_HEADER);
		if (m_lc == NULL) {
			LogMessage("DMR Slot %u, bad LC received from the network", m_slotNo);
			return;
		}

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

		m_timeoutTimer.start();

		m_frames = 0U;

		m_bits = 1U;
		m_errs = 0U;

		for (unsigned int i = 0U; i < 3U; i++)
			writeQueue(data);

		m_state = RS_RELAYING_NETWORK_AUDIO;

		setShortLC(m_slotNo, m_lc->getDstId(), m_lc->getFLCO());

		m_display->writeDMR(m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP, m_lc->getDstId());

#if defined(DUMP_DMR)
		openFile();
		writeFile(data);
#endif
		LogMessage("DMR Slot %u, received network voice header from %u to %s%u", m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP ? "TG " : "", m_lc->getDstId());
	} else if (dataType == DT_VOICE_PI_HEADER) {
		if (m_state != RS_RELAYING_NETWORK_AUDIO)
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

		writeQueue(data);

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	} else if (dataType == DT_TERMINATOR_WITH_LC) {
		if (m_state != RS_RELAYING_NETWORK_AUDIO)
			return;

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

		// 480ms of terminator to space things out
		for (unsigned int i = 0U; i < 8U; i++)
			writeQueue(data);

		writeEndOfTransmission();

#if defined(DUMP_DMR)
		writeFile(data);
		closeFile();
#endif
		// We've received the voice header and terminator haven't we?
		m_frames += 2U;
		LogMessage("DMR Slot %u, received network end of voice transmission, %u%% packet loss, BER: %u%%", m_slotNo, (m_lost * 100U) / m_frames, (m_errs * 100U) / m_bits);
	} else if (dataType == DT_DATA_HEADER) {
		if (m_state == RS_RELAYING_NETWORK_DATA)
			return;

		CDMRDataHeader dataHeader(data + 2U);
		// if (!dataHeader.isValid()) {
		//	LogMessage("DMR Slot %u: unable to decode the data header", m_slotNo);
		//	return;
		// }

		bool gi = dataHeader.getGI();
		unsigned int srcId = dataHeader.getSrcId();
		unsigned int dstId = dataHeader.getDstId();

		m_blocks = dataHeader.getBlocks();

		m_lc = new CLC(gi ? FLCO_GROUP : FLCO_USER_USER, srcId, dstId);

		// Regenerate the Slot Type
		CSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_DATA_HEADER);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CDMRSync sync;
		sync.addSync(data + 2U, DST_BS_DATA);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		// Put a small delay into starting transmission
		writeQueue(m_idle);
		writeQueue(m_idle);

		for (unsigned i = 0U; i < 3U; i++)
			writeQueue(data);

		m_state = RS_RELAYING_NETWORK_DATA;

		setShortLC(m_slotNo, dmrData.getDstId(), gi ? FLCO_GROUP : FLCO_USER_USER);

		m_display->writeDMR(m_slotNo, dmrData.getSrcId(), gi, dmrData.getDstId());

		LogMessage("DMR Slot %u, received network data header from %u to %s%u, %u blocks", m_slotNo, dmrData.getSrcId(), gi ? "TG ": "", dmrData.getDstId(), m_blocks);
	} else if (dataType == DT_VOICE_SYNC) {
		if (m_state == RS_LISTENING) {
			m_lc = new CLC(dmrData.getFLCO(), dmrData.getSrcId(), dmrData.getDstId());

			m_timeoutTimer.start();

			// 540ms of idle to give breathing space for lost frames
			for (unsigned int i = 0U; i < 9U; i++)
				writeQueue(m_idle);

#if defined(DUMP_DMR)
			openFile();
#endif
			m_frames = 0U;

			m_bits = 1U;
			m_errs = 0U;

			m_state = RS_RELAYING_NETWORK_AUDIO;

			setShortLC(m_slotNo, m_lc->getDstId(), m_lc->getFLCO());

			m_display->writeDMR(m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP, m_lc->getDstId());

			LogMessage("DMR Slot %u, received network late entry from %u to %s%u", m_slotNo, m_lc->getSrcId(), m_lc->getFLCO() == FLCO_GROUP ? "TG " : "", m_lc->getDstId());
		}

		if (m_state == RS_RELAYING_NETWORK_AUDIO) {
			unsigned char fid = m_lc->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA)
				m_errs += m_fec.regenerateDMR(data + 2U);
			m_bits += 216U;

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			// Initialise the lost packet data
			if (m_frames == 0U) {
				::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);
				m_seqNo = dmrData.getSeqNo();
				m_n = dmrData.getN();
				m_elapsed.start();
				m_lost = 0U;
			} else {
				bool allow = insertSilence(data, dmrData.getSeqNo());
				if (!allow)
					return;
			}

			// Convert the Audio Sync to be from the BS
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_AUDIO);

			writeQueue(data);

			m_packetTimer.start();
			m_frames++;

			// Save details in case we need to infill data
			m_seqNo = dmrData.getSeqNo();
			m_n = dmrData.getN();

#if defined(DUMP_DMR)
			writeFile(data);
#endif
		}
	} else if (dataType == DT_VOICE) {
		if (m_state != RS_RELAYING_NETWORK_AUDIO)
			return;

		unsigned char fid = m_lc->getFID();
		if (fid == FID_ETSI || fid == FID_DMRA)
			m_errs += m_fec.regenerateDMR(data + 2U);
		m_bits += 216U;

		// Change the color code in the EMB
		m_lastEMB.putData(data + 2U);
		m_lastEMB.setColorCode(m_colorCode);
		m_lastEMB.getData(data + 2U);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		// Initialise the lost packet data
		if (m_frames == 0U) {
			::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);
			m_seqNo = dmrData.getSeqNo();
			m_n = dmrData.getN();
			m_elapsed.start();
			m_lost = 0U;
		} else {
			bool allow = insertSilence(data, dmrData.getSeqNo());
			if (!allow)
				return;
		}

		writeQueue(data);

		m_packetTimer.start();
		m_frames++;

		// Save details in case we need to infill data
		m_seqNo = dmrData.getSeqNo();
		m_n = dmrData.getN();

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	} else if (dataType == DT_CSBK) {
		CCSBK csbk(data + 2U);
		// if (!csbk.isValid()) {
		//	LogMessage("DMR Slot %u: unable to decode the CSBK", m_slotNo);
		//	return;
		// }

		CSBKO csbko = csbk.getCSBKO();
		switch (csbko) {
		case CSBKO_BSDWNACT:
			return;

		case CSBKO_UUVREQ:
		case CSBKO_UUANSRSP:
		case CSBKO_NACKRSP:
		case CSBKO_PRECCSBK: {
				// Regenerate the Slot Type
				CSlotType slotType;
				slotType.putData(data + 2U);
				slotType.setColorCode(m_colorCode);
				slotType.getData(data + 2U);

				// Convert the Data Sync to be from the BS
				CDMRSync sync;
				sync.addSync(data + 2U, DST_BS_DATA);

				data[0U] = TAG_DATA;
				data[1U] = 0x00U;

				writeQueue(data);

#if defined(DUMP_DMR)
				openFile();
				writeFile(data);
				closeFile();
#endif
				LogMessage("DMR Slot %u, received network CSBK from %u to %u", m_slotNo, csbk.getSrcId(), csbk.getDstId());
			}
			break;

		default:
			LogWarning("DMR Slot %u, unhandled network CSBK type - 0x%02X", m_slotNo, csbko);
			break;
		}
	} else if (dataType == DT_RATE_12_DATA || dataType == DT_RATE_34_DATA || dataType == DT_RATE_1_DATA) {
		if (m_state != RS_RELAYING_NETWORK_DATA)
			return;

		// Regenerate the Slot Type
		CSlotType slotType;
		slotType.putData(data + 2U);
		slotType.setColorCode(m_colorCode);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CDMRSync sync;
		sync.addSync(data + 2U, DST_BS_DATA);

		m_blocks--;

		data[0U] = m_blocks == 0U ? TAG_EOT : TAG_DATA;
		data[1U] = 0x00U;

#if defined(DUMP_DMR)
		writeFile(data);
#endif
		writeQueue(data);

		if (m_blocks == 0U) {
			LogMessage("DMR Slot %u, ended network data transmission", m_slotNo);
			writeEndOfTransmission();
		}
	} else {
		// Unhandled data type
		LogWarning("DMR Slot %u, unhandled network data type - 0x%02X", m_slotNo, dataType);
	}
}

void CDMRSlot::clock(unsigned int ms)
{
	m_timeoutTimer.clock(ms);

	if (m_state == RS_RELAYING_NETWORK_AUDIO || m_state == RS_RELAYING_NETWORK_DATA) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			if (m_state == RS_RELAYING_NETWORK_AUDIO) {
				// We've received the voice header haven't we?
				m_frames += 1U;
				LogMessage("DMR Slot %u, network watchdog has expired, %u%% packet loss, BER: %u%%", m_slotNo, (m_lost * 100U) / m_frames, (m_errs * 100U) / m_bits);
				writeEndOfTransmission();
#if defined(DUMP_DMR)
				closeFile();
#endif
			} else {
				LogMessage("DMR Slot %u, network watchdog has expired", m_slotNo);
				writeEndOfTransmission();
#if defined(DUMP_DMR)
				closeFile();
#endif
			}
		}
	}

	if (m_state == RS_RELAYING_NETWORK_AUDIO) {
		m_packetTimer.clock(ms);

		if (m_packetTimer.isRunning() && m_packetTimer.hasExpired()) {
			unsigned int frames = m_elapsed.elapsed() / DMR_SLOT_TIME;

			if (frames > m_frames) {
				unsigned int count = frames - m_frames;
				if (count > 2U) {
					LogMessage("DMR Slot %u, lost audio for 100ms filling in", m_slotNo);
					insertSilence(count - 1U);
				}
			}

			m_packetTimer.start();
		}
	}
}

void CDMRSlot::writeQueue(const unsigned char *data)
{
	unsigned char len = DMR_FRAME_LENGTH_BYTES + 2U;
	m_queue.addData(&len, 1U);

	// If the timeout has expired, replace the audio with idles to keep the slot busy
	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired())
		m_queue.addData(m_idle, len);
	else
		m_queue.addData(data, len);
}

void CDMRSlot::writeNetwork(const unsigned char* data, unsigned char dataType, FLCO flco, unsigned int srcId, unsigned int dstId)
{
	assert(data != NULL);

	if (m_network == NULL)
		return;

	// Don't send to the network if the timeout has expired
	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired())
		return;

	CDMRData dmrData;
	dmrData.setSlotNo(m_slotNo);
	dmrData.setDataType(dataType);
	dmrData.setSrcId(srcId);
	dmrData.setDstId(dstId);
	dmrData.setFLCO(flco);
	dmrData.setN(m_n);
	dmrData.setSeqNo(m_seqNo);

	m_seqNo++;

	dmrData.setData(data + 2U);

	m_network->write(dmrData);
}

void CDMRSlot::writeNetwork(const unsigned char* data, unsigned char dataType)
{
	assert(data != NULL);
	assert(m_lc != NULL);

	writeNetwork(data, dataType, m_lc->getFLCO(), m_lc->getSrcId(), m_lc->getDstId());
}

void CDMRSlot::init(unsigned int colorCode, CModem* modem, CHomebrewDMRIPSC* network, IDisplay* display)
{
	assert(modem != NULL);
	assert(display != NULL);

	m_colorCode = colorCode;
	m_modem     = modem;
	m_network   = network;
	m_display   = display;

	m_idle    = new unsigned char[DMR_FRAME_LENGTH_BYTES + 2U];

	::memcpy(m_idle, DMR_IDLE_DATA, DMR_FRAME_LENGTH_BYTES + 2U);

	// Generate the Slot Type for the Idle frame
	CSlotType slotType;
	slotType.setColorCode(colorCode);
	slotType.setDataType(DT_IDLE);
	slotType.getData(m_idle + 2U);
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

	CShortLC shortLC;
	shortLC.encode(lc, sLC);

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

bool CDMRSlot::insertSilence(const unsigned char* data, unsigned char seqNo)
{
	assert(data != NULL);

	// Check to see if we have any spaces to fill?
	unsigned char seq = m_seqNo + 1U;
	if (seq == seqNo) {
		// Just copy the data, nothing else to do here
		::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);
		return true;
	}

	unsigned int oldSeqNo = m_seqNo + 1U;
	unsigned int newSeqNo = seqNo;

	unsigned int count;
	if (newSeqNo > oldSeqNo)
		count = newSeqNo - oldSeqNo;
	else
		count = (256U + newSeqNo) - oldSeqNo;

	if (count > 10U) {
		LogMessage("DMR Slot %u, rejecting frame as being too old", m_slotNo);
		return false;
	}

	insertSilence(count);

	::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);

	return true;
}

void CDMRSlot::insertSilence(unsigned int count)
{
	unsigned char n = (m_n + 1U) % 6U;
	unsigned char seqNo = m_seqNo + 1U;

	for (unsigned int i = 0U; i < count; i++) {
		unsigned char data[DMR_FRAME_LENGTH_BYTES + 2U];
		::memcpy(data, m_lastFrame, DMR_FRAME_LENGTH_BYTES + 2U);

		if (n == 0U) {
			CDMRSync sync;
			sync.addSync(data + 2U, DST_BS_AUDIO);
		} else {
			// Color Code will have been set earlier
			m_lastEMB.setLCSS(0U);
			m_lastEMB.getData(data + 2U);
		}

		writeQueue(data);

		m_seqNo = seqNo;
		m_n     = n;

		m_frames++;
		m_lost++;

		seqNo++;
		n = (n + 1U) % 6U;
	}

	LogMessage("DMR Slot %u, inserted %u audio frames", m_slotNo, count);
}
