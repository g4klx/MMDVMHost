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

#include "DMRSlotType.h"
#include "DMRShortLC.h"
#include "DMRFullLC.h"
#include "BPTC19696.h"
#include "DMRSlot.h"
#include "DMRCSBK.h"
#include "Utils.h"
#include "Sync.h"
#include "CRC.h"
#include "Log.h"

#include <cassert>
#include <ctime>
#include <algorithm>

unsigned int   CDMRSlot::m_id = 0U;
unsigned int   CDMRSlot::m_colorCode = 0U;
bool           CDMRSlot::m_selfOnly = false;
std::vector<unsigned int> CDMRSlot::m_prefixes;
std::vector<unsigned int> CDMRSlot::m_blackList;
CModem*        CDMRSlot::m_modem = NULL;
CDMRIPSC*      CDMRSlot::m_network = NULL;
IDisplay*      CDMRSlot::m_display = NULL;
bool           CDMRSlot::m_duplex = true;
CDMRLookup*    CDMRSlot::m_lookup = NULL;

unsigned char* CDMRSlot::m_idle = NULL;

FLCO           CDMRSlot::m_flco1;
unsigned char  CDMRSlot::m_id1 = 0U;
bool           CDMRSlot::m_voice1 = true;
FLCO           CDMRSlot::m_flco2;
unsigned char  CDMRSlot::m_id2 = 0U;
bool           CDMRSlot::m_voice2 = true;

// #define	DUMP_DMR

CDMRSlot::CDMRSlot(unsigned int slotNo, unsigned int timeout) :
m_slotNo(slotNo),
m_queue(2000U, "DMR Slot"),
m_rfState(RS_RF_LISTENING),
m_netState(RS_NET_IDLE),
m_rfEmbeddedLC(),
m_netEmbeddedLC(),
m_rfLC(NULL),
m_netLC(NULL),
m_rfDataHeader(),
m_netDataHeader(),
m_rfSeqNo(0U),
m_netSeqNo(0U),
m_rfN(0U),
m_netN(0U),
m_networkWatchdog(1000U, 0U, 1500U),
m_rfTimeoutTimer(1000U, timeout),
m_netTimeoutTimer(1000U, timeout),
m_packetTimer(1000U, 0U, 300U),
m_interval(),
m_elapsed(),
m_rfFrames(0U),
m_netFrames(0U),
m_netLost(0U),
m_fec(),
m_rfBits(0U),
m_netBits(0U),
m_rfErrs(0U),
m_netErrs(0U),
m_lastFrame(NULL),
m_lastEMB(),
m_fp(NULL)
{
	m_lastFrame = new unsigned char[DMR_FRAME_LENGTH_BYTES + 2U];

	m_interval.start();
}

CDMRSlot::~CDMRSlot()
{
	delete[] m_lastFrame;
}

