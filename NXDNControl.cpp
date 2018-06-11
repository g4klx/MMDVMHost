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
#include "NXDNFACCH1.h"
#include "NXDNSACCH.h"
#include "NXDNAudio.h"
#include "NXDNUDCH.h"
#include "AMBEFEC.h"
#include "Utils.h"
#include "Sync.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

const unsigned char SCRAMBLER[] = {
	0x00U, 0x00U, 0x00U, 0x82U, 0xA0U, 0x88U, 0x8AU, 0x00U, 0xA2U, 0xA8U, 0x82U, 0x8AU, 0x82U, 0x02U,
	0x20U, 0x08U, 0x8AU, 0x20U, 0xAAU, 0xA2U, 0x82U, 0x08U, 0x22U, 0x8AU, 0xAAU, 0x08U, 0x28U, 0x88U,
	0x28U, 0x28U, 0x00U, 0x0AU, 0x02U, 0x82U, 0x20U, 0x28U, 0x82U, 0x2AU, 0xAAU, 0x20U, 0x22U, 0x80U,
	0xA8U, 0x8AU, 0x08U, 0xA0U, 0xAAU, 0x02U };

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
m_rfErrs(0U),
m_rfBits(1U),
m_rfLastLICH(),
m_rfLayer3(),
m_netLayer3(),
m_rfMask(0x00U),
m_netMask(0x00U),
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
			LogMessage("NXDN, transmission lost, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 12.5F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
		else
			LogMessage("NXDN, transmission lost, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 12.5F, float(m_rfErrs * 100U) / float(m_rfBits));
		writeEndRF();
		return false;
	}

	if (type == TAG_LOST && m_rfState == RS_RF_DATA) {
		writeEndRF();
		return false;
	}

	if (type == TAG_LOST) {
		m_rfState = RS_RF_LISTENING;
		m_rfMask  = 0x00U;
		m_rfLayer3.reset();
		return false;
	}

	// Have we got RSSI bytes on the end?
	if (len == (NXDN_FRAME_LENGTH_BYTES + 4U)) {
		uint16_t raw = 0U;
		raw |= (data[50U] << 8) & 0xFF00U;
		raw |= (data[51U] << 0) & 0x00FFU;

		// Convert the raw RSSI to dBm
		int rssi = m_rssiMapper->interpolate(raw);
		if (rssi != 0)
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

	scrambler(data + 2U);

	CNXDNLICH lich;
	bool valid = lich.decode(data + 2U);

	if (valid)
		m_rfLastLICH = lich;

	// Stop repeater packets coming through, unless we're acting as a remote gateway
	if (m_remoteGateway) {
		unsigned char direction = m_rfLastLICH.getDirection();
		if (direction == NXDN_LICH_DIRECTION_INBOUND)
			return false;
	} else {
		unsigned char direction = m_rfLastLICH.getDirection();
		if (direction == NXDN_LICH_DIRECTION_OUTBOUND)
			return false;
	}

	unsigned char usc    = m_rfLastLICH.getFCT();
	unsigned char option = m_rfLastLICH.getOption();

	bool ret;
	if (usc == NXDN_LICH_USC_UDCH)
		ret = processData(option, data);
	else
		ret = processVoice(usc, option, data);

	return ret;
}

bool CNXDNControl::processVoice(unsigned char usc, unsigned char option, unsigned char *data)
{
	CNXDNSACCH sacch;
	bool valid = sacch.decode(data + 2U);
	if (valid) {
		unsigned char ran = sacch.getRAN();
		if (ran != m_ran && ran != 0U)
			return false;
	} else if (m_rfState == RS_RF_LISTENING) {
		return false;
	}

	unsigned char netData[40U];
	::memset(netData, 0x00U, 40U);

	if (usc == NXDN_LICH_USC_SACCH_NS) {
		// The SACCH on a non-superblock frame is usually an idle and not interesting apart from the RAN.
		CNXDNFACCH1 facch;
		bool valid = facch.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
		if (!valid)
			valid = facch.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
		if (!valid)
			return false;

		unsigned char buffer[10U];
		facch.getData(buffer);

		CNXDNLayer3 layer3;
		layer3.decode(buffer, NXDN_FACCH1_LENGTH_BITS);

		unsigned char type = layer3.getMessageType();
		if (type == NXDN_MESSAGE_TYPE_TX_REL) {
			if (m_rfState != RS_RF_AUDIO) {
				m_rfState = RS_RF_LISTENING;
				m_rfMask  = 0x00U;
				m_rfLayer3.reset();
				return false;
			}
		} else if (type == NXDN_MESSAGE_TYPE_VCALL) {
			if (m_rfState == RS_RF_LISTENING && m_selfOnly) {
				unsigned short srcId = layer3.getSourceUnitId();
				if (srcId != m_id) {
					m_rfState = RS_RF_REJECTED;
					return false;
				}
			}
		} else {
			return false;
		}

		m_rfLayer3 = layer3;

		data[0U] = type == NXDN_MESSAGE_TYPE_TX_REL ? TAG_EOT : TAG_DATA;
		data[1U] = 0x00U;

		CSync::addNXDNSync(data + 2U);

		CNXDNLICH lich;
		lich.setRFCT(NXDN_LICH_RFCT_RDCH);
		lich.setFCT(NXDN_LICH_USC_SACCH_NS);
		lich.setOption(NXDN_LICH_STEAL_FACCH);
		lich.setDirection(m_remoteGateway || !m_duplex ? NXDN_LICH_DIRECTION_INBOUND : NXDN_LICH_DIRECTION_OUTBOUND);
		lich.encode(data + 2U);

		lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
		netData[0U] = lich.getRaw();

		CNXDNSACCH sacch;
		sacch.setRAN(m_ran);
		sacch.setStructure(NXDN_SR_SINGLE);
		sacch.setData(SACCH_IDLE);
		sacch.encode(data + 2U);

		sacch.getRaw(netData + 1U);

		facch.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
		facch.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);

		facch.getRaw(netData + 5U + 0U);
		facch.getRaw(netData + 5U + 14U);

		scrambler(data + 2U);

		writeNetwork(netData, data[0U] == TAG_EOT ? NNMT_VOICE_TRAILER : NNMT_VOICE_HEADER);

#if defined(DUMP_NXDN)
		writeFile(data + 2U);
#endif
		if (m_duplex)
			writeQueueRF(data);

		if (data[0U] == TAG_EOT) {
			m_rfFrames++;
			if (m_rssi != 0U)
				LogMessage("NXDN, received RF end of transmission, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", float(m_rfFrames) / 12.5F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
			else
				LogMessage("NXDN, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 12.5F, float(m_rfErrs * 100U) / float(m_rfBits));
			writeEndRF();
		} else {
			m_rfFrames  = 0U;
			m_rfErrs    = 0U;
			m_rfBits    = 1U;
			m_rfTimeoutTimer.start();
			m_rfState   = RS_RF_AUDIO;
			m_minRSSI   = m_rssi;
			m_maxRSSI   = m_rssi;
			m_aveRSSI   = m_rssi;
			m_rssiCount = 1U;
#if defined(DUMP_NXDN)
			openFile();
#endif
			unsigned short srcId = m_rfLayer3.getSourceUnitId();
			unsigned short dstId = m_rfLayer3.getDestinationGroupId();
			bool grp             = m_rfLayer3.getIsGroup();

			std::string source = m_lookup->find(srcId);
			LogMessage("NXDN, received RF header from %s to %s%u", source.c_str(), grp ? "TG " : "", dstId);
			m_display->writeNXDN(source.c_str(), grp, dstId, "R");
		}

		return true;
	} else {
		if (m_rfState == RS_RF_LISTENING) {
			CNXDNFACCH1 facch;
			bool valid = false;
			switch (option) {
			case NXDN_LICH_STEAL_FACCH:
				valid = facch.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
				if (!valid)
					valid = facch.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
				break;
			case NXDN_LICH_STEAL_FACCH1_1:
				valid = facch.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
				break;
			case NXDN_LICH_STEAL_FACCH1_2:
				valid = facch.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
				break;
			default:
				break;
			}

			bool hasInfo = false;
			if (valid) {
				unsigned char buffer[10U];
				facch.getData(buffer);

				CNXDNLayer3 layer3;
				layer3.decode(buffer, NXDN_FACCH1_LENGTH_BITS);

				hasInfo = layer3.getMessageType() == NXDN_MESSAGE_TYPE_VCALL;
				if (!hasInfo)
					return false;

				m_rfLayer3 = layer3;
			}

			if (!hasInfo) {
				unsigned char message[3U];
				sacch.getData(message);

				unsigned char structure = sacch.getStructure();
				switch (structure) {
				case NXDN_SR_1_4:
					m_rfLayer3.decode(message, 18U, 0U);
					if(m_rfLayer3.getMessageType() == NXDN_MESSAGE_TYPE_VCALL)
						m_rfMask = 0x01U;
					else
						m_rfMask = 0x00U;
					break;
				case NXDN_SR_2_4:
					m_rfMask |= 0x02U;
					m_rfLayer3.decode(message, 18U, 18U);
					break;
				case NXDN_SR_3_4:
					m_rfMask |= 0x04U;
					m_rfLayer3.decode(message, 18U, 36U);
					break;
				case NXDN_SR_4_4:
					m_rfMask |= 0x08U;
					m_rfLayer3.decode(message, 18U, 54U);
					break;
				default:
					break;
				}

				if (m_rfMask != 0x0FU)
					return false;

				unsigned char type = m_rfLayer3.getMessageType();
				if (type != NXDN_MESSAGE_TYPE_VCALL)
					return false;
			}

			unsigned short srcId = m_rfLayer3.getSourceUnitId();
			unsigned short dstId = m_rfLayer3.getDestinationGroupId();
			bool grp = m_rfLayer3.getIsGroup();

			if (m_selfOnly) {
				if (srcId != m_id) {
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
			std::string source = m_lookup->find(srcId);
			LogMessage("NXDN, received RF late entry from %s to %s%u", source.c_str(), grp ? "TG " : "", dstId);
			m_display->writeNXDN(source.c_str(), grp, dstId, "R");

			m_rfState = RS_RF_AUDIO;

			// Create a dummy start message
			unsigned char start[NXDN_FRAME_LENGTH_BYTES + 2U];

			start[0U] = TAG_DATA;
			start[1U] = 0x00U;

			// Generate the sync
			CSync::addNXDNSync(start + 2U);

			// Generate the LICH
			CNXDNLICH lich;
			lich.setRFCT(NXDN_LICH_RFCT_RDCH);
			lich.setFCT(NXDN_LICH_USC_SACCH_NS);
			lich.setOption(NXDN_LICH_STEAL_FACCH);
			lich.setDirection(m_remoteGateway || !m_duplex ? NXDN_LICH_DIRECTION_INBOUND : NXDN_LICH_DIRECTION_OUTBOUND);
			lich.encode(start + 2U);

			lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
			netData[0U] = lich.getRaw();

			CNXDNSACCH sacch;
			sacch.setRAN(m_ran);
			sacch.setStructure(NXDN_SR_SINGLE);
			sacch.setData(SACCH_IDLE);
			sacch.encode(start + 2U);

			sacch.getRaw(netData + 1U);

			unsigned char message[22U];
			m_rfLayer3.getData(message);

			facch.setData(message);
			facch.encode(start + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
			facch.encode(start + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);

			facch.getRaw(netData + 5U + 0U);
			facch.getRaw(netData + 5U + 14U);

			scrambler(start + 2U);

			writeNetwork(netData, NNMT_VOICE_HEADER);

#if defined(DUMP_NXDN)
			writeFile(start + 2U);
#endif
			if (m_duplex)
				writeQueueRF(start);
		}
	}

	if (m_rfState == RS_RF_AUDIO) {
		// Regenerate the sync
		CSync::addNXDNSync(data + 2U);

		// Regenerate the LICH
		CNXDNLICH lich;
		lich.setRFCT(NXDN_LICH_RFCT_RDCH);
		lich.setFCT(NXDN_LICH_USC_SACCH_SS);
		lich.setOption(option);
		lich.setDirection(m_remoteGateway || !m_duplex ? NXDN_LICH_DIRECTION_INBOUND : NXDN_LICH_DIRECTION_OUTBOUND);
		lich.encode(data + 2U);

		lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
		netData[0U] = lich.getRaw();

		// Regenerate SACCH if it's valid
		CNXDNSACCH sacch;
		bool validSACCH = sacch.decode(data + 2U);
		if (validSACCH) {
			sacch.setRAN(m_ran);
			sacch.encode(data + 2U);
		}

		sacch.getRaw(netData + 1U);

		// Regenerate the audio and interpret the FACCH1 data
		if (option == NXDN_LICH_STEAL_NONE) {
			CAMBEFEC ambe;
			unsigned int errors = 0U;
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 0U);
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 9U);
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 18U);
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 27U);
			m_rfErrs += errors;
			m_rfBits += 188U;
			m_display->writeNXDNBER(float(errors) / 1.88F);
			LogDebug("NXDN, AMBE FEC %u/188 (%.1f%%)", errors, float(errors) / 1.88F);

			CNXDNAudio audio;
			audio.decode(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 0U,  netData + 5U + 0U);
			audio.decode(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 18U, netData + 5U + 14U);
		} else if (option == NXDN_LICH_STEAL_FACCH1_1) {
			CNXDNFACCH1 facch1;
			bool valid = facch1.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
			if (valid)
				facch1.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
			facch1.getRaw(netData + 5U + 0U);

			CAMBEFEC ambe;
			unsigned int errors = 0U;
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 18U);
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 27U);
			m_rfErrs += errors;
			m_rfBits += 94U;
			m_display->writeNXDNBER(float(errors) / 0.94F);
			LogDebug("NXDN, AMBE FEC %u/94 (%.1f%%)", errors, float(errors) / 0.94F);

			CNXDNAudio audio;
			audio.decode(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 18U, netData + 5U + 14U);
		} else if (option == NXDN_LICH_STEAL_FACCH1_2) {
			CAMBEFEC ambe;
			unsigned int errors = 0U;
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES);
			errors += ambe.regenerateYSFDN(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 9U);
			m_rfErrs += errors;
			m_rfBits += 94U;
			m_display->writeNXDNBER(float(errors) / 0.94F);
			LogDebug("NXDN, AMBE FEC %u/94 (%.1f%%)", errors, float(errors) / 0.94F);

			CNXDNAudio audio;
			audio.decode(data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 0U, netData + 5U + 0U);

			CNXDNFACCH1 facch1;
			bool valid = facch1.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
			if (valid)
				facch1.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
			facch1.getRaw(netData + 5U + 14U);
		} else {
			CNXDNFACCH1 facch11;
			bool valid1 = facch11.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
			if (valid1)
				facch11.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
			facch11.getRaw(netData + 5U + 0U);

			CNXDNFACCH1 facch12;
			bool valid2 = facch12.decode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
			if (valid2)
				facch12.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
			facch12.getRaw(netData + 5U + 14U);
		}

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		scrambler(data + 2U);

		writeNetwork(netData, NNMT_VOICE_BODY);

