/*
*   Copyright (C) 2016,2017,2018 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Bryan Biedenkapp <gatekeep@gmail.com>
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
#include "P25Trellis.h"
#include "P25Utils.h"
#include "Utils.h"
#include "Sync.h"
#include "CRC.h"
#include "Log.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>

// #define	DUMP_P25

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Control::CP25Control(unsigned int nac, unsigned int id, bool selfOnly, bool uidOverride, CP25Network* network, CDisplay* display, unsigned int timeout, bool duplex, CDMRLookup* lookup, bool remoteGateway, CRSSIInterpolator* rssiMapper) :
m_nac(nac),
m_id(id),
m_selfOnly(selfOnly),
m_uidOverride(uidOverride),
m_remoteGateway(remoteGateway),
m_network(network),
m_display(display),
m_duplex(duplex),
m_lookup(lookup),
m_queue(1000U, "P25 Control"),
m_rfState(RS_RF_LISTENING),
m_netState(RS_NET_IDLE),
m_rfTimeout(1000U, timeout),
m_netTimeout(1000U, timeout),
m_networkWatchdog(1000U, 0U, 1500U),
m_rfFrames(0U),
m_rfBits(0U),
m_rfErrs(0U),
m_netFrames(0U),
m_netLost(0U),
m_rfDataFrames(0U),
m_nid(nac),
m_lastDUID(P25_DUID_TERM),
m_audio(),
m_rfData(),
m_netData(),
m_rfLSD(),
m_netLSD(),
m_netLDU1(NULL),
m_netLDU2(NULL),
m_lastIMBE(NULL),
m_rfLDU(NULL),
m_rfPDU(NULL),
m_rfPDUCount(0U),
m_rfPDUBits(0U),
m_rssiMapper(rssiMapper),
m_rssi(0U),
m_maxRSSI(0U),
m_minRSSI(0U),
m_aveRSSI(0U),
m_rssiCount(0U),
m_fp(NULL)
{
	assert(display != NULL);
	assert(lookup != NULL);
	assert(rssiMapper != NULL);

	m_netLDU1 = new unsigned char[9U * 25U];
	m_netLDU2 = new unsigned char[9U * 25U];

	::memset(m_netLDU1, 0x00U, 9U * 25U);
	::memset(m_netLDU2, 0x00U, 9U * 25U);

	m_lastIMBE = new unsigned char[11U];
	::memcpy(m_lastIMBE, P25_NULL_IMBE, 11U);

	m_rfLDU = new unsigned char[P25_LDU_FRAME_LENGTH_BYTES];
	::memset(m_rfLDU, 0x00U, P25_LDU_FRAME_LENGTH_BYTES);

	m_rfPDU = new unsigned char[P25_MAX_PDU_COUNT * P25_LDU_FRAME_LENGTH_BYTES + 2U];
	::memset(m_rfPDU, 0x00U, P25_MAX_PDU_COUNT * P25_LDU_FRAME_LENGTH_BYTES + 2U);
}

CP25Control::~CP25Control()
{
	delete[] m_netLDU1;
	delete[] m_netLDU2;
	delete[] m_lastIMBE;
	delete[] m_rfLDU;
	delete[] m_rfPDU;
}

bool CP25Control::writeModem(unsigned char* data, unsigned int len)
{
	assert(data != NULL);

	bool sync = data[1U] == 0x01U;

	if (data[0U] == TAG_LOST && m_rfState == RS_RF_AUDIO) {
		if (m_rssi != 0U)
			LogMessage("P25, transmission lost, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
		else
			LogMessage("P25, transmission lost, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));

		if (m_netState == RS_NET_IDLE)
			m_display->clearP25();

		writeNetwork(m_rfLDU, m_lastDUID, true);
		writeNetwork(data + 2U, P25_DUID_TERM, true);
		m_rfState = RS_RF_LISTENING;
		m_rfTimeout.stop();
		m_rfData.reset();
#if defined(DUMP_P25)
		closeFile();
#endif
		return false;
	}

	if (data[0U] == TAG_LOST && m_rfState == RS_RF_DATA) {
		if (m_netState == RS_NET_IDLE)
			m_display->clearP25();

		m_rfState    = RS_RF_LISTENING;
		m_rfPDUCount = 0U;
		m_rfPDUBits  = 0U;
#if defined(DUMP_P25)
		closeFile();
#endif
		return false;
	}

	if (data[0U] == TAG_LOST) {
		m_rfState = RS_RF_LISTENING;
		return false;
	}

	if (!sync && m_rfState == RS_RF_LISTENING)
		return false;

	// Decode the NID
	bool valid = m_nid.decode(data + 2U);

	if (m_rfState == RS_RF_LISTENING && !valid)
		return false;

	unsigned char duid = m_nid.getDUID();
	if (!valid) {
		switch (m_lastDUID) {
		case P25_DUID_HEADER:
		case P25_DUID_LDU2:
			duid = P25_DUID_LDU1;
			break;
		case P25_DUID_LDU1:
			duid = P25_DUID_LDU2;
			break;
		case P25_DUID_PDU:
			duid = P25_DUID_PDU;
			break;
		case P25_DUID_TSDU:
			duid = P25_DUID_TSDU;
			break;
		default:
			break;
		}
	}

	// Have we got RSSI bytes on the end of a P25 LDU?
	if (len == (P25_LDU_FRAME_LENGTH_BYTES + 4U)) {
		uint16_t raw = 0U;
		raw |= (data[218U] << 8) & 0xFF00U;
		raw |= (data[219U] << 0) & 0x00FFU;

		// Convert the raw RSSI to dBm
		int rssi = m_rssiMapper->interpolate(raw);
		if (rssi != 0)
			LogDebug("P25, raw RSSI: %u, reported RSSI: %d dBm", raw, rssi);

		// RSSI is always reported as positive
		m_rssi = (rssi >= 0) ? rssi : -rssi;

		if (m_rssi > m_minRSSI)
			m_minRSSI = m_rssi;
		if (m_rssi < m_maxRSSI)
			m_maxRSSI = m_rssi;

		m_aveRSSI += m_rssi;
		m_rssiCount++;
	}

	if (duid == P25_DUID_LDU1) {
		if (m_rfState == RS_RF_LISTENING) {
			m_rfData.reset();
			bool ret = m_rfData.decodeLDU1(data + 2U);
			if (!ret) {
				m_lastDUID = duid;
				return false;
			}

			unsigned int srcId = m_rfData.getSrcId();

			if (m_selfOnly) {
				if (m_id > 99999999U) {		// Check that the Config DMR-ID is bigger than 8 digits
					if (srcId != m_id / 100U)
						return false;
				}

				else if (m_id > 9999999U) {	// Check that the Config DMR-ID is bigger than 7 digits
					if (srcId != m_id / 10U)
						return false;
				}

				else if (srcId != m_id) {	// All other cases
					return false;
				}
			}

			if (!m_uidOverride) {
				bool found = m_lookup->exists(srcId);
				if (!found)
					return false;
			}

			bool           grp = m_rfData.getLCF() == P25_LCF_GROUP;
			unsigned int dstId = m_rfData.getDstId();
			std::string source = m_lookup->find(srcId);

			LogMessage("P25, received RF voice transmission from %s to %s%u", source.c_str(), grp ? "TG " : "", dstId);
			m_display->writeP25(source.c_str(), grp, dstId, "R");

			m_rfState = RS_RF_AUDIO;

			m_minRSSI = m_rssi;
			m_maxRSSI = m_rssi;
			m_aveRSSI = m_rssi;
			m_rssiCount = 1U;

			createRFHeader();
			writeNetwork(data + 2U, P25_DUID_HEADER, false);
		} else if (m_rfState == RS_RF_AUDIO) {
			writeNetwork(m_rfLDU, m_lastDUID, false);
		}

		if (m_rfState == RS_RF_AUDIO) {
			// Regenerate Sync
			CSync::addP25Sync(data + 2U);

			// Regenerate NID
			m_nid.encode(data + 2U, P25_DUID_LDU1);

			// Regenerate LDU1 Data
			m_rfData.encodeLDU1(data + 2U);

			// Regenerate the Low Speed Data
			m_rfLSD.process(data + 2U);

			// Regenerate Audio
			unsigned int errors = m_audio.process(data + 2U);
			LogDebug("P25, LDU1 audio, errs: %u/1233 (%.1f%%)", errors, float(errors) / 12.33F);

			m_display->writeP25BER(float(errors) / 12.33F);

			m_rfBits += 1233U;
			m_rfErrs += errors;
			m_rfFrames++;
			m_lastDUID = duid;

			// Add busy bits
			addBusyBits(data + 2U, P25_LDU_FRAME_LENGTH_BITS, false, true);

#if defined(DUMP_P25)
			writeFile(data + 2U, len - 2U);
#endif

			::memcpy(m_rfLDU, data + 2U, P25_LDU_FRAME_LENGTH_BYTES);

			if (m_duplex) {
				data[0U] = TAG_DATA;
				data[1U] = 0x00U;
				writeQueueRF(data, P25_LDU_FRAME_LENGTH_BYTES + 2U);
			}

			m_display->writeP25RSSI(m_rssi);

			return true;
		}
	} else if (duid == P25_DUID_LDU2) {
		if (m_rfState == RS_RF_AUDIO) {
			writeNetwork(m_rfLDU, m_lastDUID, false);

			// Regenerate Sync
			CSync::addP25Sync(data + 2U);

			// Regenerate NID
			m_nid.encode(data + 2U, P25_DUID_LDU2);

			// Add the dummy LDU2 data
			m_rfData.encodeLDU2(data + 2U);

			// Regenerate the Low Speed Data
			m_rfLSD.process(data + 2U);

			// Regenerate Audio
			unsigned int errors = m_audio.process(data + 2U);
			LogDebug("P25, LDU2 audio, errs: %u/1233 (%.1f%%)", errors, float(errors) / 12.33F);

			m_display->writeP25BER(float(errors) / 12.33F);

			m_rfBits += 1233U;
			m_rfErrs += errors;
			m_rfFrames++;
			m_lastDUID = duid;

			// Add busy bits
			addBusyBits(data + 2U, P25_LDU_FRAME_LENGTH_BITS, false, true);

#if defined(DUMP_P25)
			writeFile(data + 2U, len - 2U);
#endif

			::memcpy(m_rfLDU, data + 2U, P25_LDU_FRAME_LENGTH_BYTES);

			if (m_duplex) {
				data[0U] = TAG_DATA;
				data[1U] = 0x00U;
				writeQueueRF(data, P25_LDU_FRAME_LENGTH_BYTES + 2U);
			}

			m_display->writeP25RSSI(m_rssi);

			return true;
		}
	} else if (duid == P25_DUID_TSDU) {
		if (m_rfState != RS_RF_DATA) {
			m_rfPDUCount = 0U;
			m_rfPDUBits = 0U;
			m_rfState = RS_RF_DATA;
			m_rfDataFrames = 0U;
		}
	
		bool ret = m_rfData.decodeTSDU(data + 2U);
		if (!ret) {
			m_lastDUID = duid;
			return false;
		}
	
		unsigned int srcId = m_rfData.getSrcId();
		unsigned int dstId = m_rfData.getDstId();
	
		unsigned char data[P25_TSDU_FRAME_LENGTH_BYTES + 2U];
	
		switch (m_rfData.getLCF()) {
		case P25_LCF_TSBK_CALL_ALERT:
			LogMessage("P25, received RF TSDU transmission, CALL ALERT from %u to %u", srcId, dstId);
			::memset(data + 2U, 0x00U, P25_TSDU_FRAME_LENGTH_BYTES);
	
			// Regenerate Sync
			CSync::addP25Sync(data + 2U);

			// Regenerate NID
			m_nid.encode(data + 2U, P25_DUID_TSDU);

			// Regenerate TDULC Data
			m_rfData.encodeTSDU(data + 2U);

			// Add busy bits
			addBusyBits(data + 2U, P25_TSDU_FRAME_LENGTH_BITS, true, false);

			// Set first busy bits to 1,1
			setBusyBits(data + 2U, P25_SS0_START, true, true);

			if (m_duplex) {
				data[0U] = TAG_DATA;
				data[1U] = 0x00U;

				writeQueueRF(data, P25_TSDU_FRAME_LENGTH_BYTES + 2U);
			}
			break;
		case P25_LCF_TSBK_ACK_RSP_FNE:
			LogMessage("P25, received RF TSDU transmission, ACK RESPONSE FNE from %u to %u", srcId, dstId);
			::memset(data + 2U, 0x00U, P25_TSDU_FRAME_LENGTH_BYTES);

			// Regenerate Sync
			CSync::addP25Sync(data + 2U);

			// Regenerate NID
			m_nid.encode(data + 2U, P25_DUID_TSDU);

			// Regenerate TDULC Data
			m_rfData.encodeTSDU(data + 2U);

			// Add busy bits
			addBusyBits(data + 2U, P25_TSDU_FRAME_LENGTH_BITS, true, false);

			// Set first busy bits to 1,1
			setBusyBits(data + 2U, P25_SS0_START, true, true);

			if (m_duplex) {
				data[0U] = TAG_DATA;
				data[1U] = 0x00U;

				writeQueueRF(data, P25_TSDU_FRAME_LENGTH_BYTES + 2U);
			}
			break;
		default:
			LogMessage("P25, recieved RF TSDU transmission, unhandled LCF $%02X", m_rfData.getLCF());
			break;
		}

		m_rfState = RS_RF_LISTENING;
		return true;
	} else if (duid == P25_DUID_TERM || duid == P25_DUID_TERM_LC) {
		if (m_rfState == RS_RF_AUDIO) {
			writeNetwork(m_rfLDU, m_lastDUID, true);

			::memset(data + 2U, 0x00U, P25_TERM_FRAME_LENGTH_BYTES);

			// Regenerate Sync
			CSync::addP25Sync(data + 2U);

			// Regenerate NID
			m_nid.encode(data + 2U, P25_DUID_TERM);

			// Add busy bits
			addBusyBits(data + 2U, P25_TERM_FRAME_LENGTH_BITS, false, true);

			m_rfState = RS_RF_LISTENING;
			m_rfTimeout.stop();
			m_rfData.reset();
			m_lastDUID = duid;

			if (m_rssi != 0U)
				LogMessage("P25, received RF end of voice transmission, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
			else
				LogMessage("P25, received RF end of voice transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 5.56F, float(m_rfErrs * 100U) / float(m_rfBits));

			m_display->clearP25();

#if defined(DUMP_P25)
			closeFile();
#endif

			writeNetwork(data + 2U, P25_DUID_TERM, true);

			if (m_duplex) {
				data[0U] = TAG_EOT;
				data[1U] = 0x00U;
				writeQueueRF(data, P25_TERM_FRAME_LENGTH_BYTES + 2U);
			}
		}
	} else if (duid == P25_DUID_PDU) {
		if (m_rfState != RS_RF_DATA) {
			m_rfPDUCount   = 0U;
			m_rfPDUBits    = 0U;
			m_rfState      = RS_RF_DATA;
			m_rfDataFrames = 0U;
		}

		unsigned int start = m_rfPDUCount * P25_LDU_FRAME_LENGTH_BITS;

		unsigned char buffer[P25_LDU_FRAME_LENGTH_BYTES];
		unsigned int bits = CP25Utils::decode(data + 2U, buffer, start, start + P25_LDU_FRAME_LENGTH_BITS);

		for (unsigned int i = 0U; i < bits; i++, m_rfPDUBits++) {
			bool b = READ_BIT(buffer, i);
			WRITE_BIT(m_rfPDU, m_rfPDUBits, b);
		}

		if (m_rfPDUCount == 0U) {
			CP25Trellis trellis;
			unsigned char header[P25_PDU_HEADER_LENGTH_BYTES];
			bool valid = trellis.decode12(m_rfPDU + P25_SYNC_LENGTH_BYTES + P25_NID_LENGTH_BYTES, header);
			if (valid)
				valid = CCRC::checkCCITT162(header, P25_PDU_HEADER_LENGTH_BYTES);

			if (valid) {
				unsigned int llId = (header[3U] << 16) + (header[4U] << 8) + header[5U];
				unsigned int sap  = header[1U] & 0x3FU;
				m_rfDataFrames    = header[6U] & 0x7FU;

				LogMessage("P25, received RF data transmission for Local Link Id %u, SAP %u, %u blocks", llId, sap, m_rfDataFrames);
			} else {
				m_rfPDUCount   = 0U;
				m_rfPDUBits    = 0U;
				m_rfState      = RS_RF_LISTENING;
				m_rfDataFrames = 0U;
			}
		}

		if (m_rfState == RS_RF_DATA) {
			m_rfPDUCount++;

			unsigned int bitLength = ((m_rfDataFrames + 1U) * P25_PDU_FEC_LENGTH_BITS) + P25_SYNC_LENGTH_BITS + P25_NID_LENGTH_BITS;

			if (m_rfPDUBits >= bitLength) {
				unsigned int offset = P25_SYNC_LENGTH_BYTES + P25_NID_LENGTH_BYTES;

				// Regenerate the PDU header
				CP25Trellis trellis;
				unsigned char header[P25_PDU_HEADER_LENGTH_BYTES];
				trellis.decode12(m_rfPDU + offset, header);
				trellis.encode12(header, m_rfPDU + offset);
				offset += P25_PDU_FEC_LENGTH_BITS;

				// Regenerate the PDU data
				for (unsigned int i = 0U; i < m_rfDataFrames; i++) {
					unsigned char data[P25_PDU_CONFIRMED_LENGTH_BYTES];

					bool valid = trellis.decode34(m_rfPDU + offset, data);
					if (valid) {
						trellis.encode34(data, m_rfPDU + offset);
					} else {
						valid = trellis.decode12(m_rfPDU + offset, data);
						if (valid)
							trellis.encode12(data, m_rfPDU + offset);
					}

					offset += P25_PDU_FEC_LENGTH_BITS;
				}

				unsigned char pdu[1024U];

				// Add the data
				unsigned int newBitLength = CP25Utils::encode(m_rfPDU, pdu + 2U, bitLength);
				unsigned int newByteLength = newBitLength / 8U;
				if ((newBitLength % 8U) > 0U)
					newByteLength++;

				// Regenerate Sync
				CSync::addP25Sync(pdu + 2U);

				// Regenerate NID
				m_nid.encode(pdu + 2U, P25_DUID_PDU);

				// Add busy bits
				addBusyBits(pdu + 2U, newBitLength, false, true);

				if (m_duplex) {
					pdu[0U] = TAG_DATA;
					pdu[1U] = 0x00U;
					writeQueueRF(pdu, newByteLength + 2U);
				}

				LogMessage("P25, ended RF data transmission");
				m_display->clearP25();

				m_rfPDUCount = 0U;
				m_rfPDUBits = 0U;
				m_rfState = RS_RF_LISTENING;
				m_rfDataFrames = 0U;
			}

			return true;
		}
	}

	return false;
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

void CP25Control::writeNetwork()
{
	unsigned char data[100U];
	unsigned int length = m_network->read(data, 100U);
	if (length == 0U)
		return;

	if (m_rfState != RS_RF_LISTENING && m_netState == RS_NET_IDLE)
		return;

	m_networkWatchdog.start();

	switch (data[0U]) {
	case 0x62U:
		::memcpy(m_netLDU1 + 0U, data, 22U);
		checkNetLDU2();
		break;
	case 0x63U:
		::memcpy(m_netLDU1 + 25U, data, 14U);
		checkNetLDU2();
		break;
	case 0x64U:
		::memcpy(m_netLDU1 + 50U, data, 17U);
		checkNetLDU2();
		break;
	case 0x65U:
		::memcpy(m_netLDU1 + 75U, data, 17U);
		checkNetLDU2();
		break;
	case 0x66U:
		::memcpy(m_netLDU1 + 100U, data, 17U);
		checkNetLDU2();
		break;
	case 0x67U:
		::memcpy(m_netLDU1 + 125U, data, 17U);
		checkNetLDU2();
		break;
	case 0x68U:
		::memcpy(m_netLDU1 + 150U, data, 17U);
		checkNetLDU2();
		break;
	case 0x69U:
		::memcpy(m_netLDU1 + 175U, data, 17U);
		checkNetLDU2();
		break;
	case 0x6AU:
		::memcpy(m_netLDU1 + 200U, data, 16U);
		checkNetLDU2();
		if (m_netState != RS_NET_IDLE)
			createNetLDU1();
		break;
	case 0x6BU:
		::memcpy(m_netLDU2 + 0U, data, 22U);
		checkNetLDU1();
		break;
	case 0x6CU:
		::memcpy(m_netLDU2 + 25U, data, 14U);
		checkNetLDU1();
		break;
	case 0x6DU:
		::memcpy(m_netLDU2 + 50U, data, 17U);
		checkNetLDU1();
		break;
	case 0x6EU:
		::memcpy(m_netLDU2 + 75U, data, 17U);
		checkNetLDU1();
		break;
	case 0x6FU:
		::memcpy(m_netLDU2 + 100U, data, 17U);
		checkNetLDU1();
		break;
	case 0x70U:
		::memcpy(m_netLDU2 + 125U, data, 17U);
		checkNetLDU1();
		break;
	case 0x71U:
		::memcpy(m_netLDU2 + 150U, data, 17U);
		checkNetLDU1();
		break;
	case 0x72U:
		::memcpy(m_netLDU2 + 175U, data, 17U);
		checkNetLDU1();
		break;
	case 0x73U:
		::memcpy(m_netLDU2 + 200U, data, 16U);
		if (m_netState == RS_NET_IDLE) {
			createNetHeader();
			createNetLDU1();
		} else {
			checkNetLDU1();
		}
		createNetLDU2();
		break;
	case 0x80U:
		createNetTerminator();
		break;
	default:
		break;
	}
}

void CP25Control::clock(unsigned int ms)
{
	if (m_network != NULL)
		writeNetwork();

	m_rfTimeout.clock(ms);
	m_netTimeout.clock(ms);

	if (m_netState == RS_NET_AUDIO) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			LogMessage("P25, network watchdog has expired, %.1f seconds, %u%% packet loss", float(m_netFrames) / 50.0F, (m_netLost * 100U) / m_netFrames);
			m_display->clearP25();
			m_networkWatchdog.stop();
			m_netState = RS_NET_IDLE;
			m_netData.reset();
			m_netTimeout.stop();
		}
	}
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

void CP25Control::writeQueueNet(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	if (m_netTimeout.isRunning() && m_netTimeout.hasExpired())
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

void CP25Control::writeNetwork(const unsigned char *data, unsigned char type, bool end)
{
	assert(data != NULL);

	if (m_network == NULL)
		return;

	if (m_rfTimeout.isRunning() && m_rfTimeout.hasExpired())
		return;

	switch (type)
	{
	case P25_DUID_LDU1:
		m_network->writeLDU1(data, m_rfData, m_rfLSD, end);
		break;
	case P25_DUID_LDU2:
		m_network->writeLDU2(data, m_rfData, m_rfLSD, end);
		break;
	default:
		break;
	}
}

void CP25Control::setBusyBits(unsigned char* data, unsigned int ssOffset, bool b1, bool b2)
{
    assert(data != NULL);

    WRITE_BIT(data, ssOffset, b1);
    WRITE_BIT(data, ssOffset + 1U, b2);
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

void CP25Control::checkNetLDU1()
{
	if (m_netState == RS_NET_IDLE)
		return;

	// Check for an unflushed LDU1
	if (m_netLDU1[0U]   != 0x00U || m_netLDU1[25U]  != 0x00U || m_netLDU1[50U]  != 0x00U ||
		m_netLDU1[75U]  != 0x00U || m_netLDU1[100U] != 0x00U || m_netLDU1[125U] != 0x00U ||
		m_netLDU1[150U] != 0x00U || m_netLDU1[175U] != 0x00U || m_netLDU1[200U] != 0x00U)
		createNetLDU1();
}

void CP25Control::checkNetLDU2()
{
	if (m_netState == RS_NET_IDLE)
		return;

	// Check for an unflushed LDU1
	if (m_netLDU2[0U]   != 0x00U || m_netLDU2[25U]  != 0x00U || m_netLDU2[50U]  != 0x00U ||
		m_netLDU2[75U]  != 0x00U || m_netLDU2[100U] != 0x00U || m_netLDU2[125U] != 0x00U ||
		m_netLDU2[150U] != 0x00U || m_netLDU2[175U] != 0x00U || m_netLDU2[200U] != 0x00U)
		createNetLDU2();
}

void CP25Control::insertMissingAudio(unsigned char* data)
{
	if (data[0U] == 0x00U) {
		::memcpy(data + 10U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 10U, 11U);
	}

	if (data[25U] == 0x00U) {
		::memcpy(data + 26U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 26U, 11U);
	}

	if (data[50U] == 0x00U) {
		::memcpy(data + 55U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 55U, 11U);
	}

	if (data[75U] == 0x00U) {
		::memcpy(data + 80U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 80U, 11U);
	}

	if (data[100U] == 0x00U) {
		::memcpy(data + 105U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 105U, 11U);
	}

	if (data[125U] == 0x00U) {
		::memcpy(data + 130U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 130U, 11U);
	}

	if (data[150U] == 0x00U) {
		::memcpy(data + 155U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 155U, 11U);
	}

	if (data[175U] == 0x00U) {
		::memcpy(data + 180U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 180U, 11U);
	}

	if (data[200U] == 0x00U) {
		::memcpy(data + 204U, m_lastIMBE, 11U);
		m_netLost++;
	} else {
		::memcpy(m_lastIMBE, data + 204U, 11U);
	}
}

void CP25Control::createRFHeader()
{
	unsigned char buffer[P25_HDR_FRAME_LENGTH_BYTES + 2U];
	::memset(buffer, 0x00U, P25_HDR_FRAME_LENGTH_BYTES + 2U);

	buffer[0U] = TAG_HEADER;
	buffer[1U] = 0x00U;

	// Add the sync
	CSync::addP25Sync(buffer + 2U);

	// Add the NID
	m_nid.encode(buffer + 2U, P25_DUID_HEADER);

	// Add the dummy header
	m_rfData.encodeHeader(buffer + 2U);

	// Add busy bits
	addBusyBits(buffer + 2U, P25_HDR_FRAME_LENGTH_BITS, false, true);

	m_rfFrames = 0U;
	m_rfErrs = 0U;
	m_rfBits = 1U;
	m_rfTimeout.start();
	m_lastDUID = P25_DUID_HEADER;
	::memset(m_rfLDU, 0x00U, P25_LDU_FRAME_LENGTH_BYTES);

#if defined(DUMP_P25)
	openFile();
	writeFile(buffer + 2U, buffer - 2U);
#endif

	if (m_duplex) {
		buffer[0U] = TAG_HEADER;
		buffer[1U] = 0x00U;
		writeQueueRF(buffer, P25_HDR_FRAME_LENGTH_BYTES + 2U);
	}
}

void CP25Control::createNetHeader()
{
	unsigned char  lcf = m_netLDU1[51U];
	unsigned char mfId = m_netLDU1[52U];
	unsigned int dstId = (m_netLDU1[76U] << 16) + (m_netLDU1[77U] << 8) + m_netLDU1[78U];
	unsigned int srcId = (m_netLDU1[101U] << 16) + (m_netLDU1[102U] << 8) + m_netLDU1[103U];

	unsigned char algId = m_netLDU2[126U];
	unsigned int    kId = (m_netLDU2[127U] << 8) + m_netLDU2[128U];

	unsigned char mi[P25_MI_LENGTH_BYTES];
	::memcpy(mi + 0U, m_netLDU2 + 51U, 3U);
	::memcpy(mi + 3U, m_netLDU2 + 76U, 3U);
	::memcpy(mi + 6U, m_netLDU2 + 101U, 3U);

	m_netData.reset();
	m_netData.setMI(mi);
	m_netData.setAlgId(algId);
	m_netData.setKId(kId);
	m_netData.setLCF(lcf);
	m_netData.setMFId(mfId);
	m_netData.setSrcId(srcId);
	m_netData.setDstId(dstId);

	std::string source = m_lookup->find(srcId);

	LogMessage("P25, received network transmission from %s to %s%u", source.c_str(), lcf == P25_LCF_GROUP ? "TG " : "", dstId);

	m_display->writeP25(source.c_str(), lcf == P25_LCF_GROUP, dstId, "N");

	m_netState = RS_NET_AUDIO;
	m_netTimeout.start();
	m_netFrames = 0U;
	m_netLost = 0U;

	unsigned char buffer[P25_HDR_FRAME_LENGTH_BYTES + 2U];
	::memset(buffer, 0x00U, P25_HDR_FRAME_LENGTH_BYTES + 2U);

	buffer[0U] = TAG_HEADER;
	buffer[1U] = 0x00U;

	// Add the sync
	CSync::addP25Sync(buffer + 2U);

	// Add the NID
	m_nid.encode(buffer + 2U, P25_DUID_HEADER);

	// Add the dummy header
	m_netData.encodeHeader(buffer + 2U);

	// Add busy bits
	if (m_remoteGateway)
		addBusyBits(buffer + 2U, P25_HDR_FRAME_LENGTH_BITS, false, false);
	else
		addBusyBits(buffer + 2U, P25_HDR_FRAME_LENGTH_BITS, false, true);

	writeQueueNet(buffer, P25_HDR_FRAME_LENGTH_BYTES + 2U);
}

void CP25Control::createNetLDU1()
{
	insertMissingAudio(m_netLDU1);

	unsigned char buffer[P25_LDU_FRAME_LENGTH_BYTES + 2U];
	::memset(buffer, 0x00U, P25_LDU_FRAME_LENGTH_BYTES + 2U);

	buffer[0U] = TAG_DATA;
	buffer[1U] = 0x00U;

	// Add the sync
	CSync::addP25Sync(buffer + 2U);

	// Add the NID
	m_nid.encode(buffer + 2U, P25_DUID_LDU1);

	// Add the LDU1 data
	m_netData.encodeLDU1(buffer + 2U);

	// Add the Audio
	m_audio.encode(buffer + 2U, m_netLDU1 + 10U, 0U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 26U, 1U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 55U, 2U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 80U, 3U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 105U, 4U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 130U, 5U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 155U, 6U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 180U, 7U);
	m_audio.encode(buffer + 2U, m_netLDU1 + 204U, 8U);

	// Add the Low Speed Data
	m_netLSD.setLSD1(m_netLDU1[201U]);
	m_netLSD.setLSD2(m_netLDU1[202U]);
	m_netLSD.encode(buffer + 2U);

	// Add busy bits
	if (m_remoteGateway)
		addBusyBits(buffer + 2U, P25_LDU_FRAME_LENGTH_BITS, false, false);
	else
		addBusyBits(buffer + 2U, P25_LDU_FRAME_LENGTH_BITS, false, true);

	writeQueueNet(buffer, P25_LDU_FRAME_LENGTH_BYTES + 2U);

	::memset(m_netLDU1, 0x00U, 9U * 25U);

	m_netFrames += 9U;
}

void CP25Control::createNetLDU2()
{
	insertMissingAudio(m_netLDU2);

	unsigned char buffer[P25_LDU_FRAME_LENGTH_BYTES + 2U];
	::memset(buffer, 0x00U, P25_LDU_FRAME_LENGTH_BYTES + 2U);

	buffer[0U] = TAG_DATA;
	buffer[1U] = 0x00U;

	// Add the sync
	CSync::addP25Sync(buffer + 2U);

	// Add the NID
	m_nid.encode(buffer + 2U, P25_DUID_LDU2);

	// Add the dummy LDU2 data
	m_netData.encodeLDU2(buffer + 2U);

	// Add the Audio
	m_audio.encode(buffer + 2U, m_netLDU2 + 10U, 0U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 26U, 1U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 55U, 2U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 80U, 3U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 105U, 4U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 130U, 5U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 155U, 6U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 180U, 7U);
	m_audio.encode(buffer + 2U, m_netLDU2 + 204U, 8U);

	// Add the Low Speed Data
	m_netLSD.setLSD1(m_netLDU2[201U]);
	m_netLSD.setLSD2(m_netLDU2[202U]);
	m_netLSD.encode(buffer + 2U);

	// Add busy bits
	if (m_remoteGateway)
		addBusyBits(buffer + 2U, P25_LDU_FRAME_LENGTH_BITS, false, false);
	else
		addBusyBits(buffer + 2U, P25_LDU_FRAME_LENGTH_BITS, false, true);

	writeQueueNet(buffer, P25_LDU_FRAME_LENGTH_BYTES + 2U);

	::memset(m_netLDU2, 0x00U, 9U * 25U);

	m_netFrames += 9U;
}

void CP25Control::createNetTerminator()
{
	unsigned char buffer[P25_TERM_FRAME_LENGTH_BYTES + 2U];
	::memset(buffer, 0x00U, P25_TERM_FRAME_LENGTH_BYTES + 2U);

	buffer[0U] = TAG_EOT;
	buffer[1U] = 0x00U;

	// Add the sync
	CSync::addP25Sync(buffer + 2U);

	// Add the NID
	m_nid.encode(buffer + 2U, P25_DUID_TERM);

	// Add busy bits
	if (m_remoteGateway)
		addBusyBits(buffer + 2U, P25_TERM_FRAME_LENGTH_BITS, false, false);
	else
		addBusyBits(buffer + 2U, P25_TERM_FRAME_LENGTH_BITS, false, true);

	writeQueueNet(buffer, P25_TERM_FRAME_LENGTH_BYTES + 2U);

	LogMessage("P25, network end of transmission, %.1f seconds, %u%% packet loss", float(m_netFrames) / 50.0F, (m_netLost * 100U) / m_netFrames);

	m_display->clearP25();
	m_netTimeout.stop();
	m_networkWatchdog.stop();
	m_netData.reset();
	m_netState = RS_NET_IDLE;
}

bool CP25Control::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "P25_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("P25", 1U, 3U, m_fp);

	return true;
}

bool CP25Control::writeFile(const unsigned char* data, unsigned char length)
{
	if (m_fp == NULL)
		return false;

	::fwrite(&length, 1U, 1U, m_fp);

	::fwrite(data, 1U, length, m_fp);

	return true;
}

void CP25Control::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}