void CDMRSlot::writeModem(unsigned char *data)
{
	assert(data != NULL);

	if (data[0U] == TAG_LOST && m_rfState == RS_RF_AUDIO) {
		if (m_rfBits == 0U) m_rfBits = 1U;
		LogMessage("DMR Slot %u, RF transmission lost, %.1f seconds, BER: %.1f%%", m_slotNo, float(m_rfFrames) / 16.667F, float(m_rfErrs * 100U) / float(m_rfBits));
		writeEndRF(true);
		return;
	}

	if (data[0U] == TAG_LOST && m_rfState == RS_RF_DATA) {
		LogMessage("DMR Slot %u, RF transmission lost", m_slotNo);
		writeEndRF();
		return;
	}

	if (data[0U] == TAG_LOST) {
		m_rfState = RS_RF_LISTENING;
		return;
	}

	bool dataSync  = (data[1U] & DMR_SYNC_DATA)  == DMR_SYNC_DATA;
	bool audioSync = (data[1U] & DMR_SYNC_AUDIO) == DMR_SYNC_AUDIO;

	if (dataSync) {
		CDMRSlotType slotType;
		slotType.putData(data + 2U);

		unsigned char dataType = slotType.getDataType();

		if (dataType == DT_VOICE_LC_HEADER) {
			if (m_rfState == RS_RF_AUDIO)
				return;

			CDMRFullLC fullLC;
			CDMRLC* lc = fullLC.decode(data + 2U, DT_VOICE_LC_HEADER);
			if (lc == NULL) {
				LogMessage("DMR Slot %u, unable to decode the RF LC", m_slotNo);
				return;
			}

			unsigned int id = lc->getSrcId();
			if (!validateId(id)) {
				LogMessage("DMR Slot %u, invalid access atemmpt from %u", m_slotNo, id);
				delete lc;
				return;
			}

			m_rfLC = lc;

			// Store the LC for the embedded LC
			m_rfEmbeddedLC.setData(*m_rfLC);

			// Regenerate the LC data
			fullLC.encode(*m_rfLC, data + 2U, DT_VOICE_LC_HEADER);

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CSync::addDMRDataSync(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			m_rfTimeoutTimer.start();

			m_rfFrames = 0U;
			m_rfSeqNo  = 0U;
			m_rfBits   = 1U;
			m_rfErrs   = 0U;

			if (m_duplex) {
				writeQueueRF(data);
				writeQueueRF(data);
			}

			writeNetworkRF(data, DT_VOICE_LC_HEADER);

			m_rfState = RS_RF_AUDIO;

			std::string src = m_lookup->find(id);
			std::string dst = m_lookup->find(m_rfLC->getDstId());

			if (m_netState == RS_NET_IDLE) {
				setShortLC(m_slotNo, m_rfLC->getDstId(), m_rfLC->getFLCO(), true);
				m_display->writeDMR(m_slotNo, src, m_rfLC->getFLCO() == FLCO_GROUP, dst, "R");
			}

			LogMessage("DMR Slot %u, received RF voice header from %s to %s%s", m_slotNo, src.c_str(), m_rfLC->getFLCO() == FLCO_GROUP ? "TG " : "", dst.c_str());
		} else if (dataType == DT_VOICE_PI_HEADER) {
			if (m_rfState != RS_RF_AUDIO)
				return;

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CSync::addDMRDataSync(data + 2U);

			// Regenerate the payload
			CBPTC19696 bptc;
			unsigned char payload[12U];
			bptc.decode(data + 2U, payload);
			bptc.encode(payload, data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			if (m_duplex)
				writeQueueRF(data);

			writeNetworkRF(data, DT_VOICE_PI_HEADER);
		} else if (dataType == DT_TERMINATOR_WITH_LC) {
			if (m_rfState != RS_RF_AUDIO)
				return;

			// Regenerate the LC data
			CDMRFullLC fullLC;
			fullLC.encode(*m_rfLC, data + 2U, DT_TERMINATOR_WITH_LC);

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Set the Data Sync to be from the BS
			CSync::addDMRDataSync(data + 2U);

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;

			writeNetworkRF(data, DT_TERMINATOR_WITH_LC);
			writeNetworkRF(data, DT_TERMINATOR_WITH_LC);

			// 480ms of terminator to space things out
			if (m_duplex) {
				for (unsigned int i = 0U; i < 8U; i++)
					writeQueueRF(data);
			}

			if (m_rfBits == 0U) m_rfBits = 1U;
			LogMessage("DMR Slot %u, received RF end of voice transmission, %.1f seconds, BER: %.1f%%", m_slotNo, float(m_rfFrames) / 16.667F, float(m_rfErrs * 100U) / float(m_rfBits));

			writeEndRF();
		} else if (dataType == DT_DATA_HEADER) {
			if (m_rfState == RS_RF_DATA)
				return;

			CDMRDataHeader dataHeader;
			bool valid = dataHeader.put(data + 2U);
			if (!valid) {
				LogMessage("DMR Slot %u, unable to decode the RF data header", m_slotNo);
				return;
			}

			bool gi = dataHeader.getGI();
			unsigned int srcId = dataHeader.getSrcId();
			unsigned int dstId = dataHeader.getDstId();

			if (!validateId(srcId)) {
				LogMessage("DMR Slot %u, invalid access atemmpt from %u", m_slotNo, srcId);
				return;
			}

			m_rfFrames = dataHeader.getBlocks();

			m_rfDataHeader = dataHeader;

			m_rfSeqNo  = 0U;

			m_rfLC = new CDMRLC(gi ? FLCO_GROUP : FLCO_USER_USER, srcId, dstId);

			// Regenerate the data header
			dataHeader.get(data + 2U);

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CSync::addDMRDataSync(data + 2U);

			data[0U] = m_rfFrames == 0U ? TAG_EOT : TAG_DATA;
			data[1U] = 0x00U;

			if (m_duplex)
				writeQueueRF(data);

			writeNetworkRF(data, DT_DATA_HEADER);

			m_rfState = RS_RF_DATA;

			std::string src = m_lookup->find(srcId);
			std::string dst = m_lookup->find(dstId);

			if (m_netState == RS_NET_IDLE) {
				setShortLC(m_slotNo, dstId, gi ? FLCO_GROUP : FLCO_USER_USER, false);
				m_display->writeDMR(m_slotNo, src, gi, dst, "R");
			}

			LogMessage("DMR Slot %u, received RF data header from %s to %s%s, %u blocks", m_slotNo, src.c_str(), gi ? "TG ": "", dst.c_str(), m_rfFrames);

			if (m_rfFrames == 0U)
				endOfRFData();
		} else if (dataType == DT_CSBK) {
			CDMRCSBK csbk;
			bool valid = csbk.put(data + 2U);
			if (!valid) {
				LogMessage("DMR Slot %u, unable to decode the RF CSBK", m_slotNo);
				return;
			}

			CSBKO csbko = csbk.getCSBKO();
			if (csbko == CSBKO_BSDWNACT)
				return;

			bool gi = csbk.getGI();
			unsigned int srcId = csbk.getSrcId();
			unsigned int dstId = csbk.getDstId();

			if (!validateId(srcId)) {
				LogMessage("DMR Slot %u, invalid access atemmpt from %u", m_slotNo, srcId);
				return;
			}

			// Regenerate the CSBK data
			csbk.get(data + 2U);

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CSync::addDMRDataSync(data + 2U);

			m_rfSeqNo = 0U;

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			if (m_duplex)
				writeQueueRF(data);

			writeNetworkRF(data, DT_CSBK, gi ? FLCO_GROUP : FLCO_USER_USER, srcId, dstId);

			switch (csbko) {
			case CSBKO_UUVREQ:
				LogMessage("DMR Slot %u, received RF Unit to Unit Voice Service Request CSBK from %u to %u", m_slotNo, srcId, dstId);
				break;
			case CSBKO_UUANSRSP:
				LogMessage("DMR Slot %u, received RF Unit to Unit Voice Service Answer Response CSBK from %u to %u", m_slotNo, srcId, dstId);
				break;
			case CSBKO_NACKRSP:
				LogMessage("DMR Slot %u, received RF Negative Acknowledgment Response CSBK from %u to %u", m_slotNo, srcId, dstId);
				break;
			case CSBKO_PRECCSBK:
				LogMessage("DMR Slot %u, received RF Preamble CSBK from %u to %s%u", m_slotNo, srcId, gi ? "TG" : "", dstId);
				break;
			default:
				LogWarning("DMR Slot %u, unhandled RF CSBK type - 0x%02X", m_slotNo, csbko);
				break;
			}
		} else if (dataType == DT_RATE_12_DATA || dataType == DT_RATE_34_DATA || dataType == DT_RATE_1_DATA) {
			if (m_rfState != RS_RF_DATA || m_rfFrames == 0U)
				return;

			// Regenerate the rate 1/2 payload
			if (dataType == DT_RATE_12_DATA) {
				CBPTC19696 bptc;
				unsigned char payload[12U];
				bptc.decode(data + 2U, payload);
				bptc.encode(payload, data + 2U);
			}

			// Regenerate the Slot Type
			slotType.getData(data + 2U);

			// Convert the Data Sync to be from the BS
			CSync::addDMRDataSync(data + 2U);

			m_rfFrames--;

			data[0U] = m_rfFrames == 0U ? TAG_EOT : TAG_DATA;
			data[1U] = 0x00U;

			if (m_duplex)
				writeQueueRF(data);

			writeNetworkRF(data, dataType);

			if (m_rfFrames == 0U)
				endOfRFData();
		} else {
			// Unhandled data type
			LogWarning("DMR Slot %u, unhandled RF data type - 0x%02X", m_slotNo, dataType);
		}
	} else if (audioSync) {
		if (m_rfState == RS_RF_AUDIO) {
			// Convert the Audio Sync to be from the BS
			CSync::addDMRAudioSync(data + 2U);

			unsigned int errors = 0U;
			unsigned char fid = m_rfLC->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA) {
				errors = m_fec.regenerateDMR(data + 2U);
				// LogDebug("DMR Slot %u, audio sequence no. 0, errs: %u/141", m_slotNo, errors);
				m_rfErrs += errors;
			}

			m_rfBits += 141U;

			m_rfFrames++;

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			if (m_duplex)
				writeQueueRF(data);

			writeNetworkRF(data, DT_VOICE_SYNC, errors);
		} else if (m_rfState == RS_RF_LISTENING) {
			m_rfState = RS_RF_LATE_ENTRY;
		}
	} else {
		if (m_rfState == RS_RF_AUDIO) {
			m_rfN = data[1U] & 0x0FU;

			// Regenerate the embedded LC
			unsigned char lcss = m_rfEmbeddedLC.getData(data + 2U, m_rfN);

			CDMREMB emb;
			emb.putData(data + 2U);

			// Regenerate the EMB
			emb.setColorCode(m_colorCode);
			emb.setLCSS(lcss);
			emb.getData(data + 2U);

			unsigned int errors = 0U;
			unsigned char fid = m_rfLC->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA) {
				errors = m_fec.regenerateDMR(data + 2U);
				// LogDebug("DMR Slot %u, audio sequence no. %u, errs: %u/141", m_slotNo, m_rfN, errors);
				m_rfErrs += errors;
			}

			m_rfBits += 141U;

			m_rfFrames++;

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			if (m_duplex)
				writeQueueRF(data);

			writeNetworkRF(data, DT_VOICE, errors);
		} else if (m_rfState == RS_RF_LATE_ENTRY) {
			CDMREMB emb;
			emb.putData(data + 2U);

			// If we haven't received an LC yet, then be strict on the color code
			unsigned char colorCode = emb.getColorCode();
			if (colorCode != m_colorCode)
				return;

			CDMRLC* lc = m_rfEmbeddedLC.addData(data + 2U, emb.getLCSS());
			if (lc != NULL) {
				unsigned int id = lc->getSrcId();
				if (!validateId(id)) {
					LogMessage("DMR Slot %u, invalid access atemmpt from %u", m_slotNo, id);
					delete lc;
					return;
				}

				m_rfLC = lc;

				// Store the LC for the embedded LC
				m_rfEmbeddedLC.setData(*m_rfLC);

				// Create a dummy start frame to replace the received frame
				unsigned char start[DMR_FRAME_LENGTH_BYTES + 2U];

				CSync::addDMRDataSync(data + 2U);

				CDMRFullLC fullLC;
				fullLC.encode(*m_rfLC, start + 2U, DT_VOICE_LC_HEADER);

				CDMRSlotType slotType;
				slotType.setColorCode(m_colorCode);
				slotType.setDataType(DT_VOICE_LC_HEADER);
				slotType.getData(start + 2U);

				start[0U] = TAG_DATA;
				start[1U] = 0x00U;

				m_rfTimeoutTimer.start();

				m_rfFrames = 0U;
				m_rfSeqNo  = 0U;
				m_rfBits   = 1U;
				m_rfErrs   = 0U;

				if (m_duplex) {
					writeQueueRF(start);
					writeQueueRF(start);
				}

				writeNetworkRF(start, DT_VOICE_LC_HEADER);

				m_rfN = data[1U] & 0x0FU;

				// Regenerate the embedded LC
				unsigned char lcss = m_rfEmbeddedLC.getData(data + 2U, m_rfN);

				// Regenerate the EMB
				emb.setLCSS(lcss);
				emb.getData(data + 2U);

				// Send the original audio frame out
				unsigned int errors = 0U;
				unsigned char fid = m_rfLC->getFID();
				if (fid == FID_ETSI || fid == FID_DMRA) {
					errors = m_fec.regenerateDMR(data + 2U);
					// LogDebug("DMR Slot %u, audio sequence no. %u, errs: %u/141", m_slotNo, m_rfN, errors);
					m_rfErrs += errors;
				}

				m_rfBits += 141U;

				m_rfFrames++;

				data[0U] = TAG_DATA;
				data[1U] = 0x00U;

				if (m_duplex)
					writeQueueRF(data);

				writeNetworkRF(data, DT_VOICE, errors);

				m_rfState = RS_RF_AUDIO;

				std::string src = m_lookup->find(m_rfLC->getSrcId());
				std::string dst = m_lookup->find(m_rfLC->getDstId());

				if (m_netState == RS_NET_IDLE) {
					setShortLC(m_slotNo, m_rfLC->getDstId(), m_rfLC->getFLCO(), true);
					m_display->writeDMR(m_slotNo, src, m_rfLC->getFLCO() == FLCO_GROUP, dst, "R");
				}

				LogMessage("DMR Slot %u, received RF late entry from %s to %s%s", m_slotNo, src.c_str(), m_rfLC->getFLCO() == FLCO_GROUP ? "TG " : "", dst.c_str());
			}
		}
	}
}