#if defined(DUMP_NXDN)
		writeFile(data + 2U);
#endif

		if (m_duplex)
			writeQueueRF(data);

		m_rfFrames++;

		m_display->writeNXDNRSSI(m_rssi);
	}

	return true;
}

bool CNXDNControl::processData(unsigned char option, unsigned char *data)
{
	CNXDNUDCH udch;
	bool validUDCH = udch.decode(data + 2U);
	if (m_rfState == RS_RF_LISTENING && !validUDCH)
		return false;

	if (validUDCH) {
		unsigned char ran = udch.getRAN();
		if (ran != m_ran && ran != 0U)
			return false;
	}

	unsigned char netData[40U];
	::memset(netData, 0x00U, 40U);

	// The layer3 data will only be correct if valid is true
	unsigned char buffer[23U];
	udch.getData(buffer);

	CNXDNLayer3 layer3;
	layer3.decode(buffer, 184U);

	if (m_rfState == RS_RF_LISTENING) {
		unsigned char type = layer3.getMessageType();
		if (type != NXDN_MESSAGE_TYPE_DCALL_HDR)
			return false;

		unsigned short srcId = layer3.getSourceUnitId();
		unsigned short dstId = layer3.getDestinationGroupId();
		bool grp             = layer3.getIsGroup();

		if (m_selfOnly) {
			if (srcId != m_id)
				return false;
		}

		unsigned char frames = layer3.getDataBlocks();

		std::string source = m_lookup->find(srcId);

		m_display->writeNXDN(source.c_str(), grp, dstId, "R");
		m_display->writeNXDNRSSI(m_rssi);

		LogMessage("NXDN, received RF data header from %s to %s%u, %u blocks", source.c_str(), grp ? "TG " : "", dstId, frames);

		m_rfLayer3 = layer3;
		m_rfFrames = 0U;

		m_rfState = RS_RF_DATA;

#if defined(DUMP_NXDN)
		openFile();
#endif
	}

	if (m_rfState != RS_RF_DATA)
		return false;

	CSync::addNXDNSync(data + 2U);

	CNXDNLICH lich;
	lich.setRFCT(NXDN_LICH_RFCT_RDCH);
	lich.setFCT(NXDN_LICH_USC_UDCH);
	lich.setOption(option);
	lich.setDirection(m_remoteGateway || !m_duplex ? NXDN_LICH_DIRECTION_INBOUND : NXDN_LICH_DIRECTION_OUTBOUND);
	lich.encode(data + 2U);

	lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
	netData[0U] = lich.getRaw();

	udch.getRaw(netData + 1U);

	unsigned char type = NXDN_MESSAGE_TYPE_DCALL_DATA;

	if (validUDCH) {
		type = layer3.getMessageType();
		data[0U] = type == NXDN_MESSAGE_TYPE_TX_REL ? TAG_EOT : TAG_DATA;

		udch.setRAN(m_ran);
		udch.encode(data + 2U);
	} else {
		data[0U] = TAG_DATA;
		data[1U] = 0x00U;
	}

	scrambler(data + 2U);

	switch (type) {
	case NXDN_MESSAGE_TYPE_DCALL_HDR:
		writeNetwork(netData, NNMT_DATA_HEADER);
		break;
	case NXDN_MESSAGE_TYPE_TX_REL:
		writeNetwork(netData, NNMT_DATA_TRAILER);
		break;
	default:
		writeNetwork(netData, NNMT_DATA_BODY);
		break;
	}

	if (m_duplex)
		writeQueueRF(data);

	m_rfFrames++;

#if defined(DUMP_NXDN)
	writeFile(data + 2U);
#endif

	if (data[0U] == TAG_EOT) {
		LogMessage("NXDN, ended RF data transmission");
		writeEndRF();
	}

	return true;
}

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

	m_rfMask = 0x00U;
	m_rfLayer3.reset();

	m_rfTimeoutTimer.stop();

	if (m_netState == RS_NET_IDLE) {
		m_display->clearNXDN();

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

	m_netMask = 0x00U;
	m_netLayer3.reset();

	m_netTimeoutTimer.stop();
	m_networkWatchdog.stop();
	m_packetTimer.stop();

	m_display->clearNXDN();

	if (m_network != NULL)
		m_network->reset();
}

