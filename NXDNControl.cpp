/*
 *	Copyright (C) 2015,2016,2017,2018 Jonathan Naylor, G4KLX
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

#include "NXDNControl.h"
#include "Utils.h"
#include "Sync.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

const unsigned char SCRAMBLER[] = {
	0x00U, 0x09U, 0xCAU, 0xB0U, 0xDEU, 0x9BU, 0x91U, 0x42U,
	0xB4U, 0xFDU, 0x92U, 0x5BU, 0xF2U, 0x6AU, 0x66U, 0x03U,
	0x19U, 0x46U, 0x97U, 0xF4U, 0x58U, 0xEBU, 0x2CU, 0xF1U,
	0xF7U};

// #define	DUMP_NXDN

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNControl::CNXDNControl(unsigned int ran, unsigned int id, bool selfOnly, CNXDNNetwork* network, CDisplay* display, unsigned int timeout, bool duplex, bool remoteGateway, CNXDNLookup* lookup, CRSSIInterpolator* rssiMapper) :
m_ran(ran),
m_id(id),
m_selfOnly(selfOnly),
m_network(network),
m_display(display),
m_duplex(duplex),
m_remoteGateway(remoteGateway),
m_lookup(lookup),
m_queue(5000U, "NXDN Control"),
m_rfState(RS_RF_LISTENING),
m_netState(RS_NET_IDLE),
m_rfTimeoutTimer(1000U, timeout),
m_netTimeoutTimer(1000U, timeout),
m_packetTimer(1000U, 0U, 200U),
m_networkWatchdog(1000U, 0U, 1500U),
m_elapsed(),
m_rfFrames(0U),
m_netFrames(0U),
m_netLost(0U),
m_rfErrs(0U),
m_rfBits(1U),
m_netErrs(0U),
m_netBits(1U),
m_lastLICH(),
m_netN(0U),
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
}

CNXDNControl::~CNXDNControl()
{
}

bool CNXDNControl::writeModem(unsigned char *data, unsigned int len)
{
	assert(data != NULL);

	unsigned char type = data[0U];

	if (type == TAG_LOST && m_rfState == RS_RF_AUDIO) {
		if (m_rssi != 0U)
			LogMessage("NXDN, transmission lost, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 10.0F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
		else
			LogMessage("NXDN, transmission lost, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 10.0F, float(m_rfErrs * 100U) / float(m_rfBits));
		writeEndRF();
		return false;
	}

	if (type == TAG_LOST && m_rfState == RS_RF_REJECTED) {
		m_rfState  = RS_RF_LISTENING;
		return false;
	}

	if (type == TAG_LOST) {
		m_rfState = RS_RF_LISTENING;
		return false;
	}

	// Have we got RSSI bytes on the end?
	if (len == (NXDN_FRAME_LENGTH_BYTES + 4U)) {
		uint16_t raw = 0U;
		raw |= (data[50U] << 8) & 0xFF00U;
		raw |= (data[51U] << 0) & 0x00FFU;

		// Convert the raw RSSI to dBm
		int rssi = m_rssiMapper->interpolate(raw);
		LogDebug("NXDN, raw RSSI: %u, reported RSSI: %d dBm", raw, rssi);

		// RSSI is always reported as positive
		m_rssi = (rssi >= 0) ? rssi : -rssi;

		if (m_rssi > m_minRSSI)
			m_minRSSI = m_rssi;
		if (m_rssi < m_maxRSSI)
			m_maxRSSI = m_rssi;

		m_aveRSSI += m_rssi;
		m_rssiCount++;
	}

	CUtils::dump(2U, "NXDN, raw data", data + 2U, NXDN_FRAME_LENGTH_BYTES);

	scrambler(data + 2U);

	CUtils::dump(2U, "NXDN, after descrambling", data + 2U, NXDN_FRAME_LENGTH_BYTES);

	CNXDNLICH lich;
	bool valid = lich.decode(data + 2U);

	if (valid)
		m_lastLICH = lich;

	// Stop repeater packets coming through, unless we're acting as a remote gateway
	if (m_remoteGateway) {
		unsigned char direction = m_lastLICH.getDirection();
		if (direction == NXDN_LICH_DIRECTION_INBOUND)
			return false;
	} else {
		unsigned char direction = m_lastLICH.getDirection();
		if (direction == NXDN_LICH_DIRECTION_OUTBOUND)
			return false;
	}

	unsigned char usc    = m_lastLICH.getFCT();
	unsigned char option = m_lastLICH.getOption();

	bool ret = false;
#ifdef notdef
	if (usc == NXDN_LICH_USC_UDCH) {
		ret = processData(option, data);
	else
		ret = processVoice(usc, option, data);
#endif
	return ret;
}

#ifdef notdef
bool CNXDNControl::processVWData(bool valid, unsigned char *data)
{
	unsigned char fi = m_lastFICH.getFI();
	if (valid && fi == YSF_FI_HEADER) {
		if (m_rfState == RS_RF_LISTENING) {
			bool valid = m_rfPayload.processHeaderData(data + 2U);
			if (!valid)
				return false;

			m_rfSource = m_rfPayload.getSource();

			if (m_selfOnly) {
				bool ret = checkCallsign(m_rfSource);
				if (!ret) {
					LogMessage("NXDN, invalid access attempt from %10.10s", m_rfSource);
					m_rfState = RS_RF_REJECTED;
					return false;
				}
			}

			unsigned char cm = m_lastFICH.getCM();
			if (cm == YSF_CM_GROUP1 || cm == YSF_CM_GROUP2)
				m_rfDest = (unsigned char*)"ALL       ";
			else
				m_rfDest = m_rfPayload.getDest();

			m_rfFrames = 0U;
			m_rfErrs = 0U;
			m_rfBits = 1U;
			m_rfTimeoutTimer.start();
			m_rfState = RS_RF_AUDIO;

			m_minRSSI = m_rssi;
			m_maxRSSI = m_rssi;
			m_aveRSSI = m_rssi;
			m_rssiCount = 1U;
#if defined(DUMP_NXDN)
			openFile();
#endif

			m_display->writeFusion((char*)m_rfSource, (char*)m_rfDest, "R", "          ");
			LogMessage("NXDN, received RF header from %10.10s to %10.10s", m_rfSource, m_rfDest);

			CSync::addNXDNSync(data + 2U);

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

			m_rfFrames++;

			m_display->writeFusionRSSI(m_rssi);

			return true;
		}
	} else if (valid && fi == YSF_FI_TERMINATOR) {
		if (m_rfState == RS_RF_REJECTED) {
			m_rfPayload.reset();
			m_rfSource = NULL;
			m_rfDest   = NULL;
			m_rfState  = RS_RF_LISTENING;
		} else if (m_rfState == RS_RF_AUDIO) {
			m_rfPayload.processHeaderData(data + 2U);

			CSync::addNXDNSync(data + 2U);

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

			m_rfFrames++;

			if (m_rssi != 0U)
				LogMessage("NXDN, received RF end of transmission, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 10.0F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
			else
				LogMessage("NXDN, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 10.0F, float(m_rfErrs * 100U) / float(m_rfBits));

			writeEndRF();
		}
	} else {
		if (m_rfState == RS_RF_AUDIO) {
			// If valid is false, update the m_lastFICH for this transmission
			if (!valid) {
				// XXX Check these values
				m_lastFICH.setFT(0U);
				m_lastFICH.setFN(0U);
			}

			CSync::addNXDNSync(data + 2U);

			CYSFFICH fich = m_lastFICH;

			unsigned char fn = fich.getFN();
			unsigned char ft = fich.getFT();

			if (fn != 0U || ft != 1U) {
				// The first packet after the header is odd, don't try and regenerate it
				unsigned int errors = m_rfPayload.processVoiceFRModeAudio(data + 2U);
				m_rfErrs += errors;
				m_rfBits += 720U;
				m_display->writeFusionBER(float(errors) / 7.2F);
				LogDebug("NXDN, V Mode 3, seq %u, AMBE FEC %u/720 (%.1f%%)", m_rfFrames % 128, errors, float(errors) / 7.2F);
			}

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			m_rfFrames++;

			m_display->writeFusionRSSI(m_rssi);

			return true;
		}
	}

	return false;
}

bool CNXDNControl::processDNData(bool valid, unsigned char *data)
{
	unsigned char fi = m_lastFICH.getFI();
	if (valid && fi == YSF_FI_HEADER) {
		if (m_rfState == RS_RF_LISTENING) {
			bool valid = m_rfPayload.processHeaderData(data + 2U);
			if (!valid)
				return false;

			m_rfSource = m_rfPayload.getSource();

			if (m_selfOnly) {
				bool ret = checkCallsign(m_rfSource);
				if (!ret) {
					LogMessage("NXDN, invalid access attempt from %10.10s", m_rfSource);
					m_rfState = RS_RF_REJECTED;
					return false;
				}
			}

			unsigned char cm = m_lastFICH.getCM();
			if (cm == YSF_CM_GROUP1 || cm == YSF_CM_GROUP2)
				m_rfDest = (unsigned char*)"ALL       ";
			else
				m_rfDest = m_rfPayload.getDest();

			m_rfFrames = 0U;
			m_rfErrs = 0U;
			m_rfBits = 1U;
			m_rfTimeoutTimer.start();
			m_rfState = RS_RF_AUDIO;

			m_minRSSI = m_rssi;
			m_maxRSSI = m_rssi;
			m_aveRSSI = m_rssi;
			m_rssiCount = 1U;
#if defined(DUMP_NXDN)
			openFile();
#endif

			m_display->writeFusion((char*)m_rfSource, (char*)m_rfDest, "R", "          ");
			LogMessage("NXDN, received RF header from %10.10s to %10.10s", m_rfSource, m_rfDest);

			CSync::addNXDNSync(data + 2U);

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

			m_rfFrames++;

			m_display->writeFusionRSSI(m_rssi);

			return true;
		}
	} else if (valid && fi == YSF_FI_TERMINATOR) {
		if (m_rfState == RS_RF_REJECTED) {
			m_rfPayload.reset();
			m_rfSource = NULL;
			m_rfDest   = NULL;
			m_rfState  = RS_RF_LISTENING;
		} else if (m_rfState == RS_RF_AUDIO) {
			m_rfPayload.processHeaderData(data + 2U);

			CSync::addNXDNSync(data + 2U);

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

			m_rfFrames++;

			if (m_rssi != 0U)
				LogMessage("NXDN, received RF end of transmission, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 10.0F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
			else
				LogMessage("NXDN, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 10.0F, float(m_rfErrs * 100U) / float(m_rfBits));

			writeEndRF();
		}
	} else {
		if (m_rfState == RS_RF_AUDIO) {
			// If valid is false, update the m_lastFICH for this transmission
			if (!valid) {
				unsigned char ft = m_lastFICH.getFT();
				unsigned char fn = m_lastFICH.getFN() + 1U;

				if (fn > ft)
					fn = 0U;

				m_lastFICH.setFN(fn);
			}

			CSync::addNXDNSync(data + 2U);

			unsigned char fn = m_lastFICH.getFN();
			unsigned char dt = m_lastFICH.getDT();

			switch (dt) {
			case YSF_DT_VD_MODE1: {
					m_rfPayload.processVDMode1Data(data + 2U, fn);
					unsigned int errors = m_rfPayload.processVDMode1Audio(data + 2U);
					m_rfErrs += errors;
					m_rfBits += 235U;
					m_display->writeFusionBER(float(errors) / 2.35F);
					LogDebug("NXDN, V/D Mode 1, seq %u, AMBE FEC %u/235 (%.1f%%)", m_rfFrames % 128, errors, float(errors) / 2.35F);
				}
				break;

			case YSF_DT_VD_MODE2: {
					m_rfPayload.processVDMode2Data(data + 2U, fn);
					unsigned int errors = m_rfPayload.processVDMode2Audio(data + 2U);
					m_rfErrs += errors;
					m_rfBits += 135U;
					m_display->writeFusionBER(float(errors) / 1.35F);
					LogDebug("NXDN, V/D Mode 2, seq %u, Repetition FEC %u/135 (%.1f%%)", m_rfFrames % 128, errors, float(errors) / 1.35F);
				}
				break;

			default:
				break;
			}

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			m_rfFrames++;

			m_display->writeFusionRSSI(m_rssi);

			return true;
		} else if (valid && m_rfState == RS_RF_LISTENING) {
			// Only use clean frames for late entry.
			unsigned char fn = m_lastFICH.getFN();
			unsigned char dt = m_lastFICH.getDT();

			switch (dt) {
			case YSF_DT_VD_MODE1:
				valid = m_rfPayload.processVDMode1Data(data + 2U, fn);
				break;

			case YSF_DT_VD_MODE2:
				valid = m_rfPayload.processVDMode2Data(data + 2U, fn);
				break;

			default:
				valid = false;
				break;
			}

			if (!valid)
				return false;

			unsigned char cm = m_lastFICH.getCM();
			if (cm == YSF_CM_GROUP1 || cm == YSF_CM_GROUP2)
				m_rfDest = (unsigned char*)"ALL       ";
			else
				m_rfDest = m_rfPayload.getDest();

			m_rfSource = m_rfPayload.getSource();

			if (m_rfSource == NULL || m_rfDest == NULL)
				return false;

			if (m_selfOnly) {
				bool ret = checkCallsign(m_rfSource);
				if (!ret) {
					LogMessage("NXDN, invalid access attempt from %10.10s", m_rfSource);
					m_rfState = RS_RF_REJECTED;
					return false;
				}
			}

			m_rfFrames = 0U;
			m_rfErrs = 0U;
			m_rfBits = 1U;
			m_rfTimeoutTimer.start();
			m_rfState = RS_RF_AUDIO;

			m_minRSSI = m_rssi;
			m_maxRSSI = m_rssi;
			m_aveRSSI = m_rssi;
			m_rssiCount = 1U;
#if defined(DUMP_NXDN)
			openFile();
#endif

			// Build a new header and transmit it
			unsigned char buffer[YSF_FRAME_LENGTH_BYTES + 2U];

			CSync::addNXDNSync(buffer + 2U);

			CYSFFICH fich = m_lastFICH;
			fich.setFI(YSF_FI_HEADER);
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(buffer + 2U);

			unsigned char csd1[20U], csd2[20U];
			memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_rfSource, YSF_CALLSIGN_LENGTH);
			memset(csd2, ' ', YSF_CALLSIGN_LENGTH + YSF_CALLSIGN_LENGTH);

			if (cm == YSF_CM_GROUP1 || cm == YSF_CM_GROUP2)
				memset(csd1 + 0U, '*', YSF_CALLSIGN_LENGTH);
			else
				memcpy(csd1 + 0U, m_rfDest, YSF_CALLSIGN_LENGTH);

			CYSFPayload payload;
			payload.writeHeader(buffer + 2U, csd1, csd2);

			buffer[0U] = TAG_DATA;
			buffer[1U] = 0x00U;

			writeNetwork(buffer, m_rfFrames % 128U);

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(buffer + 2U);
				writeQueueRF(buffer);
			}

#if defined(DUMP_NXDN)
			writeFile(buffer + 2U);
#endif

			m_display->writeFusion((char*)m_rfSource, (char*)m_rfDest, "R", "          ");
			LogMessage("NXDN, received RF late entry from %10.10s to %10.10s", m_rfSource, m_rfDest);

			CSync::addNXDNSync(data + 2U);

			fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			m_rfFrames++;

			m_display->writeFusionRSSI(m_rssi);

			return true;
		}
	}

	return false;
}

bool CNXDNControl::processFRData(bool valid, unsigned char *data)
{
	unsigned char fi = m_lastFICH.getFI();
	if (valid && fi == YSF_FI_HEADER) {
		if (m_rfState == RS_RF_LISTENING) {
			valid = m_rfPayload.processHeaderData(data + 2U);
			if (!valid)
				return false;

			m_rfSource = m_rfPayload.getSource();

			if (m_selfOnly) {
				bool ret = checkCallsign(m_rfSource);
				if (!ret) {
					LogMessage("NXDN, invalid access attempt from %10.10s", m_rfSource);
					m_rfState = RS_RF_REJECTED;
					return false;
				}
			}

			unsigned char cm = m_lastFICH.getCM();
			if (cm == YSF_CM_GROUP1 || cm == YSF_CM_GROUP2)
				m_rfDest = (unsigned char*)"ALL       ";
			else
				m_rfDest = m_rfPayload.getDest();

			m_rfFrames = 0U;
			m_rfState = RS_RF_DATA;

			m_minRSSI = m_rssi;
			m_maxRSSI = m_rssi;
			m_aveRSSI = m_rssi;
			m_rssiCount = 1U;
#if defined(DUMP_NXDN)
			openFile();
#endif

			m_display->writeFusion((char*)m_rfSource, (char*)m_rfDest, "R", "          ");
			LogMessage("NXDN, received RF header from %10.10s to %10.10s", m_rfSource, m_rfDest);

			CSync::addNXDNSync(data + 2U);

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

			m_rfFrames++;

			m_display->writeFusionRSSI(m_rssi);

			return true;
		}
	} else if (valid && fi == YSF_FI_TERMINATOR) {
		if (m_rfState == RS_RF_REJECTED) {
			m_rfPayload.reset();
			m_rfSource = NULL;
			m_rfDest   = NULL;
			m_rfState  = RS_RF_LISTENING;
		} else if (m_rfState == RS_RF_DATA) {
			m_rfPayload.processHeaderData(data + 2U);

			CSync::addNXDNSync(data + 2U);

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

			m_rfFrames++;

			if (m_rssi != 0U)
				LogMessage("NXDN, received RF end of transmission, %.1f seconds, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 10.0F, m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
			else
				LogMessage("NXDN, received RF end of transmission, %.1f seconds", float(m_rfFrames) / 10.0F);

			writeEndRF();
		}
	} else {
		if (m_rfState == RS_RF_DATA) {
			// If valid is false, update the m_lastFICH for this transmission
			if (!valid) {
				unsigned char ft = m_lastFICH.getFT();
				unsigned char fn = m_lastFICH.getFN() + 1U;

				if (fn > ft)
					fn = 0U;

				m_lastFICH.setFN(fn);
			}

			CSync::addNXDNSync(data + 2U);

			unsigned char fn = m_lastFICH.getFN();

			m_rfPayload.processDataFRModeData(data + 2U, fn);

			CYSFFICH fich = m_lastFICH;

			// Remove any DSQ information
			fich.setSQL(false);
			fich.setSQ(0U);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;

			writeNetwork(data, m_rfFrames % 128U);

			if (m_duplex) {
				fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
				fich.encode(data + 2U);
				writeQueueRF(data);
			}

#if defined(DUMP_NXDN)
			writeFile(data + 2U);
#endif

			m_rfFrames++;

			m_display->writeFusionRSSI(m_rssi);

			return true;
		}
	}

	return false;
}

#endif

unsigned int CNXDNControl::readModem(unsigned char* data)
{
	assert(data != NULL);

	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CNXDNControl::writeEndRF()
{
	m_rfState = RS_RF_LISTENING;

	m_rfTimeoutTimer.stop();

	if (m_netState == RS_NET_IDLE) {
		m_display->clearFusion();

		if (m_network != NULL)
			m_network->reset();
	}

#if defined(DUMP_NXDN)
	closeFile();
#endif
}

void CNXDNControl::writeEndNet()
{
	m_netState = RS_NET_IDLE;

	m_netTimeoutTimer.stop();
	m_networkWatchdog.stop();
	m_packetTimer.stop();

	m_display->clearFusion();

	if (m_network != NULL)
		m_network->reset();
}

#ifdef notdef

void CNXDNControl::writeNetwork()
{
	unsigned char data[200U];
	unsigned int length = m_network->read(data);
	if (length == 0U)
		return;

	if (m_rfState != RS_RF_LISTENING && m_netState == RS_NET_IDLE)
		return;

	m_networkWatchdog.start();

	unsigned char n = (data[5U] & 0xFEU) >> 1;
	bool end = (data[5U] & 0x01U) == 0x01U;

	if (!m_netTimeoutTimer.isRunning()) {
		if (end)
			return;

		m_display->writeFusion((char*)m_netSource, (char*)m_netDest, "N", (char*)(data + 4U));
		LogMessage("NXDN, received network data from %10.10s to %10.10s at %10.10s", m_netSource, m_netDest, data + 4U);

		m_netTimeoutTimer.start();
		m_packetTimer.start();
		m_elapsed.start();
		m_netState  = RS_NET_AUDIO;
		m_netFrames = 0U;
		m_netLost   = 0U;
		m_netErrs   = 0U;
		m_netBits   = 1U;
		m_netN      = 0U;
	} else {
		// Check for duplicate frames, if we can
		if (m_netN == n)
			return;

		bool changed = false;

		if (::memcmp(data + 14U, "          ", YSF_CALLSIGN_LENGTH) != 0 && ::memcmp(m_netSource, "??????????", YSF_CALLSIGN_LENGTH) == 0) {
			::memcpy(m_netSource, data + 14U, YSF_CALLSIGN_LENGTH);
			changed = true;
		}

		if (::memcmp(data + 24U, "          ", YSF_CALLSIGN_LENGTH) != 0 && ::memcmp(m_netDest, "??????????", YSF_CALLSIGN_LENGTH) == 0) {
			::memcpy(m_netDest, data + 24U, YSF_CALLSIGN_LENGTH);
			changed = true;
		}

		if (changed) {
			m_display->writeFusion((char*)m_netSource, (char*)m_netDest, "N", (char*)(data + 4U));
			LogMessage("YSF, received network data from %10.10s to %10.10s at %10.10s", m_netSource, m_netDest, data + 4U);
		}
	}

	data[33U] = end ? TAG_EOT : TAG_DATA;
	data[34U] = 0x00U;

	CYSFFICH fich;
	bool valid = fich.decode(data + 35U);
	if (valid) {
		unsigned char dt = fich.getDT();
		unsigned char fn = fich.getFN();
		unsigned char ft = fich.getFT();
		unsigned char fi = fich.getFI();

		fich.setVoIP(true);
		fich.setMR(m_remoteGateway ? YSF_MR_NOT_BUSY : YSF_MR_BUSY);
		fich.encode(data + 35U);

		// Set the downlink callsign
		switch (fi) {
		case YSF_FI_HEADER:
		case YSF_FI_TERMINATOR:
			m_netPayload.processHeaderData(data + 35U);
			break;

		case YSF_FI_COMMUNICATIONS:
			switch (dt) {
			case YSF_DT_VD_MODE1: {
					m_netPayload.processVDMode1Data(data + 35U, fn, gateway);
					unsigned int errors = m_netPayload.processVDMode1Audio(data + 35U);
					m_netErrs += errors;
					m_netBits += 235U;
				}
				break;

			case YSF_DT_VD_MODE2: {
					m_netPayload.processVDMode2Data(data + 35U, fn, gateway);
					unsigned int errors = m_netPayload.processVDMode2Audio(data + 35U);
					m_netErrs += errors;
					m_netBits += 135U;
				}
				break;

			case YSF_DT_DATA_FR_MODE:
				m_netPayload.processDataFRModeData(data + 35U, fn, gateway);
				break;

			case YSF_DT_VOICE_FR_MODE:
				if (fn != 0U || ft != 1U) {
					// The first packet after the header is odd, don't try and regenerate it
					unsigned int errors = m_netPayload.processVoiceFRModeAudio(data + 35U);
					m_netErrs += errors;
					m_netBits += 720U;
				}
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
	}

	writeQueueNet(data + 33U);

	m_packetTimer.start();
	m_netFrames++;
	m_netN = n;

	if (end) {
		LogMessage("NXDN, received network end of transmission, %.1f seconds, %u%% packet loss, BER: %.1f%%", float(m_netFrames) / 10.0F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));
		writeEndNet();
	}
}

#endif

void CNXDNControl::clock(unsigned int ms)
{
#ifdef notdef
	if (m_network != NULL)
		writeNetwork();
#endif

	m_rfTimeoutTimer.clock(ms);
	m_netTimeoutTimer.clock(ms);

	if (m_netState == RS_NET_AUDIO) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			LogMessage("NXDN, network watchdog has expired, %.1f seconds, %u%% packet loss, BER: %.1f%%", float(m_netFrames) / 10.0F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));
			writeEndNet();
		}
	}
}

void CNXDNControl::writeQueueRF(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netState != RS_NET_IDLE)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	unsigned char len = NXDN_FRAME_LENGTH_BYTES + 2U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("NXDN, overflow in the NXDN RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CNXDNControl::writeQueueNet(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired())
		return;

	unsigned char len = NXDN_FRAME_LENGTH_BYTES + 2U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("NXDN, overflow in the NXDN RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CNXDNControl::writeNetwork(const unsigned char *data, unsigned int count)
{
	assert(data != NULL);

	if (m_network == NULL)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	m_network->write(data + 2U, count, data[0U] == TAG_EOT);
}

void CNXDNControl::scrambler(unsigned char* data) const
{
	assert(data != NULL);

	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < NXDN_FRAME_LENGTH_SYMBOLS; i++, offset += 2U) {
		bool invert = READ_BIT1(SCRAMBLER, i);
		if (invert) {
			unsigned int offset1 = offset + 0U;
			unsigned int offset2 = offset + 1U;

			bool b1 = READ_BIT1(data, offset1);
			bool b2 = READ_BIT1(data, offset2);

			if (b1 && b2) {
				// -3 to +3, 11 to 01
				WRITE_BIT1(data, offset1, false);
				WRITE_BIT1(data, offset2, true);
			} else if (b1 && !b2) {
				// -1 to +1, 10 to 00
				WRITE_BIT1(data, offset1, false);
				WRITE_BIT1(data, offset2, false);
			} else if (!b1 && b2) {
				// +3 to -3, 01 to 11
				WRITE_BIT1(data, offset1, true);
				WRITE_BIT1(data, offset2, true);
			} else {
				// +1 to -1, 00 to 10
				WRITE_BIT1(data, offset1, true);
				WRITE_BIT1(data, offset2, false);
			}
		}
	}
}

bool CNXDNControl::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "NXDN_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("NXDN", 1U, 3U, m_fp);

	return true;
}

bool CNXDNControl::writeFile(const unsigned char* data)
{
	if (m_fp == NULL)
		return false;

	::fwrite(data, 1U, NXDN_FRAME_LENGTH_BYTES, m_fp);

	return true;
}

void CNXDNControl::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}