unsigned int CDMRSlot::readModem(unsigned char* data)
{
	assert(data != NULL);

	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CDMRSlot::endOfRFData()
{
	LogMessage("DMR Slot %u, ended RF data transmission", m_slotNo);

	if (m_duplex) {
		unsigned char bytes[DMR_FRAME_LENGTH_BYTES + 2U];

		CSync::addDMRDataSync(bytes + 2U);

		CDMRSlotType slotType;
		slotType.setDataType(DT_TERMINATOR_WITH_LC);
		slotType.setColorCode(m_colorCode);
		slotType.getData(bytes + 2U);

		m_rfDataHeader.getTerminator(bytes + 2U);

		bytes[0U] = TAG_DATA;
		bytes[1U] = 0x00U;

		for (unsigned int i = 0U; i < 5U; i++)
			writeQueueRF(bytes);
	}

	writeEndRF();
}

void CDMRSlot::writeEndRF(bool writeEnd)
{
	m_rfState = RS_RF_LISTENING;

	if (m_netState == RS_NET_IDLE) {
		setShortLC(m_slotNo, 0U);
		m_display->clearDMR(m_slotNo);
	}

	m_rfTimeoutTimer.stop();

	m_rfFrames = 0U;
	m_rfErrs = 0U;
	m_rfBits = 0U;

	if (writeEnd) {
		if (m_netState == RS_NET_IDLE && m_duplex) {
			// Create a dummy start end frame
			unsigned char data[DMR_FRAME_LENGTH_BYTES + 2U];

			CSync::addDMRDataSync(data + 2U);

			CDMRFullLC fullLC;
			fullLC.encode(*m_rfLC, data + 2U, DT_TERMINATOR_WITH_LC);

			CDMRSlotType slotType;
			slotType.setColorCode(m_colorCode);
			slotType.setDataType(DT_TERMINATOR_WITH_LC);
			slotType.getData(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			// 480ms of terminator to space things out
			for (unsigned int i = 0U; i < 8U; i++)
				writeQueueRF(data);
		}
	}

	delete m_rfLC;
	m_rfLC = NULL;
}

void CDMRSlot::endOfNetData()
{
	LogMessage("DMR Slot %u, ended network data transmission", m_slotNo);

	if (m_duplex) {
		unsigned char bytes[DMR_FRAME_LENGTH_BYTES + 2U];

		CSync::addDMRDataSync(bytes + 2U);

		CDMRSlotType slotType;
		slotType.setDataType(DT_TERMINATOR_WITH_LC);
		slotType.setColorCode(m_colorCode);
		slotType.getData(bytes + 2U);

		m_netDataHeader.getTerminator(bytes + 2U);

		bytes[0U] = TAG_DATA;
		bytes[1U] = 0x00U;

		for (unsigned int i = 0U; i < 5U; i++)
			writeQueueNet(bytes);
	}

	writeEndNet();
}

void CDMRSlot::writeEndNet(bool writeEnd)
{
	m_netState = RS_NET_IDLE;

	setShortLC(m_slotNo, 0U);

	m_display->clearDMR(m_slotNo);

	m_networkWatchdog.stop();
	m_netTimeoutTimer.stop();
	m_packetTimer.stop();

	m_netFrames = 0U;
	m_netLost = 0U;

	m_netErrs = 0U;
	m_netBits = 0U;

	if (writeEnd) {
		// Create a dummy start end frame
		unsigned char data[DMR_FRAME_LENGTH_BYTES + 2U];

		CSync::addDMRDataSync(data + 2U);

		CDMRFullLC fullLC;
		fullLC.encode(*m_netLC, data + 2U, DT_TERMINATOR_WITH_LC);

		CDMRSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_TERMINATOR_WITH_LC);
		slotType.getData(data + 2U);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		// 480ms of terminator to space things out
		for (unsigned int i = 0U; i < 8U; i++)
			writeQueueNet(data);
	}

	delete m_netLC;
	m_netLC = NULL;

#if defined(DUMP_DMR)
	closeFile();
#endif
}

void CDMRSlot::writeNetwork(const CDMRData& dmrData)
{
	if (m_rfState != RS_RF_LISTENING && m_netState == RS_NET_IDLE)
		return;

	m_networkWatchdog.start();

	unsigned char dataType = dmrData.getDataType();

	unsigned char data[DMR_FRAME_LENGTH_BYTES + 2U];
	dmrData.getData(data + 2U);

	if (dataType == DT_VOICE_LC_HEADER) {
		if (m_netState == RS_NET_AUDIO)
			return;

		CDMRFullLC fullLC;
		m_netLC = fullLC.decode(data + 2U, DT_VOICE_LC_HEADER);
		if (m_netLC == NULL) {
			LogMessage("DMR Slot %u, bad LC received from the network", m_slotNo);
			return;
		}

		// Store the LC for the embedded LC
		m_netEmbeddedLC.setData(*m_netLC);

		// Regenerate the LC data
		fullLC.encode(*m_netLC, data + 2U, DT_VOICE_LC_HEADER);

		// Regenerate the Slot Type
		CDMRSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_VOICE_LC_HEADER);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CSync::addDMRDataSync(data + 2U);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		m_netTimeoutTimer.start();

		m_netFrames = 0U;
		m_netLost = 0U;

		m_netBits = 1U;
		m_netErrs = 0U;

		// 300ms of idle to give breathing space for lost frames
		for (unsigned int i = 0U; i < 5U; i++)
			writeQueueNet(m_idle);

		writeQueueNet(data);
		writeQueueNet(data);

		m_netState = RS_NET_AUDIO;

		setShortLC(m_slotNo, m_netLC->getDstId(), m_netLC->getFLCO(), true);

		std::string src = m_lookup->find(m_netLC->getSrcId());
		std::string dst = m_lookup->find(m_netLC->getDstId());

		m_display->writeDMR(m_slotNo, src, m_netLC->getFLCO() == FLCO_GROUP, dst, "N");

#if defined(DUMP_DMR)
		openFile();
		writeFile(data);
#endif
		LogMessage("DMR Slot %u, received network voice header from %s to %s%s", m_slotNo, src.c_str(), m_netLC->getFLCO() == FLCO_GROUP ? "TG " : "", dst.c_str());
	} else if (dataType == DT_VOICE_PI_HEADER) {
		if (m_netState != RS_NET_AUDIO)
			return;

		// Regenerate the Slot Type
		CDMRSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_VOICE_PI_HEADER);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CSync::addDMRDataSync(data + 2U);

		// Regenerate the payload
		CBPTC19696 bptc;
		unsigned char payload[12U];
		bptc.decode(data + 2U, payload);
		bptc.encode(payload, data + 2U);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		writeQueueNet(data);

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	} else if (dataType == DT_TERMINATOR_WITH_LC) {
		if (m_netState != RS_NET_AUDIO)
			return;

		// Regenerate the LC data
		CDMRFullLC fullLC;
		fullLC.encode(*m_netLC, data + 2U, DT_TERMINATOR_WITH_LC);

		// Regenerate the Slot Type
		CDMRSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_TERMINATOR_WITH_LC);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CSync::addDMRDataSync(data + 2U);

		data[0U] = TAG_EOT;
		data[1U] = 0x00U;

		// 480ms of terminator to space things out
		for (unsigned int i = 0U; i < 8U; i++)
			writeQueueNet(data);

