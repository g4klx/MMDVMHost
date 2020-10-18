/*
 *	Copyright (C) 2015-2020 Jonathan Naylor, G4KLX
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

#include "M17Control.h"
#include "M17Convolution.h"
#include "M17CRC.h"
#include "Golay24128.h"
#include "Utils.h"
#include "Sync.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

const unsigned int INTERLEAVER[] = {
	0U, 137U, 90U, 227U, 180U, 317U, 270U, 39U, 360U, 129U, 82U, 219U, 172U, 309U, 262U, 31U, 352U, 121U, 74U, 211U, 164U,
	301U, 254U, 23U, 344U, 113U, 66U, 203U, 156U, 293U, 246U, 15U, 336U, 105U, 58U, 195U, 148U, 285U, 238U, 7U, 328U, 97U,
	50U, 187U, 140U, 277U, 230U, 367U, 320U, 89U, 42U, 179U, 132U, 269U, 222U, 359U, 312U, 81U, 34U, 171U, 124U, 261U, 214U,
	351U, 304U, 73U, 26U, 163U, 116U, 253U, 206U, 343U, 296U, 65U, 18U, 155U, 108U, 245U, 198U, 335U, 288U, 57U, 10U, 147U,
	100U, 237U, 190U, 327U, 280U, 49U, 2U, 139U, 92U, 229U, 182U, 319U, 272U, 41U, 362U, 131U, 84U, 221U, 174U, 311U, 264U,
	33U, 354U, 123U, 76U, 213U, 166U, 303U, 256U, 25U, 346U, 115U, 68U, 205U, 158U, 295U, 248U, 17U, 338U, 107U, 60U, 197U,
	150U, 287U, 240U, 9U, 330U, 99U, 52U, 189U, 142U, 279U, 232U, 1U, 322U, 91U, 44U, 181U, 134U, 271U, 224U, 361U, 314U, 83U,
	36U, 173U, 126U, 263U, 216U, 353U, 306U, 75U, 28U, 165U, 118U, 255U, 208U, 345U, 298U, 67U, 20U, 157U, 110U, 247U, 200U,
	337U, 290U, 59U, 12U, 149U, 102U, 239U, 192U, 329U, 282U, 51U, 4U, 141U, 94U, 231U, 184U, 321U, 274U, 43U, 364U, 133U, 86U,
	223U, 176U, 313U, 266U, 35U, 356U, 125U, 78U, 215U, 168U, 305U, 258U, 27U, 348U, 117U, 70U, 207U, 160U, 297U, 250U, 19U,
	340U, 109U, 62U, 199U, 152U, 289U, 242U, 11U, 332U, 101U, 54U, 191U, 144U, 281U, 234U, 3U, 324U, 93U, 46U, 183U, 136U, 273U,
	226U, 363U, 316U, 85U, 38U, 175U, 128U, 265U, 218U, 355U, 308U, 77U, 30U, 167U, 120U, 257U, 210U, 347U, 300U, 69U, 22U,
	159U, 112U, 249U, 202U, 339U, 292U, 61U, 14U, 151U, 104U, 241U, 194U, 331U, 284U, 53U, 6U, 143U, 96U, 233U, 186U, 323U,
	276U, 45U, 366U, 135U, 88U, 225U, 178U, 315U, 268U, 37U, 358U, 127U, 80U, 217U, 170U, 307U, 260U, 29U, 350U, 119U, 72U,
	209U, 162U, 299U, 252U, 21U, 342U, 111U, 64U, 201U, 154U, 291U, 244U, 13U, 334U, 103U, 56U, 193U, 146U, 283U, 236U, 5U,
	326U, 95U, 48U, 185U, 138U, 275U, 228U, 365U, 318U, 87U, 40U, 177U, 130U, 267U, 220U, 357U, 310U, 79U, 32U, 169U, 122U,
	259U, 212U, 349U, 302U, 71U, 24U, 161U, 114U, 251U, 204U, 341U, 294U, 63U, 16U, 153U, 106U, 243U, 196U, 333U, 286U, 55U,
	8U, 145U, 98U, 235U, 188U, 325U, 278U, 47U};

const unsigned char SCRAMBLER[] = {
	0x00U, 0x00U, 0xD6U, 0xB5U, 0xE2U, 0x30U, 0x82U, 0xFFU, 0x84U, 0x62U, 0xBAU, 0x4EU, 0x96U, 0x90U, 0xD8U, 0x98U, 0xDDU,
	0x5DU, 0x0CU, 0xC8U, 0x52U, 0x43U, 0x91U, 0x1DU, 0xF8U, 0x6EU, 0x68U, 0x2FU, 0x35U, 0xDAU, 0x14U, 0xEAU, 0xCDU, 0x76U,
	0x19U, 0x8DU, 0xD5U, 0x80U, 0xD1U, 0x33U, 0x87U, 0x13U, 0x57U, 0x18U, 0x2DU, 0x29U, 0x78U, 0xC3U};

// #define	DUMP_M17

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CM17Control::CM17Control(const std::string& callsign, bool selfOnly, CM17Network* network, CDisplay* display, unsigned int timeout, bool duplex, CRSSIInterpolator* rssiMapper) :
m_callsign(callsign),
m_selfOnly(selfOnly),
m_network(network),
m_display(display),
m_duplex(duplex),
m_queue(5000U, "M17 Control"),
m_rfState(RS_RF_LISTENING),
m_netState(RS_NET_IDLE),
m_rfTimeoutTimer(1000U, timeout),
m_netTimeoutTimer(1000U, timeout),
m_packetTimer(1000U, 0U, 200U),
m_networkWatchdog(1000U, 0U, 1500U),
m_elapsed(),
m_rfFrames(0U),
m_netFrames(0U),
m_rfFN(0U),
m_rfErrs(0U),
m_rfBits(1U),
m_rfLICH(),
m_netLICH(),
m_rssiMapper(rssiMapper),
m_rssi(0U),
m_maxRSSI(0U),
m_minRSSI(0U),
m_aveRSSI(0U),
m_rssiCount(0U),
m_enabled(true),
m_fp(NULL)
{
	assert(display != NULL);
	assert(rssiMapper != NULL);
}

CM17Control::~CM17Control()
{
}

bool CM17Control::writeModem(unsigned char* data, unsigned int len)
{
	assert(data != NULL);

	if (!m_enabled)
		return false;

	unsigned char type = data[0U];

	if (type == TAG_LOST && m_rfState == RS_RF_AUDIO) {
		std::string source = m_rfLICH.getSource();
		std::string dest = m_rfLICH.getDest();

		if (m_rssi != 0U)
			LogMessage("M17, transmission lost from %s to %s, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", source.c_str(), dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
		else
			LogMessage("M17, transmission lost from %s to %s, %.1f seconds, BER: %.1f%%", source.c_str(), dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits));
		writeEndRF();
		return false;
	}

	if (type == TAG_LOST && m_rfState == RS_RF_DATA) {
		writeEndRF();
		return false;
	}

	if (type == TAG_LOST) {
		m_rfState = RS_RF_LISTENING;
		return false;
	}

	// Have we got RSSI bytes on the end?
	if (len == (M17_FRAME_LENGTH_BYTES + 4U)) {
		uint16_t raw = 0U;
		raw |= (data[50U] << 8) & 0xFF00U;
		raw |= (data[51U] << 0) & 0x00FFU;

		// Convert the raw RSSI to dBm
		int rssi = m_rssiMapper->interpolate(raw);
		if (rssi != 0)
			LogDebug("M17, raw RSSI: %u, reported RSSI: %d dBm", raw, rssi);

		// RSSI is always reported as positive
		m_rssi = (rssi >= 0) ? rssi : -rssi;

		if (m_rssi > m_minRSSI)
			m_minRSSI = m_rssi;
		if (m_rssi < m_maxRSSI)
			m_maxRSSI = m_rssi;

		m_aveRSSI += m_rssi;
		m_rssiCount++;
	}

	unsigned char temp[M17_FRAME_LENGTH_BYTES];
	decorrelator(data + 2U, temp);
	interleaver(temp, data + 2U);

	if (m_rfState == RS_RF_LISTENING) {
		m_rfLICH.reset();

		CM17Convolution conv;
		unsigned char frame[M17_LICH_LENGTH_BYTES];
		conv.decodeLinkSetup(data, frame);

		bool valid = CM17CRC::checkCRC(frame, M17_LICH_LENGTH_BYTES);
		if (valid) {
			m_rfFrames = 0U;
			m_rfErrs = 0U;
			m_rfBits = 1U;
			m_rfTimeoutTimer.start();
			m_minRSSI = m_rssi;
			m_maxRSSI = m_rssi;
			m_aveRSSI = m_rssi;
			m_rssiCount = 1U;
			m_rfFN = 0U;

#if defined(DUMP_M17)
			openFile();
#endif
			m_rfLICH.setLinkSetup(frame);

			std::string source = m_rfLICH.getSource();
			std::string dest   = m_rfLICH.getDest();

			unsigned char dataType = m_rfLICH.getDataType();
			switch (dataType) {
			case 1U:
				LogMessage("M17, received RF data transmission from %s to %s", source.c_str(), dest.c_str());
				m_rfState = RS_RF_DATA;
				break;
			case 2U:
				LogMessage("M17, received RF voice transmission from %s to %s", source.c_str(), dest.c_str());
				m_rfState = RS_RF_AUDIO;
				break;
			case 3U:
				LogMessage("M17, received RF voice + data transmission from %s to %s", source.c_str(), dest.c_str());
				m_rfState = RS_RF_AUDIO;
				break;
			default:
				LogMessage("M17, received RF unknown transmission from %s to %s", source.c_str(), dest.c_str());
				m_rfState = RS_RF_DATA;
				break;
			}

			m_display->writeM17(source.c_str(), dest.c_str(), "R");

#if defined(DUMP_M17)
			writeFile(data + 2U);
#endif
			if (m_duplex) {
				data[0U] = TAG_DATA;
				data[1U] = 0x00U;

				// Generate the sync
				CSync::addM17Sync(data + 2U);

				unsigned char setup[M17_LICH_LENGTH_BYTES];
				m_rfLICH.getLinkSetup(data + 2U);

				// Add the convolution FEC
				CM17Convolution conv;
				conv.encodeLinkSetup(setup, data + 2U + M17_SYNC_LENGTH_BYTES);

				unsigned char temp[M17_FRAME_LENGTH_BYTES];
				interleaver(data + 2U, temp);
				decorrelator(temp, data + 2U);

				writeQueueRF(data);
			}
		} else {
			m_rfState = RS_RF_LATE_ENTRY;
		}
	}

	if (m_rfState == RS_RF_LATE_ENTRY) {
		CM17Convolution conv;
		unsigned char frame[M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES];
		conv.decodeData(data, frame);

		bool valid = CM17CRC::checkCRC(frame, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES);
		if (valid) {
			m_rfFN = (frame[0U] << 8) + (frame[1U] << 0);

			unsigned int frag1, frag2, frag3, frag4;

			// XXX TODO populate frag1-4

			unsigned int lich1 = CGolay24128::decode24128(frag1);
			unsigned int lich2 = CGolay24128::decode24128(frag2);
			unsigned int lich3 = CGolay24128::decode24128(frag3);
			unsigned int lich4 = CGolay24128::decode24128(frag4);

			m_rfLICH.setFragment(data + 2U + M17_SYNC_LENGTH_BYTES, m_rfFN & 0x7FFFU);

			valid = m_rfLICH.isValid();
			if (valid) {
				m_rfFrames = 0U;
				m_rfErrs = 0U;
				m_rfBits = 1U;
				m_rfTimeoutTimer.start();
				m_minRSSI = m_rssi;
				m_maxRSSI = m_rssi;
				m_aveRSSI = m_rssi;
				m_rssiCount = 1U;

#if defined(DUMP_M17)
				openFile();
#endif
				std::string source = m_rfLICH.getSource();
				std::string dest   = m_rfLICH.getDest();

				unsigned char dataType = m_rfLICH.getDataType();
				switch (dataType) {
				case 1U:
					LogMessage("M17, received RF late entry data transmission from %s to %s", source.c_str(), dest.c_str());
					m_rfState = RS_RF_DATA;
					break;
				case 2U:
					LogMessage("M17, received RF late entry voice transmission from %s to %s", source.c_str(), dest.c_str());
					m_rfState = RS_RF_AUDIO;
					break;
				case 3U:
					LogMessage("M17, received RF late entry voice + data transmission from %s to %s", source.c_str(), dest.c_str());
					m_rfState = RS_RF_AUDIO;
					break;
				default:
					LogMessage("M17, received RF late entry unknown transmission from %s to %s", source.c_str(), dest.c_str());
					m_rfState = RS_RF_DATA;
					break;
				}

				m_display->writeM17(source.c_str(), dest.c_str(), "R");

				if (m_duplex) {
					// Create a Link Setup frame
					data[0U] = TAG_DATA;
					data[1U] = 0x00U;

					// Generate the sync
					CSync::addM17Sync(data + 2U);

					unsigned char setup[M17_LICH_LENGTH_BYTES];
					m_rfLICH.getLinkSetup(data + 2U);

					// Add the convolution FEC
					CM17Convolution conv;
					conv.encodeLinkSetup(setup, data + 2U + M17_SYNC_LENGTH_BYTES);

					unsigned char temp[M17_FRAME_LENGTH_BYTES];
					interleaver(data + 2U, temp);
					decorrelator(temp, data + 2U);

					writeQueueRF(data);
				}
			}
		}
	}

	if (m_rfState == RS_RF_AUDIO || m_rfState == RS_RF_DATA) {
#if defined(DUMP_M17)
		writeFile(data + 2U);
#endif
		CM17Convolution conv;
		unsigned char frame[M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES];
		conv.decodeData(data, frame);

		bool valid = CM17CRC::checkCRC(frame, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES);
		if (valid) {
			m_rfFN = (frame[0U] << 8) + (frame[1U] << 0);
		} else {
			// Create a silence frame
			m_rfFN++;

			// The new FN
			frame[0U] = m_rfFN >> 8;
			frame[1U] = m_rfFN >> 0;

			// Add silent audio
			unsigned char dataType = m_rfLICH.getDataType();
			switch (dataType) {
			case 2U:
				::memcpy(frame + M17_FN_LENGTH_BYTES + 0U, M17_3200_SILENCE, 8U);
				::memcpy(frame + M17_FN_LENGTH_BYTES + 8U, M17_3200_SILENCE, 8U);
				break;
			case 3U:
				::memcpy(frame + M17_FN_LENGTH_BYTES + 0U, M17_1600_SILENCE, 8U);
				break;
			default:
				break;
			}

			// Add the CRC
			CM17CRC::encodeCRC(frame, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES);
		}

		unsigned char rfData[2U + M17_FRAME_LENGTH_BYTES];

		rfData[0U] = TAG_DATA;
		rfData[1U] = 0x00U;

		CSync::addM17Sync(rfData + 2U);

		// Re-encode the LICH fragment
		m_rfLICH.getFragment(rfData + 2U + M17_SYNC_LENGTH_BYTES, m_rfFN & 0x7FFFU);

		// XXX TODO Golay on LICH fragment

		// Re-encode the payload
		conv.encodeData(frame, rfData + 2U + M17_SYNC_LENGTH_BYTES + M17_LICH_FRAGMENT_LENGTH_BYTES);

		// Calculate the BER
		if (valid) {
			for (unsigned int i = 2U; i < 50U; i++)
				m_rfErrs += countBits(rfData[i] ^ data[i]);

			m_rfBits += 272U;

			float ber = float(m_rfErrs) / float(m_rfBits);
			m_display->writeM17BER(ber);
		}

		if (m_duplex)
			writeQueueRF(rfData);

		unsigned char netData[M17_LICH_LENGTH_BYTES + M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES];

		m_rfLICH.getNetwork(netData + 0U);

		// Copy the FN and payload from the frame
		::memcpy(netData + M17_LICH_LENGTH_BYTES, frame, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES);

		// The CRC is added in the networking code

		writeNetwork(netData);

		m_rfFrames++;

		// EOT?
		if ((m_rfFN & 0x8000U) == 0x8000U) {
			std::string source = m_rfLICH.getSource();
			std::string dest   = m_rfLICH.getDest();

			if (m_rssi != 0U)
				LogMessage("M17, received RF end of transmission from %s to %s, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", source.c_str(), dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
			else
				LogMessage("M17, received RF end of transmission from %s to %s, %.1f seconds, BER: %.1f%%", source.c_str(), dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits));
			writeEndRF();
		}
	}

	return true;
}

unsigned int CM17Control::readModem(unsigned char* data)
{
	assert(data != NULL);

	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CM17Control::writeEndRF()
{
	m_rfState = RS_RF_LISTENING;

	m_rfTimeoutTimer.stop();

	if (m_netState == RS_NET_IDLE) {
		m_display->clearM17();

		if (m_network != NULL)
			m_network->reset();
	}

#if defined(DUMP_M17)
	closeFile();
#endif
}

void CM17Control::writeEndNet()
{
	m_netState = RS_NET_IDLE;

	m_netTimeoutTimer.stop();
	m_networkWatchdog.stop();
	m_packetTimer.stop();

	m_display->clearM17();

	if (m_network != NULL)
		m_network->reset();
}

void CM17Control::writeNetwork()
{
	unsigned char netData[100U];
	bool exists = m_network->read(netData);
	if (!exists)
		return;

	if (!m_enabled)
		return;

	if (m_rfState != RS_RF_LISTENING && m_netState == RS_NET_IDLE)
		return;

	m_networkWatchdog.start();

	if (m_netState == RS_NET_IDLE) {
		m_netLICH.setNetwork(netData);

		std::string source = m_netLICH.getSource();
		std::string dest   = m_netLICH.getDest();

		unsigned char dataType = m_netLICH.getDataType();
		switch (dataType) {
		case 1U:
			LogMessage("M17, received network data transmission from %s to %s", source.c_str(), dest.c_str());
			m_netState = RS_NET_DATA;
			break;
		case 2U:
			LogMessage("M17, received network voice transmission from %s to %s", source.c_str(), dest.c_str());
			m_netState = RS_NET_AUDIO;
			break;
		case 3U:
			LogMessage("M17, received network voice + data transmission from %s to %s", source.c_str(), dest.c_str());
			m_netState = RS_NET_AUDIO;
			break;
		default:
			LogMessage("M17, received network unknown transmission from %s to %s", source.c_str(), dest.c_str());
			m_netState = RS_NET_DATA;
			break;
		}

		m_display->writeM17(source.c_str(), dest.c_str(), "N");

		m_netTimeoutTimer.start();
		m_packetTimer.start();
		m_elapsed.start();
		m_netFrames = 1U;

		// Create a dummy start message
		unsigned char start[M17_FRAME_LENGTH_BYTES + 2U];

		start[0U] = TAG_DATA;
		start[1U] = 0x00U;

		// Generate the sync
		CSync::addM17Sync(start + 2U);

		unsigned char setup[M17_LICH_LENGTH_BYTES];
		m_netLICH.getLinkSetup(start + 2U);

		// Add the convolution FEC
		CM17Convolution conv;
		conv.encodeLinkSetup(setup, start + 2U + M17_SYNC_LENGTH_BYTES);

		unsigned char temp[M17_FRAME_LENGTH_BYTES];
		interleaver(start + 2U, temp);
		decorrelator(temp, start + 2U);

		writeQueueNet(start);
	}

	unsigned char data[M17_FRAME_LENGTH_BYTES + 2U];

	data[0U] = TAG_DATA;
	data[1U] = 0x00U;

	unsigned char* p = data + 2U;

	// Generate the sync
	CSync::addM17Sync(p);
	p += M17_SYNC_LENGTH_BYTES;

	m_netFrames++;

	// Add the fragment LICH
	uint16_t fn = (netData[38U] << 8) + (netData[39U] << 0);

	unsigned char lich[6U];
	m_netLICH.getFragment(lich, fn & 0x7FFFU);

	unsigned int lich1, lich2, lich3, lich4;

	// XXX TODO

	// Add Golay to the LICH fragment here
	CGolay24128::encode24128(lich1);
	CGolay24128::encode24128(lich2);
	CGolay24128::encode24128(lich3);
	CGolay24128::encode24128(lich4);

	::memcpy(p, &lich1, M17_LICH_FRAGMENT_LENGTH_BYTES / 4U);
	p += M17_LICH_FRAGMENT_LENGTH_BYTES / 4U;

	::memcpy(p, &lich2, M17_LICH_FRAGMENT_LENGTH_BYTES / 4U);
	p += M17_LICH_FRAGMENT_LENGTH_BYTES / 4U;

	::memcpy(p, &lich3, M17_LICH_FRAGMENT_LENGTH_BYTES / 4U);
	p += M17_LICH_FRAGMENT_LENGTH_BYTES / 4U;

	::memcpy(p, &lich4, M17_LICH_FRAGMENT_LENGTH_BYTES / 4U);
	p += M17_LICH_FRAGMENT_LENGTH_BYTES / 4U;

	unsigned char payload[M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES];

	// Add the FN and the data/audio
	::memcpy(payload, netData + 38U, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES);

	// Add the CRC
	CM17CRC::encodeCRC(payload, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES);

	// Add the Convolution FEC
	CM17Convolution conv;
	conv.encodeData(payload, p);

	unsigned char temp[M17_FRAME_LENGTH_BYTES];
	interleaver(data + 2U, temp);
	decorrelator(temp, data + 2U);

	writeQueueNet(data);

	// EOT handling
	if ((fn & 0x8000U) == 0x8000U) {
		std::string source = m_netLICH.getSource();
		std::string dest   = m_netLICH.getDest();
		LogMessage("M17, received network end of transmission from %s to %s, %.1f seconds", source.c_str(), dest.c_str(), float(m_netFrames) / 25.0F);
		writeEndNet();
	}
}

void CM17Control::clock(unsigned int ms)
{
	if (m_network != NULL)
		writeNetwork();

	m_rfTimeoutTimer.clock(ms);
	m_netTimeoutTimer.clock(ms);

	if (m_netState == RS_NET_AUDIO) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			LogMessage("M17, network watchdog has expired, %.1f seconds", float(m_netFrames) / 25.0F);
			writeEndNet();
		}
	}
}

void CM17Control::writeQueueRF(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netState != RS_NET_IDLE)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	unsigned char len = M17_FRAME_LENGTH_BYTES + 2U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("M17, overflow in the M17 RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CM17Control::writeQueueNet(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired())
		return;

	unsigned char len = M17_FRAME_LENGTH_BYTES + 2U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("M17, overflow in the M17 RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CM17Control::writeNetwork(const unsigned char *data)
{
	assert(data != NULL);

	if (m_network == NULL)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	m_network->write(data);
}

void CM17Control::interleaver(const unsigned char* in, unsigned char* out) const
{
	assert(in != NULL);
	assert(out != NULL);

	for (unsigned int i = 0U; i < (M17_FRAME_LENGTH_BITS - M17_SYNC_LENGTH_BITS); i++) {
		unsigned int n1 = i + M17_SYNC_LENGTH_BITS;
		bool b = READ_BIT(in, n1) != 0U;
		unsigned int n2 = INTERLEAVER[i] + M17_SYNC_LENGTH_BITS;
		WRITE_BIT(out, n2, b);
	}
}

void CM17Control::decorrelator(const unsigned char* in, unsigned char* out) const
{
	assert(in != NULL);
	assert(out != NULL);

	for (unsigned int i = M17_SYNC_LENGTH_BYTES; i < M17_FRAME_LENGTH_BYTES; i++) {
		out[i] = in[i] ^ SCRAMBLER[i];
	}
}

bool CM17Control::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "M17_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("M17", 1U, 3U, m_fp);

	return true;
}

bool CM17Control::writeFile(const unsigned char* data)
{
	if (m_fp == NULL)
		return false;

	::fwrite(data, 1U, M17_FRAME_LENGTH_BYTES, m_fp);

	return true;
}

void CM17Control::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}

bool CM17Control::isBusy() const
{
	return m_rfState != RS_RF_LISTENING || m_netState != RS_NET_IDLE;
}

void CM17Control::enable(bool enabled)
{
	if (!enabled && m_enabled) {
		m_queue.clear();

		// Reset the RF section
		m_rfState = RS_RF_LISTENING;

		m_rfTimeoutTimer.stop();

		// Reset the networking section
		m_netState = RS_NET_IDLE;

		m_netTimeoutTimer.stop();
		m_networkWatchdog.stop();
		m_packetTimer.stop();
	}

	m_enabled = enabled;
}

unsigned int CM17Control::countBits(unsigned char byte)
{
	unsigned int count = 0U;

	const unsigned char* p = &byte;

	for (unsigned int i = 0U; i < 8U; i++) {
		if (READ_BIT(p, i) != 0U)
			count++;
	}

	return count;
}