void CNXDNControl::writeNetwork()
{
	unsigned char netData[40U];
	bool exists = m_network->read(netData);
	if (!exists)
		return;

	if (m_rfState != RS_RF_LISTENING && m_netState == RS_NET_IDLE)
		return;

	m_networkWatchdog.start();

	unsigned char data[NXDN_FRAME_LENGTH_BYTES + 2U];

	CSync::addNXDNSync(data + 2U);

	CNXDNLICH lich;
	lich.setRaw(netData[0U]);
	unsigned char usc    = lich.getFCT();
	unsigned char option = lich.getOption();
	lich.setDirection(m_remoteGateway || !m_duplex ? NXDN_LICH_DIRECTION_INBOUND : NXDN_LICH_DIRECTION_OUTBOUND);
	lich.encode(data + 2U);

	if (usc == NXDN_LICH_USC_UDCH) {
		CNXDNLayer3 layer3;
		layer3.setData(netData + 2U, 23U);
		unsigned char type = layer3.getMessageType();

		if (m_netState == RS_NET_IDLE) {
			if (type == NXDN_MESSAGE_TYPE_DCALL_HDR) {
				unsigned short srcId = layer3.getSourceUnitId();
				unsigned short dstId = layer3.getDestinationGroupId();
				bool grp             = layer3.getIsGroup();

				unsigned char frames = layer3.getDataBlocks();

				std::string source = m_lookup->find(srcId);
				m_display->writeNXDN(source.c_str(), grp, dstId, "N");
				LogMessage("NXDN, received network data header from %s to %s%u, %u blocks", source.c_str(), grp ? "TG " : "", dstId, frames);

				m_netState = RS_NET_DATA;
			} else {
				return;
			}
		}

		if (m_netState == RS_NET_DATA) {
			data[0U] = type == NXDN_MESSAGE_TYPE_TX_REL ? TAG_EOT : TAG_DATA;
			data[1U] = 0x00U;

			CNXDNUDCH udch;
			udch.setRAN(m_ran);
			udch.setData(netData + 2U);
			udch.encode(data + 2U);

			scrambler(data + 2U);

			writeQueueNet(data);

			if (type == NXDN_MESSAGE_TYPE_TX_REL) {
				LogMessage("NXDN, ended network data transmission");
				writeEndNet();
			}
		}
	} else if (usc == NXDN_LICH_USC_SACCH_NS) {
		m_netLayer3.setData(netData + 5U + 0U, 10U);

		unsigned char type = m_netLayer3.getMessageType();
		if (type == NXDN_MESSAGE_TYPE_TX_REL && m_netState == RS_NET_IDLE)
			return;
		if (type == NXDN_MESSAGE_TYPE_VCALL && m_netState != RS_NET_IDLE)
			return;

		CNXDNSACCH sacch;
		sacch.setRAN(m_ran);
		sacch.setStructure(NXDN_SR_SINGLE);
		sacch.setData(SACCH_IDLE);
		sacch.encode(data + 2U);

		CNXDNFACCH1 facch;
		facch.setRaw(netData + 5U + 0U);
		facch.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
		facch.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);

		data[0U] = type == NXDN_MESSAGE_TYPE_TX_REL ? TAG_EOT : TAG_DATA;
		data[1U] = 0x00U;

		scrambler(data + 2U);

		writeQueueNet(data);

		if (type == NXDN_MESSAGE_TYPE_TX_REL) {
			m_netFrames++;
			LogMessage("NXDN, received network end of transmission, %.1f seconds", float(m_netFrames) / 12.5F);
			writeEndNet();
		} else if (type == NXDN_MESSAGE_TYPE_VCALL) {
			unsigned short srcId = m_netLayer3.getSourceUnitId();
			unsigned short dstId = m_netLayer3.getDestinationGroupId();
			bool grp             = m_netLayer3.getIsGroup();

			std::string source = m_lookup->find(srcId);
			LogMessage("NXDN, received network transmission from %s to %s%u", source.c_str(), grp ? "TG " : "", dstId);
			m_display->writeNXDN(source.c_str(), grp, dstId, "N");

			m_netTimeoutTimer.start();
			m_packetTimer.start();
			m_elapsed.start();
			m_netState  = RS_NET_AUDIO;
			m_netFrames = 1U;
		} else {
			CUtils::dump(2U, "NXDN, interesting non superblock network frame", netData, 33U);
			return;
		}
	} else {
		if (m_netState == RS_NET_IDLE) {
			unsigned char structure = (netData[1U] >> 6) & 0x03U;
			switch (structure) {
			case NXDN_SR_1_4:
				m_netLayer3.decode(netData + 2U, 18U, 0U);
				if (m_netLayer3.getMessageType() == NXDN_MESSAGE_TYPE_VCALL)
					m_netMask = 0x01U;
				else
					m_netMask = 0x00U;
				break;
			case NXDN_SR_2_4:
				m_netMask |= 0x02U;
				m_netLayer3.decode(netData + 2U, 18U, 18U);
				break;
			case NXDN_SR_3_4:
				m_netMask |= 0x04U;
				m_netLayer3.decode(netData + 2U, 18U, 36U);
				break;
			case NXDN_SR_4_4:
				m_netMask |= 0x08U;
				m_netLayer3.decode(netData + 2U, 18U, 54U);
				break;
			default:
				break;
			}

			if (m_netMask != 0x0FU)
				return;

			unsigned char type = m_netLayer3.getMessageType();
			if (type != NXDN_MESSAGE_TYPE_VCALL)
				return;

			unsigned short srcId = m_netLayer3.getSourceUnitId();
			unsigned short dstId = m_netLayer3.getDestinationGroupId();
			bool grp             = m_netLayer3.getIsGroup();

			std::string source = m_lookup->find(srcId);
			LogMessage("NXDN, received network transmission from %s to %s%u", source.c_str(), grp ? "TG " : "", dstId);
			m_display->writeNXDN(source.c_str(), grp, dstId, "N");

			m_netTimeoutTimer.start();
			m_packetTimer.start();
			m_elapsed.start();
			m_netState  = RS_NET_AUDIO;
			m_netFrames = 1U;

			// Create a dummy start message
			unsigned char start[NXDN_FRAME_LENGTH_BYTES + 2U];

			start[0U] = TAG_DATA;
			start[1U] = 0x00U;

			// Generate the sync
			CSync::addNXDNSync(start + 2U);

			// Generate the LICH
			CNXDNLICH lich;
			lich.setRFCT(NXDN_LICH_RFCT_RDCH);
			lich.setFCT(NXDN_LICH_USC_SACCH_NS);
			lich.setOption(NXDN_LICH_STEAL_FACCH);
			lich.setDirection(m_remoteGateway || !m_duplex ? NXDN_LICH_DIRECTION_INBOUND : NXDN_LICH_DIRECTION_OUTBOUND);
			lich.encode(start + 2U);

			CNXDNSACCH sacch;
			sacch.setRAN(m_ran);
			sacch.setStructure(NXDN_SR_SINGLE);
			sacch.setData(SACCH_IDLE);
			sacch.encode(start + 2U);

			unsigned char message[22U];
			m_netLayer3.getData(message);

			CNXDNFACCH1 facch;
			facch.setData(message);
			facch.encode(start + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);
			facch.encode(start + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);

			scrambler(start + 2U);

			writeQueueNet(start);
		}

		m_netFrames++;

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		CNXDNSACCH sacch;
		sacch.setRaw(netData + 1U);
		sacch.setRAN(m_ran);
		sacch.encode(data + 2U);

		if (option == NXDN_LICH_STEAL_NONE) {
			CNXDNAudio audio;
			audio.encode(netData + 5U + 0U,  data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 0U);
			audio.encode(netData + 5U + 14U, data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 18U);
		} else if (option == NXDN_LICH_STEAL_FACCH1_1) {
			CNXDNFACCH1 facch1;
			facch1.setRaw(netData + 5U + 0U);
			facch1.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);

			CNXDNAudio audio;
			audio.encode(netData + 5U + 14U, data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 18U);
		} else if (option == NXDN_LICH_STEAL_FACCH1_2) {
			CNXDNAudio audio;
			audio.encode(netData + 5U + 0U, data + 2U + NXDN_FSW_LICH_SACCH_LENGTH_BYTES + 0U);

			CNXDNFACCH1 facch1;
			facch1.setRaw(netData + 5U + 14U);
			facch1.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
		} else {
			CNXDNFACCH1 facch11;
			facch11.setRaw(netData + 5U + 0U);
			facch11.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS);

			CNXDNFACCH1 facch12;
			facch12.setRaw(netData + 5U + 14U);
			facch12.encode(data + 2U, NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS + NXDN_FACCH1_LENGTH_BITS);
		}

		scrambler(data + 2U);

		writeQueueNet(data);
	}
}

void CNXDNControl::clock(unsigned int ms)
{
	if (m_network != NULL)
		writeNetwork();

	m_rfTimeoutTimer.clock(ms);
	m_netTimeoutTimer.clock(ms);

	if (m_netState == RS_NET_AUDIO) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			LogMessage("NXDN, network watchdog has expired, %.1f seconds", float(m_netFrames) / 12.5F);
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

void CNXDNControl::writeNetwork(const unsigned char *data, NXDN_NETWORK_MESSAGE_TYPE type)
{
	assert(data != NULL);

	if (m_network == NULL)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	m_network->write(data, type);
}

void CNXDNControl::scrambler(unsigned char* data) const
{
	assert(data != NULL);

	for (unsigned int i = 0U; i < NXDN_FRAME_LENGTH_BYTES; i++)
		data[i] ^= SCRAMBLER[i];
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

	::fwrite("NXDN", 1U, 4U, m_fp);

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