#if defined(DUMP_DMR)
		writeFile(data);
		closeFile();
#endif
		// We've received the voice header and terminator haven't we?
		m_netFrames += 2U;
		if (m_netBits == 0U) m_netBits = 1U;
		LogMessage("DMR Slot %u, received network end of voice transmission, %.1f seconds, %u%% packet loss, BER: %.1f%%", m_slotNo, float(m_netFrames) / 16.667F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));

		writeEndNet();
	} else if (dataType == DT_DATA_HEADER) {
		if (m_netState == RS_NET_DATA)
			return;

		CDMRDataHeader dataHeader;
		bool valid = dataHeader.put(data + 2U);
		if (!valid) {
			LogMessage("DMR Slot %u, unable to decode the network data header", m_slotNo);
			return;
		}

		m_netFrames = dataHeader.getBlocks();

		m_netDataHeader = dataHeader;

		bool gi = dataHeader.getGI();
		unsigned int srcId = dataHeader.getSrcId();
		unsigned int dstId = dataHeader.getDstId();

		m_netLC = new CDMRLC(gi ? FLCO_GROUP : FLCO_USER_USER, srcId, dstId);

		// Regenerate the data header
		dataHeader.get(data + 2U);

		// Regenerate the Slot Type
		CDMRSlotType slotType;
		slotType.setColorCode(m_colorCode);
		slotType.setDataType(DT_DATA_HEADER);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CSync::addDMRDataSync(data + 2U);

		data[0U] = m_netFrames == 0U ? TAG_EOT : TAG_DATA;
		data[1U] = 0x00U;

		// Put a small delay into starting transmission
		writeQueueNet(m_idle);
		writeQueueNet(m_idle);

		writeQueueNet(data);

		m_netState = RS_NET_DATA;

		setShortLC(m_slotNo, dstId, gi ? FLCO_GROUP : FLCO_USER_USER, false);

		std::string src = m_lookup->find(srcId);
		std::string dst = m_lookup->find(dstId);

		m_display->writeDMR(m_slotNo, src, gi, dst, "N");

		LogMessage("DMR Slot %u, received network data header from %s to %s%s, %u blocks", m_slotNo, src.c_str(), gi ? "TG ": "", dst.c_str(), m_netFrames);

		if (m_netFrames == 0U)
			endOfNetData();
	} else if (dataType == DT_VOICE_SYNC) {
		if (m_netState == RS_NET_IDLE) {
			m_netLC = new CDMRLC(dmrData.getFLCO(), dmrData.getSrcId(), dmrData.getDstId());

			m_netTimeoutTimer.start();

			// 540ms of idle to give breathing space for lost frames
			for (unsigned int i = 0U; i < 9U; i++)
				writeQueueNet(m_idle);

#if defined(DUMP_DMR)
			openFile();
#endif
			m_netFrames = 0U;
			m_netLost = 0U;
			m_netBits = 1U;
			m_netErrs = 0U;

			m_netState = RS_NET_AUDIO;

			setShortLC(m_slotNo, m_netLC->getDstId(), m_netLC->getFLCO(), true);

			std::string src = m_lookup->find(m_netLC->getSrcId());
			std::string dst = m_lookup->find(m_netLC->getDstId());

			m_display->writeDMR(m_slotNo, src, m_netLC->getFLCO() == FLCO_GROUP, dst, "N");

			LogMessage("DMR Slot %u, received network late entry from %s to %s%s", m_slotNo, src.c_str(), m_netLC->getFLCO() == FLCO_GROUP ? "TG " : "", dst.c_str());
		}

		if (m_netState == RS_NET_AUDIO) {
			unsigned char fid = m_netLC->getFID();
			if (fid == FID_ETSI || fid == FID_DMRA) {
				unsigned int errors = m_fec.regenerateDMR(data + 2U);
				// LogDebug("DMR Slot %u, audio, errs: %u/141", m_slotNo, errors);
				m_netErrs += errors;
			}

			m_netBits += 141U;

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			// Initialise the lost packet data
			if (m_netFrames == 0U) {
				::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);
				m_netSeqNo = dmrData.getSeqNo();
				m_netN = dmrData.getN();
				m_elapsed.start();
				m_netLost = 0U;
			} else {
				insertSilence(data, dmrData.getSeqNo());
			}

			// Convert the Audio Sync to be from the BS
			CSync::addDMRAudioSync(data + 2U);

			writeQueueNet(data);

			m_packetTimer.start();

			m_netFrames++;

			// Save details in case we need to infill data
			m_netSeqNo = dmrData.getSeqNo();
			m_netN = dmrData.getN();

#if defined(DUMP_DMR)
			writeFile(data);
#endif
		}
	} else if (dataType == DT_VOICE) {
		if (m_netState != RS_NET_AUDIO)
			return;

		unsigned char fid = m_netLC->getFID();
		if (fid == FID_ETSI || fid == FID_DMRA) {
			unsigned int errors = m_fec.regenerateDMR(data + 2U);
			// LogDebug("DMR Slot %u, audio, errs: %u/141", m_slotNo, errors);
			m_netErrs += errors;
		}

		m_netBits += 141U;

		// Regenerate the embedded LC
		unsigned char lcss = m_netEmbeddedLC.getData(data + 2U, dmrData.getN());

		// Change the color code in the EMB
		m_lastEMB.putData(data + 2U);
		m_lastEMB.setColorCode(m_colorCode);
		m_lastEMB.setLCSS(lcss);
		m_lastEMB.getData(data + 2U);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		// Initialise the lost packet data
		if (m_netFrames == 0U) {
			::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);
			m_netSeqNo = dmrData.getSeqNo();
			m_netN = dmrData.getN();
			m_elapsed.start();
			m_netLost = 0U;
		} else {
			insertSilence(data, dmrData.getSeqNo());
		}

		writeQueueNet(data);

		m_packetTimer.start();

		m_netFrames++;

		// Save details in case we need to infill data
		m_netSeqNo = dmrData.getSeqNo();
		m_netN = dmrData.getN();

