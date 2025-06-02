/*
 *	Copyright (C) 2020,2021,2023,2025 Jonathan Naylor, G4KLX
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
#include "M17Utils.h"
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

CM17Control::CM17Control(const std::string& callsign, unsigned int can, bool selfOnly, bool allowEncryption, CM17Network* network, CDisplay* display, unsigned int timeout, bool duplex, CRSSIInterpolator* rssiMapper) :
m_callsign(callsign),
m_can(can),
m_selfOnly(selfOnly),
m_allowEncryption(allowEncryption),
m_network(network),
m_display(display),
m_duplex(duplex),
m_queue(5000U, "M17 Control"),
m_source(),
m_dest(),
m_rfState(RPT_RF_STATE::LISTENING),
m_netState(RPT_NET_STATE::IDLE),
m_rfTimeoutTimer(1000U, timeout),
m_netTimeoutTimer(1000U, timeout),
m_networkWatchdog(1000U, 0U, 1500U),
m_elapsed(),
m_rfFrames(0U),
m_netFrames(0U),
m_rfErrs(0U),
m_rfBits(1U),
m_rfLSFCount(0U),
m_rfCurrentRFLSF(),
m_rfCurrentNetLSF(),
m_rfCollectingLSF(),
m_rfCollectedLSF(),
m_rfLSFn(0U),
m_netLSF(),
m_netLSFn(0U),
m_rfTextBits(0x00U),
m_netTextBits(0x00U),
m_rfText(nullptr),
m_netText(nullptr),
m_rssiMapper(rssiMapper),
m_rssi(0U),
m_maxRSSI(0U),
m_minRSSI(0U),
m_aveRSSI(0U),
m_rssiCount(0U),
m_enabled(true),
m_fp(nullptr)
{
	assert(display != nullptr);
	assert(rssiMapper != nullptr);

	m_rfText  = new char[4U * M17_META_LENGTH_BYTES];
	m_netText = new char[4U * M17_META_LENGTH_BYTES];
}

CM17Control::~CM17Control()
{
	delete[] m_netText;
	delete[] m_rfText;
}

bool CM17Control::writeModem(unsigned char* data, unsigned int len)
{
	assert(data != nullptr);

	if (!m_enabled)
		return false;

	unsigned char type = data[0U];

	if (type == TAG_LOST && (m_rfState == RPT_RF_STATE::AUDIO || m_rfState == RPT_RF_STATE::DATA_AUDIO)) {
		if (m_rssi != 0U)
			LogMessage("M17, transmission lost from %s to %s, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", m_source.c_str(), m_dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
		else
			LogMessage("M17, transmission lost from %s to %s, %.1f seconds, BER: %.1f%%", m_source.c_str(), m_dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits));
		writeEndRF();
		return false;
	}

	if ((type == TAG_LOST) && (m_rfState == RPT_RF_STATE::DATA)) {
		writeEndRF();
		return false;
	}

	if ((type == TAG_LOST) && (m_rfState == RPT_RF_STATE::REJECTED)) {
		writeEndRF();
		return false;
	}

	if (type == TAG_LOST) {
		m_rfState = RPT_RF_STATE::LISTENING;
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

	if ((m_rfState == RPT_RF_STATE::LISTENING) && (data[0U] == TAG_HEADER)) {
		m_rfCurrentRFLSF.reset();
		m_rfCurrentNetLSF.reset();

		CM17Convolution conv;
		unsigned char frame[M17_LSF_LENGTH_BYTES];
		unsigned int ber = conv.decodeLinkSetup(data + 2U + M17_SYNC_LENGTH_BYTES, frame);

		bool valid = CM17CRC::checkCRC16(frame, M17_LSF_LENGTH_BYTES);
		if (valid) {
			m_rfCurrentNetLSF.setLinkSetup(frame);

			bool ret = processRFHeader(false);
			if (!ret) {
				m_rfCurrentRFLSF.reset();
				m_rfCurrentNetLSF.reset();
				return false;
			}

			LogDebug("M17, link setup frame: errs: %u/368 (%.1f%%)", ber, float(ber) / 3.68F);

			m_rfFrames   = 0U;
			m_rfErrs     = ber;
			m_rfBits     = 368U;
			m_rfCollectingLSF.reset();
			m_rfCollectedLSF.reset();
			m_rfTimeoutTimer.start();
			m_minRSSI    = m_rssi;
			m_maxRSSI    = m_rssi;
			m_aveRSSI    = m_rssi;
			m_rssiCount  = 1U;
			m_rfLSFn     = 0U;
			m_rfLSFCount = 0U;
			m_rfTextBits = 0x00U;
			::memset(m_rfText, 0x00U, 4U * M17_META_LENGTH_BYTES);
#if defined(DUMP_M17)
			openFile();
#endif
			return true;
		} else {
			m_rfState = RPT_RF_STATE::LATE_ENTRY;
			return false;
		}
	}

	if ((m_rfState == RPT_RF_STATE::LISTENING) && (data[0U] == TAG_DATA)) {
		m_rfState = RPT_RF_STATE::LATE_ENTRY;
		m_rfCurrentRFLSF.reset();
		m_rfCurrentNetLSF.reset();
	}

	if ((m_rfState == RPT_RF_STATE::LATE_ENTRY) && (data[0U] == TAG_DATA)) {
		unsigned int lich1, lich2, lich3, lich4;
		bool valid1 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 0U, lich1);
		bool valid2 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 3U, lich2);
		bool valid3 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 6U, lich3);
		bool valid4 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 9U, lich4);

		if (!valid1 || !valid2 || !valid3 || !valid4)
			return false;

		unsigned char lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
		CM17Utils::combineFragmentLICH(lich1, lich2, lich3, lich4, lich);

		m_rfLSFn = (lich4 >> 5) & 0x07U;
		m_rfCurrentNetLSF.setFragment(lich, m_rfLSFn);

		bool valid = m_rfCurrentNetLSF.isValid();
		if (valid) {
			bool ret = processRFHeader(true);
			if (!ret) {
				m_rfCurrentRFLSF.reset();
				m_rfCurrentNetLSF.reset();
				return false;
			}

			m_rfFrames   = 0U;
			m_rfErrs     = 0U;
			m_rfBits     = 1U;
			m_rfCollectingLSF.reset();
			m_rfCollectedLSF.reset();
			m_rfTimeoutTimer.start();
			m_minRSSI    = m_rssi;
			m_maxRSSI    = m_rssi;
			m_aveRSSI    = m_rssi;
			m_rssiCount  = 1U;
			m_rfLSFCount = 0U;
			m_rfTextBits = 0x00U;
			::memset(m_rfText, 0x00U, 4U * M17_META_LENGTH_BYTES);
#if defined(DUMP_M17)
			openFile();
#endif
			// Fall through
		} else {
			return false;
		}
	}

	if (((m_rfState == RPT_RF_STATE::AUDIO) || (m_rfState == RPT_RF_STATE::DATA_AUDIO)) && (data[0U] == TAG_DATA)) {
#if defined(DUMP_M17)
		writeFile(data + 2U);
#endif
		// Keep looking at the running LSF in case of changed META field data
		unsigned int lich1, lich2, lich3, lich4;
		bool valid1 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 0U, lich1);
		bool valid2 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 3U, lich2);
		bool valid3 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 6U, lich3);
		bool valid4 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 9U, lich4);

		if (valid1 && valid2 && valid3 && valid4) {
			unsigned char lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
			CM17Utils::combineFragmentLICH(lich1, lich2, lich3, lich4, lich);

			unsigned int n = (lich4 >> 5) & 0x07U;
			m_rfCollectingLSF.setFragment(lich, n);

			// If the latest LSF is valid, save it and start collecting the next one
			bool valid = m_rfCollectingLSF.isValid();
			if (valid) {
				m_rfCollectedLSF = m_rfCollectingLSF;
				m_rfCollectingLSF.reset();

				unsigned char encryptionType    = m_rfCollectedLSF.getEncryptionType();
				unsigned char encryptionSubType = m_rfCollectedLSF.getEncryptionSubType();
				if (encryptionType == M17_ENCRYPTION_TYPE_NONE && encryptionSubType == M17_ENCRYPTION_SUB_TYPE_TEXT) {
					unsigned char meta[20U];
					m_rfCollectedLSF.getMeta(meta);
					CUtils::dump(1U, "M17, LSF text data fragment", meta, M17_META_LENGTH_BYTES);

					m_rfTextBits |= meta[0U];

					switch (meta[0U] & 0x0FU) {
						case 0x01U:
							::memcpy(m_rfText + 0U,  meta + 1U, M17_META_LENGTH_BYTES - 1U);
							break;
						case 0x02U:
							::memcpy(m_rfText + 13U, meta + 1U, M17_META_LENGTH_BYTES - 1U);
							break;
						case 0x04U:
							::memcpy(m_rfText + 26U, meta + 1U, M17_META_LENGTH_BYTES - 1U);
							break;
						case 0x08U:
							::memcpy(m_rfText + 39U, meta + 1U, M17_META_LENGTH_BYTES - 1U);
							break;
						default:
							break;
					}

					if (m_rfTextBits == 0x11U || m_rfTextBits == 0x33U || m_rfTextBits == 0x77U || m_rfTextBits == 0xFFU) {
						LogMessage("M17, text Data: \"%s\"", m_rfText);
						m_rfTextBits = 0x00U;
					}
				}
			}
		}

		// Update the currently transmitted LSF when the fragement number is zero
		if (m_rfLSFn == 0U) {
			bool valid = m_rfCollectedLSF.isValid();
			if (valid) {
				m_rfCurrentNetLSF = m_rfCollectedLSF;
				m_rfCollectedLSF.reset();

				m_rfLSFCount++;
				if (m_rfLSFCount > 7U) {
					createRFLSF(true);
					m_rfLSFCount = 0U;
				} else {
					createRFLSF(false);
				}
			}
		}

		CM17Convolution conv;
		unsigned char frame[M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES];
		unsigned int errors = conv.decodeData(data + 2U + M17_SYNC_LENGTH_BYTES + M17_LICH_FRAGMENT_FEC_LENGTH_BYTES, frame);

		uint16_t fn = (frame[0U] << 8) + (frame[1U] << 0);

		LogDebug("M17, audio: FN: %u, errs: %u/272 (%.1f%%)", fn, errors, float(errors) / 2.72F);

		m_rfBits += 272U;
		m_rfErrs += errors;

		float ber = float(m_rfErrs) / float(m_rfBits);
		m_display->writeM17BER(ber);

		if (m_duplex) {
			unsigned char rfData[2U + M17_FRAME_LENGTH_BYTES];

			rfData[0U] = TAG_DATA;
			rfData[1U] = 0x00U;

			// Generate the sync
			CSync::addM17StreamSync(rfData + 2U);

			unsigned char lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
			m_rfCurrentRFLSF.getFragment(lich, m_rfLSFn);

			// Add the fragment number
			lich[5U] = (m_rfLSFn & 0x07U) << 5;

			unsigned int frag1, frag2, frag3, frag4;
			CM17Utils::splitFragmentLICH(lich, frag1, frag2, frag3, frag4);

			// Add Golay to the LICH fragment here
			unsigned int lich1 = CGolay24128::encode24128(frag1);
			unsigned int lich2 = CGolay24128::encode24128(frag2);
			unsigned int lich3 = CGolay24128::encode24128(frag3);
			unsigned int lich4 = CGolay24128::encode24128(frag4);

			CM17Utils::combineFragmentLICHFEC(lich1, lich2, lich3, lich4, rfData + 2U + M17_SYNC_LENGTH_BYTES);

			// Add the Convolution FEC
			conv.encodeData(frame, rfData + 2U + M17_SYNC_LENGTH_BYTES + M17_LICH_FRAGMENT_FEC_LENGTH_BYTES);

			unsigned char temp[M17_FRAME_LENGTH_BYTES];
			interleaver(rfData + 2U, temp);
			decorrelator(temp, rfData + 2U);

			writeQueueRF(rfData);
		}

		if (m_network != nullptr && m_rfTimeoutTimer.isRunning() && !m_rfTimeoutTimer.hasExpired()) {
			unsigned char netData[M17_LSF_LENGTH_BYTES + M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES];

			m_rfCurrentNetLSF.getNetwork(netData + 0U);

			// Copy the FN and payload from the frame
			::memcpy(netData + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES, frame, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES);
			// Remove any erronous EOF from the FN
			netData[M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + 0U] &= 0x7FU;

			// The CRC is added in the networking code

			m_network->write(netData);
		}

		m_rfFrames++;

		m_rfLSFn++;
		if (m_rfLSFn >= 6U)
			m_rfLSFn = 0U;

		return true;
	}

	if (((m_rfState == RPT_RF_STATE::AUDIO) || (m_rfState == RPT_RF_STATE::DATA_AUDIO)) && (data[0U] == TAG_EOT)) {
#if defined(DUMP_M17)
		writeFile(data + 2U);
#endif
		if (m_duplex) {
			unsigned char rfData[M17_FRAME_LENGTH_BYTES + 2U];

			rfData[0U] = TAG_EOT;
			rfData[1U] = 0x00U;

			// Generate the sync
			for (unsigned int i = 0U; i < M17_FRAME_LENGTH_BYTES; i += M17_SYNC_LENGTH_BYTES)
				CSync::addM17EOTSync(rfData + 2U + i);

			writeQueueRF(rfData);
		}

		if (m_network != nullptr && m_rfTimeoutTimer.isRunning() && !m_rfTimeoutTimer.hasExpired()) {
			unsigned char netData[M17_LSF_LENGTH_BYTES + M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES];

			m_rfCurrentNetLSF.getNetwork(netData + 0U);

			// Add a EOF FN and silence for the EOF frame
			netData[M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + 0U] = 0x80U;
			netData[M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + 1U] = 0x00U;

			if (m_rfState == RPT_RF_STATE::AUDIO) {
				::memcpy(netData + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + M17_FN_LENGTH_BYTES + 0U, M17_3200_SILENCE, 8U);
				::memcpy(netData + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + M17_FN_LENGTH_BYTES + 8U, M17_3200_SILENCE, 8U);
			} else if (m_rfState == RPT_RF_STATE::DATA_AUDIO) {
				::memcpy(netData + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + M17_FN_LENGTH_BYTES + 0U, M17_1600_SILENCE, 8U);
				::memset(netData + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + M17_FN_LENGTH_BYTES + 8U, 0x00U, 8U);
			} else {
				::memset(netData + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + M17_FN_LENGTH_BYTES + 0U, 0x00U, 8U);
				::memset(netData + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + M17_FN_LENGTH_BYTES + 8U, 0x00U, 8U);
			}

			// The CRC is added in the networking code

			m_network->write(netData);
		}

		if (m_rssi != 0U)
			LogMessage("M17, received RF end of transmission from %s to %s, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", m_source.c_str(), m_dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
		else
			LogMessage("M17, received RF end of transmission from %s to %s, %.1f seconds, BER: %.1f%%", m_source.c_str(), m_dest.c_str(), float(m_rfFrames) / 25.0F, float(m_rfErrs * 100U) / float(m_rfBits));
		writeEndRF();

		return true;
	}

	return false;
}

unsigned int CM17Control::readModem(unsigned char* data)
{
	assert(data != nullptr);

	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CM17Control::writeEndRF()
{
	m_rfState = RPT_RF_STATE::LISTENING;

	m_rfTimeoutTimer.stop();

	m_source.clear();
	m_dest.clear();

	m_rfCurrentRFLSF.reset();
	m_rfCurrentNetLSF.reset();
	m_rfCollectingLSF.reset();
	m_rfCollectedLSF.reset();

	if (m_netState == RPT_NET_STATE::IDLE) {
		m_display->clearM17();

		if (m_network != nullptr)
			m_network->reset();
	}

#if defined(DUMP_M17)
	closeFile();
#endif
}

void CM17Control::writeEndNet()
{
	m_netState = RPT_NET_STATE::IDLE;

	m_netTimeoutTimer.stop();
	m_networkWatchdog.stop();

	m_source.clear();
	m_dest.clear();

	m_netLSF.reset();

	m_display->clearM17();

	if (m_network != nullptr)
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

	if ((m_rfState != RPT_RF_STATE::LISTENING) && (m_rfState != RPT_RF_STATE::LATE_ENTRY) && (m_netState == RPT_NET_STATE::IDLE)) {
		m_network->reset();
		return;
	}

	m_networkWatchdog.start();

	if (!m_allowEncryption) {
		CM17LSF lsf;
		lsf.setNetwork(netData);

		unsigned char type = lsf.getEncryptionType();
		if (type != M17_ENCRYPTION_TYPE_NONE) {
			m_network->reset();
			return;
		}
	}

	if (m_netState == RPT_NET_STATE::IDLE) {
		m_netLSF.setNetwork(netData);

		m_source = m_netLSF.getSource();
		m_dest   = m_netLSF.getDest();

		m_netLSF.setSource(m_callsign);
		m_netLSF.setCAN(m_can);

		unsigned char dataType = m_netLSF.getDataType();
		switch (dataType) {
		case M17_DATA_TYPE_DATA:
			LogMessage("M17, received network data transmission from %s to %s", m_source.c_str(), m_dest.c_str());
			m_netState = RPT_NET_STATE::DATA;
			break;
		case M17_DATA_TYPE_VOICE:
			LogMessage("M17, received network voice transmission from %s to %s", m_source.c_str(), m_dest.c_str());
			m_netState = RPT_NET_STATE::AUDIO;
			break;
		case M17_DATA_TYPE_VOICE_DATA:
			LogMessage("M17, received network voice + data transmission from %s to %s", m_source.c_str(), m_dest.c_str());
			m_netState = RPT_NET_STATE::DATA_AUDIO;
			break;
		default:
			LogMessage("M17, received network unknown transmission from %s to %s", m_source.c_str(), m_dest.c_str());
			m_network->reset();
			return;
		}

		m_display->writeM17(m_source.c_str(), m_dest.c_str(), "N");

		m_netTimeoutTimer.start();
		m_elapsed.start();
		m_netFrames   = 0U;
		m_netLSFn     = 0U;
		m_netTextBits = 0x00U;
		::memset(m_netText, 0x00U, 4U * M17_META_LENGTH_BYTES);

		// Create a dummy start message
		unsigned char start[M17_FRAME_LENGTH_BYTES + 2U];

		start[0U] = TAG_HEADER;
		start[1U] = 0x00U;

		// Generate the sync
		CSync::addM17LinkSetupSync(start + 2U);

		unsigned char setup[M17_LSF_LENGTH_BYTES];
		m_netLSF.getLinkSetup(setup);

		// Add the convolution FEC
		CM17Convolution conv;
		conv.encodeLinkSetup(setup, start + 2U + M17_SYNC_LENGTH_BYTES);

		unsigned char temp[M17_FRAME_LENGTH_BYTES];
		interleaver(start + 2U, temp);
		decorrelator(temp, start + 2U);

		writeQueueNet(start);
	}

	if ((m_netState == RPT_NET_STATE::AUDIO) || (m_netState == RPT_NET_STATE::DATA_AUDIO)) {
		// Refresh the LSF every six frames in case the META field changes
		if (m_netLSFn == 0U) {
			m_netLSF.setNetwork(netData);

			m_netLSF.setSource(m_callsign);
			m_netLSF.setCAN(m_can);

			unsigned char encryptionType    = m_netLSF.getEncryptionType();
			unsigned char encryptionSubType = m_netLSF.getEncryptionSubType();
			if (encryptionType == M17_ENCRYPTION_TYPE_NONE && encryptionSubType == M17_ENCRYPTION_SUB_TYPE_TEXT) {
				unsigned char meta[20U];
				m_netLSF.getMeta(meta);
				CUtils::dump(1U, "M17, LSF text data fragment", meta, M17_META_LENGTH_BYTES);

				m_netTextBits |= meta[0U];

				switch (meta[0U] & 0x0FU) {
					case 0x01U:
						::memcpy(m_netText + 0U,  meta + 1U, M17_META_LENGTH_BYTES - 1U);
						break;
					case 0x02U:
						::memcpy(m_netText + 13U, meta + 1U, M17_META_LENGTH_BYTES - 1U);
						break;
					case 0x04U:
						::memcpy(m_netText + 26U, meta + 1U, M17_META_LENGTH_BYTES - 1U);
						break;
					case 0x08U:
						::memcpy(m_netText + 39U, meta + 1U, M17_META_LENGTH_BYTES - 1U);
						break;
					default:
						break;
				}

				if (m_netTextBits == 0x11U || m_netTextBits == 0x33U || m_netTextBits == 0x77U || m_netTextBits == 0xFFU) {
					LogMessage("M17, text Data: \"%s\"", m_netText);
					m_netTextBits = 0x00U;
				}
			}
		}

		unsigned char data[M17_FRAME_LENGTH_BYTES + 2U];

		data[0U] = TAG_DATA;
		data[1U] = 0x00U;

		// Generate the sync
		CSync::addM17StreamSync(data + 2U);

		m_netFrames++;

		// Add the fragment LICH
		unsigned char lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
		m_netLSF.getFragment(lich, m_netLSFn);

		// Add the fragment number
		lich[5U] = (m_netLSFn & 0x07U) << 5;

		unsigned int frag1, frag2, frag3, frag4;
		CM17Utils::splitFragmentLICH(lich, frag1, frag2, frag3, frag4);

		// Add Golay to the LICH fragment here
		unsigned int lich1 = CGolay24128::encode24128(frag1);
		unsigned int lich2 = CGolay24128::encode24128(frag2);
		unsigned int lich3 = CGolay24128::encode24128(frag3);
		unsigned int lich4 = CGolay24128::encode24128(frag4);

		CM17Utils::combineFragmentLICHFEC(lich1, lich2, lich3, lich4, data + 2U + M17_SYNC_LENGTH_BYTES);

		// Add the FN and the data/audio
		unsigned char payload[M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES];

		// Copy the FN minus the EOF marker
		payload[0U] = netData[28U] & 0x7FU;
		payload[1U] = netData[29U];

		::memcpy(payload + 2U, netData + 30U, M17_PAYLOAD_LENGTH_BYTES);

		// Add the Convolution FEC
		CM17Convolution conv;
		conv.encodeData(payload, data + 2U + M17_SYNC_LENGTH_BYTES + M17_LICH_FRAGMENT_FEC_LENGTH_BYTES);

		unsigned char temp[M17_FRAME_LENGTH_BYTES];
		interleaver(data + 2U, temp);
		decorrelator(temp, data + 2U);

		writeQueueNet(data);

		m_netLSFn++;
		if (m_netLSFn >= 6U)
			m_netLSFn = 0U;

		// EOT handling
		uint16_t fn = (netData[28U] << 8) + (netData[29U] << 0);
		if ((fn & 0x8000U) == 0x8000U) {
			LogMessage("M17, received network end of transmission from %s to %s, %.1f seconds", m_source.c_str(), m_dest.c_str(), float(m_netFrames) / 25.0F);

			unsigned char data[M17_FRAME_LENGTH_BYTES + 2U];

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;

			// Generate the sync
			for (unsigned int i = 0U; i < M17_FRAME_LENGTH_BYTES; i += M17_SYNC_LENGTH_BYTES)
				CSync::addM17EOTSync(data + 2U + i);

			writeQueueNet(data);

			writeEndNet();
		}
	}
}

bool CM17Control::processRFHeader(bool lateEntry)
{
	unsigned char packetStream = m_rfCurrentNetLSF.getPacketStream();
	if (packetStream == M17_PACKET_TYPE)
		return false;

	unsigned char can = m_rfCurrentNetLSF.getCAN();
	if (can != m_can)
		return false;

	m_source = m_rfCurrentNetLSF.getSource();
	m_dest   = m_rfCurrentNetLSF.getDest();

	if (!m_allowEncryption) {
		unsigned char type = m_rfCurrentNetLSF.getEncryptionType();
		if (type != M17_ENCRYPTION_TYPE_NONE) {
			LogMessage("M17, access attempt with encryption from %s to %s", m_source.c_str(), m_dest.c_str());
			m_rfState = RPT_RF_STATE::REJECTED;
			return true;
		}
	}

	if (m_selfOnly) {
		bool ret = checkCallsign(m_source);
		if (!ret) {
			LogMessage("M17, invalid access attempt from %s to %s", m_source.c_str(), m_dest.c_str());
			m_rfState = RPT_RF_STATE::REJECTED;
			return true;
		}
	}

	unsigned char dataType = m_rfCurrentNetLSF.getDataType();
	switch (dataType) {
	case M17_DATA_TYPE_DATA:
		LogMessage("M17, received RF%sdata transmission from %s to %s", lateEntry ? " late entry " : " ", m_source.c_str(), m_dest.c_str());
		m_rfState = RPT_RF_STATE::DATA;
		break;
	case M17_DATA_TYPE_VOICE:
		LogMessage("M17, received RF%svoice transmission from %s to %s", lateEntry ? " late entry " : " ", m_source.c_str(), m_dest.c_str());
		m_rfState = RPT_RF_STATE::AUDIO;
		break;
	case M17_DATA_TYPE_VOICE_DATA:
		LogMessage("M17, received RF%svoice + data transmission from %s to %s", lateEntry ? " late entry " : " ", m_source.c_str(), m_dest.c_str());
		m_rfState = RPT_RF_STATE::DATA_AUDIO;
		break;
	default:
		return false;
	}

	m_display->writeM17(m_source.c_str(), m_dest.c_str(), "R");

	createRFLSF(true);

	if (m_duplex) {
		unsigned char data[M17_FRAME_LENGTH_BYTES + 2U];

		// Create a Link Setup frame
		data[0U] = TAG_HEADER;
		data[1U] = 0x00U;

		// Generate the sync
		CSync::addM17LinkSetupSync(data + 2U);

		unsigned char setup[M17_LSF_LENGTH_BYTES];
		m_rfCurrentRFLSF.getLinkSetup(setup);

		// Add the convolution FEC
		CM17Convolution conv;
		conv.encodeLinkSetup(setup, data + 2U + M17_SYNC_LENGTH_BYTES);

		unsigned char temp[M17_FRAME_LENGTH_BYTES];
		interleaver(data + 2U, temp);
		decorrelator(temp, data + 2U);

		writeQueueRF(data);
	}

	return true;
}

void CM17Control::createRFLSF(bool addCallsign)
{
	m_rfCurrentRFLSF = m_rfCurrentNetLSF;

	m_rfCurrentRFLSF.setSource(m_callsign);

	if (addCallsign) {
		m_rfCurrentRFLSF.setEncryptionType(M17_ENCRYPTION_TYPE_NONE);
		m_rfCurrentRFLSF.setEncryptionSubType(M17_ENCRYPTION_SUB_TYPE_CALLSIGNS);

		// Copy the encoded source into the META field
		unsigned char meta[M17_META_LENGTH_BYTES];
		::memset(meta, 0x00U, M17_META_LENGTH_BYTES);
		CM17Utils::encodeCallsign(m_source, meta + 0U);

		m_rfCurrentRFLSF.setMeta(meta);
	}
}

void CM17Control::clock(unsigned int ms)
{
	if (m_network != nullptr)
		writeNetwork();

	if (!m_enabled)
	  return;

	m_rfTimeoutTimer.clock(ms);
	m_netTimeoutTimer.clock(ms);

	if ((m_netState == RPT_NET_STATE::AUDIO) || (m_netState == RPT_NET_STATE::DATA_AUDIO)) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			LogMessage("M17, network watchdog has expired, %.1f seconds", float(m_netFrames) / 25.0F);
			writeEndNet();
		}
	}
}

void CM17Control::writeQueueRF(const unsigned char *data)
{
	assert(data != nullptr);

	if (m_netState != RPT_NET_STATE::IDLE)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	const unsigned char len = M17_FRAME_LENGTH_BYTES + 2U;

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
	assert(data != nullptr);

	if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired())
		return;

	const unsigned char len = M17_FRAME_LENGTH_BYTES + 2U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("M17, overflow in the M17 RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CM17Control::interleaver(const unsigned char* in, unsigned char* out) const
{
	assert(in != nullptr);
	assert(out != nullptr);

	for (unsigned int i = 0U; i < (M17_FRAME_LENGTH_BITS - M17_SYNC_LENGTH_BITS); i++) {
		unsigned int n1 = i + M17_SYNC_LENGTH_BITS;
		bool b = READ_BIT(in, n1) != 0U;
		unsigned int n2 = INTERLEAVER[i] + M17_SYNC_LENGTH_BITS;
		WRITE_BIT(out, n2, b);
	}
}

void CM17Control::decorrelator(const unsigned char* in, unsigned char* out) const
{
	assert(in != nullptr);
	assert(out != nullptr);

	for (unsigned int i = M17_SYNC_LENGTH_BYTES; i < M17_FRAME_LENGTH_BYTES; i++) {
		out[i] = in[i] ^ SCRAMBLER[i];
	}
}

bool CM17Control::checkCallsign(const std::string& callsign) const
{
	size_t len = m_callsign.size();

	return m_callsign.compare(0U, len, callsign, 0U, len) == 0;
}

bool CM17Control::openFile()
{
	if (m_fp != nullptr)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "M17_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == nullptr)
		return false;

	::fwrite("M17", 1U, 3U, m_fp);

	return true;
}

bool CM17Control::writeFile(const unsigned char* data)
{
	if (m_fp == nullptr)
		return false;

	::fwrite(data, 1U, M17_FRAME_LENGTH_BYTES, m_fp);

	return true;
}

void CM17Control::closeFile()
{
	if (m_fp != nullptr) {
		::fclose(m_fp);
		m_fp = nullptr;
	}
}

bool CM17Control::isBusy() const
{
	return (m_rfState != RPT_RF_STATE::LISTENING) || (m_netState != RPT_NET_STATE::IDLE);
}

void CM17Control::enable(bool enabled)
{
	if (!enabled && m_enabled) {
		m_queue.clear();

		// Reset the RF section
		switch (m_rfState) {
		case RPT_RF_STATE::LISTENING:
		case RPT_RF_STATE::REJECTED:
		case RPT_RF_STATE::INVALID:
			break;

		default:
			if (m_rfTimeoutTimer.isRunning()) {
				LogMessage("M17, RF user has timed out");
			}
			break;
		}
		m_rfState = RPT_RF_STATE::LISTENING;

		m_rfTimeoutTimer.stop();

		// Reset the networking section
		switch(m_netState) {
		case RPT_NET_STATE::IDLE:
			break;

		default:
			if (m_netTimeoutTimer.isRunning()) {
				LogMessage("M17, network user has timed out");
			}
			break;
		}
		m_netState = RPT_NET_STATE::IDLE;

		m_netTimeoutTimer.stop();
		m_networkWatchdog.stop();
	}

	m_enabled = enabled;
}