#if defined(DUMP_DMR)
		writeFile(data);
#endif
	} else if (dataType == DT_CSBK) {
		CDMRCSBK csbk;
		bool valid = csbk.put(data + 2U);
		if (!valid) {
			LogMessage("DMR Slot %u, unable to decode the network CSBK", m_slotNo);
			return;
		}

		CSBKO csbko = csbk.getCSBKO();
		if (csbko == CSBKO_BSDWNACT)
			return;

		bool gi = csbk.getGI();
		unsigned int srcId = csbk.getSrcId();
		unsigned int dstId = csbk.getDstId();

		// Regenerate the CSBK data
		csbk.get(data + 2U);

		// Regenerate the Slot Type
		CDMRSlotType slotType;
		slotType.putData(data + 2U);
		slotType.setColorCode(m_colorCode);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CSync::addDMRDataSync(data + 2U);

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		writeQueueNet(data);

#if defined(DUMP_DMR)
		openFile();
		writeFile(data);
		closeFile();
#endif

		switch (csbko) {
		case CSBKO_UUVREQ:
			LogMessage("DMR Slot %u, received network Unit to Unit Voice Service Request CSBK from %u to %u", m_slotNo, srcId, dstId);
			break;
		case CSBKO_UUANSRSP:
			LogMessage("DMR Slot %u, received network Unit to Unit Voice Service Answer Response CSBK from %u to %u", m_slotNo, srcId, dstId);
			break;
		case CSBKO_NACKRSP:
			LogMessage("DMR Slot %u, received network Negative Acknowledgment Response CSBK from %u to %u", m_slotNo, srcId, dstId);
			break;
		case CSBKO_PRECCSBK:
			LogMessage("DMR Slot %u, received network Preamble CSBK from %u to %s%u", m_slotNo, srcId, gi ? "TG" : "", dstId);
			break;
		default:
			LogWarning("DMR Slot %u, unhandled network CSBK type - 0x%02X", m_slotNo, csbko);
			break;
		}
	} else if (dataType == DT_RATE_12_DATA || dataType == DT_RATE_34_DATA || dataType == DT_RATE_1_DATA) {
		if (m_netState != RS_NET_DATA || m_netFrames == 0U)
			return;

		// Regenerate the rate 1/2 payload
		if (dataType == DT_RATE_12_DATA) {
			CBPTC19696 bptc;
			unsigned char payload[12U];
			bptc.decode(data + 2U, payload);
			bptc.encode(payload, data + 2U);
		}

		// Regenerate the Slot Type
		CDMRSlotType slotType;
		slotType.putData(data + 2U);
		slotType.setColorCode(m_colorCode);
		slotType.getData(data + 2U);

		// Convert the Data Sync to be from the BS
		CSync::addDMRDataSync(data + 2U);

		m_netFrames--;

		data[0U] = m_netFrames == 0U ? TAG_EOT : TAG_DATA;
		data[1U] = 0x00U;

#if defined(DUMP_DMR)
		writeFile(data);
#endif
		writeQueueNet(data);

		if (m_netFrames == 0U)
			endOfNetData();
	} else {
		// Unhandled data type
		LogWarning("DMR Slot %u, unhandled network data type - 0x%02X", m_slotNo, dataType);
	}
}

void CDMRSlot::clock()
{
	unsigned int ms = m_interval.elapsed();
	m_interval.start();

	m_rfTimeoutTimer.clock(ms);
	m_netTimeoutTimer.clock(ms);

	if (m_netState == RS_NET_AUDIO || m_netState == RS_NET_DATA) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			if (m_netState == RS_NET_AUDIO) {
				// We've received the voice header haven't we?
				m_netFrames += 1U;
				if (m_netBits == 0U) m_netBits = 1U;
				LogMessage("DMR Slot %u, network watchdog has expired, %.1f seconds, %u%% packet loss, BER: %.1f%%", m_slotNo, float(m_netFrames) / 16.667F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));
				writeEndNet(true);
#if defined(DUMP_DMR)
				closeFile();
#endif
			} else {
				LogMessage("DMR Slot %u, network watchdog has expired", m_slotNo);
				writeEndNet();
#if defined(DUMP_DMR)
				closeFile();
#endif
			}
		}
	}

	if (m_netState == RS_NET_AUDIO) {
		m_packetTimer.clock(ms);

		if (m_packetTimer.isRunning() && m_packetTimer.hasExpired()) {
			unsigned int elapsed = m_elapsed.elapsed();
			unsigned int frames  = elapsed / DMR_SLOT_TIME;

			if (frames > m_netFrames) {
				unsigned int count = frames - m_netFrames;
				if (count > 3U) {
					LogDebug("DMR Slot %u, lost audio for 300ms filling in, elapsed: %ums, expected: %u, received: %u", m_slotNo, elapsed, frames, m_netFrames);
					insertSilence(count - 1U);
				}
			}

			m_packetTimer.start();
		}
	}
}

void CDMRSlot::writeQueueRF(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netState != RS_NET_IDLE)
		return;

	unsigned char len = DMR_FRAME_LENGTH_BYTES + 2U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("DMR Slot %u, overflow in the DMR slot RF queue", m_slotNo);
		return;
	}

	m_queue.addData(&len, 1U);

	// If the timeout has expired, replace the audio with idles to keep the slot busy
	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		m_queue.addData(m_idle, len);
	else
		m_queue.addData(data, len);
}

void CDMRSlot::writeNetworkRF(const unsigned char* data, unsigned char dataType, FLCO flco, unsigned int srcId, unsigned int dstId, unsigned char errors)
{
	assert(data != NULL);

	if (m_netState != RS_NET_IDLE)
		return;

	if (m_network == NULL)
		return;

	// Don't send to the network if the timeout has expired
	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	CDMRData dmrData;
	dmrData.setSlotNo(m_slotNo);
	dmrData.setDataType(dataType);
	dmrData.setSrcId(srcId);
	dmrData.setDstId(dstId);
	dmrData.setFLCO(flco);
	dmrData.setN(m_rfN);
	dmrData.setSeqNo(m_rfSeqNo);
	dmrData.setBER(errors);

	m_rfSeqNo++;

	dmrData.setData(data + 2U);

	m_network->write(dmrData);
}

void CDMRSlot::writeNetworkRF(const unsigned char* data, unsigned char dataType, unsigned char errors)
{
	assert(data != NULL);
	assert(m_rfLC != NULL);

	writeNetworkRF(data, dataType, m_rfLC->getFLCO(), m_rfLC->getSrcId(), m_rfLC->getDstId(), errors);
}

void CDMRSlot::writeQueueNet(const unsigned char *data)
{
	assert(data != NULL);

	unsigned char len = DMR_FRAME_LENGTH_BYTES + 2U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("DMR Slot %u, overflow in the DMR slot RF queue", m_slotNo);
		return;
	}

	m_queue.addData(&len, 1U);

	// If the timeout has expired, replace the audio with idles to keep the slot busy
	if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired())
		m_queue.addData(m_idle, len);
	else
		m_queue.addData(data, len);
}

void CDMRSlot::init(unsigned int id, unsigned int colorCode, bool selfOnly, const std::vector<unsigned int>& prefixes, const std::vector<unsigned int>& blackList, CModem* modem, CDMRIPSC* network, IDisplay* display, bool duplex, CDMRLookup* lookup)
{
	assert(id != 0U);
	assert(modem != NULL);
	assert(display != NULL);
	assert(lookup != NULL);

	m_id        = id;
	m_colorCode = colorCode;
	m_selfOnly  = selfOnly;
	m_prefixes  = prefixes;
	m_blackList = blackList;
	m_modem     = modem;
	m_network   = network;
	m_display   = display;
	m_duplex    = duplex;
	m_lookup    = lookup;

	m_idle    = new unsigned char[DMR_FRAME_LENGTH_BYTES + 2U];

	::memcpy(m_idle, DMR_IDLE_DATA, DMR_FRAME_LENGTH_BYTES + 2U);

	// Generate the Slot Type for the Idle frame
	CDMRSlotType slotType;
	slotType.setColorCode(colorCode);
	slotType.setDataType(DT_IDLE);
	slotType.getData(m_idle + 2U);
}

bool CDMRSlot::validateId(unsigned int id)
{
	if (m_selfOnly) {
		return id == m_id;
	} else {
		if (std::find(m_blackList.begin(), m_blackList.end(), id) != m_blackList.end())
			return false;

		unsigned int prefix = id / 10000U;
		if (prefix == 0U || prefix > 999U)
			return false;

		if (m_prefixes.size() == 0U)
			return true;

		return std::find(m_prefixes.begin(), m_prefixes.end(), prefix) != m_prefixes.end();
	}
}

void CDMRSlot::setShortLC(unsigned int slotNo, unsigned int id, FLCO flco, bool voice)
{
	assert(m_modem != NULL);

	switch (slotNo) {
		case 1U:
			m_id1    = 0U;
			m_flco1  = flco;
			m_voice1 = voice;
			if (id != 0U) {
				unsigned char buffer[3U];
				buffer[0U] = (id << 16) & 0xFFU;
				buffer[1U] = (id << 8)  & 0xFFU;
				buffer[2U] = (id << 0)  & 0xFFU;
				m_id1 = CCRC::crc8(buffer, 3U);
			}
			break;
		case 2U:
			m_id2    = 0U;
			m_flco2  = flco;
			m_voice2 = voice;
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
		if (m_voice1) {
			if (m_flco1 == FLCO_GROUP)
				lc[1U] |= 0x80U;
			else
				lc[1U] |= 0x90U;
		} else {
			if (m_flco1 == FLCO_GROUP)
				lc[1U] |= 0xB0U;
			else
				lc[1U] |= 0xA0U;
		}
	}

	if (m_id2 != 0U) {
		lc[3U] = m_id2;
		if (m_voice2) {
			if (m_flco2 == FLCO_GROUP)
				lc[1U] |= 0x08U;
			else
				lc[1U] |= 0x09U;
		} else {
			if (m_flco2 == FLCO_GROUP)
				lc[1U] |= 0x0BU;
			else
				lc[1U] |= 0x0AU;
		}
	}

	lc[4U] = CCRC::crc8(lc, 4U);

	unsigned char sLC[9U];

	CDMRShortLC shortLC;
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

void CDMRSlot::insertSilence(const unsigned char* data, unsigned char seqNo)
{
	assert(data != NULL);

	// Check to see if we have any spaces to fill?
	unsigned char seq = m_netSeqNo + 1U;
	if (seq == seqNo) {
		// Just copy the data, nothing else to do here
		::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);
		return;
	}

	unsigned int oldSeqNo = m_netSeqNo + 1U;
	unsigned int newSeqNo = seqNo;

	unsigned int count;
	if (newSeqNo > oldSeqNo)
		count = newSeqNo - oldSeqNo;
	else
		count = (256U + newSeqNo) - oldSeqNo;

	if (count < 10U)
		insertSilence(count);

	::memcpy(m_lastFrame, data, DMR_FRAME_LENGTH_BYTES + 2U);
}

void CDMRSlot::insertSilence(unsigned int count)
{
	unsigned char data[DMR_FRAME_LENGTH_BYTES + 2U];

	::memcpy(data, m_lastFrame, 2U);					// The control data
	::memcpy(data + 2U, m_lastFrame + 24U + 2U, 9U);	// Copy the last audio block to the first
	::memcpy(data + 24U + 2U, data + 2U, 9U);			// Copy the last audio block to the last
	::memcpy(data + 9U + 2U, data + 2U, 5U);			// Copy the last audio block to the middle (1/2)
	::memcpy(data + 19U + 2U, data + 4U + 2U, 5U);		// Copy the last audio block to the middle (2/2)

	unsigned char n = (m_netN + 1U) % 6U;
	unsigned char seqNo = m_netSeqNo + 1U;

	unsigned char fid = m_netLC->getFID();

	for (unsigned int i = 0U; i < count; i++) {
		// Only use our silence frame if its AMBE audio data
		if (fid == FID_ETSI || fid == FID_DMRA) {
			if (i > 0U)
				::memcpy(data, DMR_SILENCE_DATA, DMR_FRAME_LENGTH_BYTES + 2U);
		}

		if (n == 0U) {
			CSync::addDMRAudioSync(data + 2U);
		} else {
			unsigned char lcss = m_netEmbeddedLC.getData(data + 2U, n);

			m_lastEMB.setColorCode(m_colorCode);
			m_lastEMB.setLCSS(lcss);
			m_lastEMB.getData(data + 2U);
		}

		writeQueueNet(data);

		m_netSeqNo = seqNo;
		m_netN     = n;

		m_netFrames++;
		m_netLost++;

		seqNo++;
		n = (n + 1U) % 6U;
	}
}
