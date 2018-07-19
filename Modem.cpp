/*
 *   Copyright (C) 2011-2018 by Jonathan Naylor G4KLX
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

#include "DStarDefines.h"
#include "DMRDefines.h"
#include "YSFDefines.h"
#include "P25Defines.h"
#include "NXDNDefines.h"
#include "POCSAGDefines.h"
#include "Thread.h"
#include "Modem.h"
#include "Utils.h"
#include "Log.h"

#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <ctime>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <unistd.h>
#endif

const unsigned char MMDVM_FRAME_START = 0xE0U;

const unsigned char MMDVM_GET_VERSION = 0x00U;
const unsigned char MMDVM_GET_STATUS  = 0x01U;
const unsigned char MMDVM_SET_CONFIG  = 0x02U;
const unsigned char MMDVM_SET_MODE    = 0x03U;
const unsigned char MMDVM_SET_FREQ    = 0x04U;

const unsigned char MMDVM_SEND_CWID   = 0x0AU;

const unsigned char MMDVM_DSTAR_HEADER = 0x10U;
const unsigned char MMDVM_DSTAR_DATA   = 0x11U;
const unsigned char MMDVM_DSTAR_LOST   = 0x12U;
const unsigned char MMDVM_DSTAR_EOT    = 0x13U;

const unsigned char MMDVM_DMR_DATA1   = 0x18U;
const unsigned char MMDVM_DMR_LOST1   = 0x19U;
const unsigned char MMDVM_DMR_DATA2   = 0x1AU;
const unsigned char MMDVM_DMR_LOST2   = 0x1BU;
const unsigned char MMDVM_DMR_SHORTLC = 0x1CU;
const unsigned char MMDVM_DMR_START   = 0x1DU;
const unsigned char MMDVM_DMR_ABORT   = 0x1EU;

const unsigned char MMDVM_YSF_DATA    = 0x20U;
const unsigned char MMDVM_YSF_LOST    = 0x21U;

const unsigned char MMDVM_P25_HDR     = 0x30U;
const unsigned char MMDVM_P25_LDU     = 0x31U;
const unsigned char MMDVM_P25_LOST    = 0x32U;

const unsigned char MMDVM_NXDN_DATA   = 0x40U;
const unsigned char MMDVM_NXDN_LOST   = 0x41U;

const unsigned char MMDVM_POCSAG_DATA = 0x50U;

const unsigned char MMDVM_ACK         = 0x70U;
const unsigned char MMDVM_NAK         = 0x7FU;

const unsigned char MMDVM_SERIAL      = 0x80U;

const unsigned char MMDVM_TRANSPARENT = 0x90U;

const unsigned char MMDVM_DEBUG1      = 0xF1U;
const unsigned char MMDVM_DEBUG2      = 0xF2U;
const unsigned char MMDVM_DEBUG3      = 0xF3U;
const unsigned char MMDVM_DEBUG4      = 0xF4U;
const unsigned char MMDVM_DEBUG5      = 0xF5U;

const unsigned int MAX_RESPONSES = 30U;

const unsigned int BUFFER_LENGTH = 2000U;


CModem::CModem(const std::string& port, const std::string& protocol, unsigned int address, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, bool trace, bool debug) :
m_port(port),
m_dmrColorCode(0U),
m_ysfLoDev(false),
m_ysfTXHang(4U),
m_duplex(duplex),
m_rxInvert(rxInvert),
m_txInvert(txInvert),
m_pttInvert(pttInvert),
m_txDelay(txDelay),
m_dmrDelay(dmrDelay),
m_rxLevel(0U),
m_cwIdTXLevel(0U),
m_dstarTXLevel(0U),
m_dmrTXLevel(0U),
m_ysfTXLevel(0U),
m_p25TXLevel(0U),
m_nxdnTXLevel(0U),
m_pocsagTXLevel(0U),
m_trace(trace),
m_debug(debug),
m_rxFrequency(0U),
m_txFrequency(0U),
m_pocsagFrequency(0U),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_ysfEnabled(false),
m_p25Enabled(false),
m_nxdnEnabled(false),
m_pocsagEnabled(false),
m_rxDCOffset(0),
m_txDCOffset(0),
m_serial(port, SERIAL_115200, protocol, address, true),
m_buffer(NULL),
m_length(0U),
m_offset(0U),
m_rxDStarData(1000U, "Modem RX D-Star"),
m_txDStarData(1000U, "Modem TX D-Star"),
m_rxDMRData1(1000U, "Modem RX DMR1"),
m_rxDMRData2(1000U, "Modem RX DMR2"),
m_txDMRData1(1000U, "Modem TX DMR1"),
m_txDMRData2(1000U, "Modem TX DMR2"),
m_rxYSFData(1000U, "Modem RX YSF"),
m_txYSFData(1000U, "Modem TX YSF"),
m_rxP25Data(1000U, "Modem RX P25"),
m_txP25Data(1000U, "Modem TX P25"),
m_rxNXDNData(1000U, "Modem RX NXDN"),
m_txNXDNData(1000U, "Modem TX NXDN"),
m_txPOCSAGData(1000U, "Modem TX POCSAG"),
m_rxTransparentData(1000U, "Modem RX Transparent"),
m_txTransparentData(1000U, "Modem TX Transparent"),
m_statusTimer(1000U, 0U, 250U),
m_inactivityTimer(1000U, 2U),
m_playoutTimer(1000U, 0U, 10U),
m_dstarSpace(0U),
m_dmrSpace1(0U),
m_dmrSpace2(0U),
m_ysfSpace(0U),
m_p25Space(0U),
m_nxdnSpace(0U),
m_pocsagSpace(0U),
m_tx(false),
m_cd(false),
m_lockout(false),
m_error(false),
m_hwType(HWT_UNKNOWN)
{
	assert(!port.empty());

	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CModem::~CModem()
{
	delete[] m_buffer;
}

void CModem::setRFParams(unsigned int rxFrequency, int rxOffset, unsigned int txFrequency, int txOffset, int txDCOffset, int rxDCOffset, float rfLevel, unsigned int pocsagFrequency)
{
	m_rxFrequency     = rxFrequency + rxOffset;
	m_txFrequency     = txFrequency + txOffset;
	m_txDCOffset      = txDCOffset;
	m_rxDCOffset      = rxDCOffset;
	m_rfLevel         = rfLevel;
	m_pocsagFrequency = pocsagFrequency + txOffset;
}

void CModem::setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled, bool nxdnEnabled, bool pocsagEnabled)
{
	m_dstarEnabled  = dstarEnabled;
	m_dmrEnabled    = dmrEnabled;
	m_ysfEnabled    = ysfEnabled;
	m_p25Enabled    = p25Enabled;
	m_nxdnEnabled   = nxdnEnabled;
	m_pocsagEnabled = pocsagEnabled;
}

void CModem::setLevels(float rxLevel, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float pocsagTXLevel)
{
	m_rxLevel       = rxLevel;
	m_cwIdTXLevel   = cwIdTXLevel;
	m_dstarTXLevel  = dstarTXLevel;
	m_dmrTXLevel    = dmrTXLevel;
	m_ysfTXLevel    = ysfTXLevel;
	m_p25TXLevel    = p25TXLevel;
	m_nxdnTXLevel   = nxdnTXLevel;
	m_pocsagTXLevel = pocsagTXLevel;
}

void CModem::setDMRParams(unsigned int colorCode)
{
	assert(colorCode < 16U);

	m_dmrColorCode = colorCode;
}

void CModem::setYSFParams(bool loDev, unsigned int txHang)
{
	m_ysfLoDev  = loDev;
	m_ysfTXHang = txHang;
}

bool CModem::open()
{
	::LogMessage("Opening the MMDVM");

	bool ret = m_serial.open();
	if (!ret)
		return false;

	ret = readVersion();
	if (!ret) {
		m_serial.close();
		return false;
	} else {
		/* Stopping the inactivity timer here when a firmware version has been
		   successfuly read prevents the death spiral of "no reply from modem..." */
		m_inactivityTimer.stop();
	}

	ret = setFrequency();
	if (!ret) {
		m_serial.close();
		return false;
	}

	ret = setConfig();
	if (!ret) {
		m_serial.close();
		return false;
	}

	m_statusTimer.start();

	m_error  = false;
	m_offset = 0U;

	return true;
}

void CModem::clock(unsigned int ms)
{
	// Poll the modem status every 250ms
	m_statusTimer.clock(ms);
	if (m_statusTimer.hasExpired()) {
		readStatus();
		m_statusTimer.start();
	}

	m_inactivityTimer.clock(ms);
	if (m_inactivityTimer.hasExpired()) {
		LogError("No reply from the modem for some time, resetting it");
		m_error = true;
		close();

		CThread::sleep(2000U);		// 2s
		while (!open())
			CThread::sleep(5000U);	// 5s
	}

	RESP_TYPE_MMDVM type = getResponse();

	if (type == RTM_TIMEOUT) {
		// Nothing to do
	} else if (type == RTM_ERROR) {
		// Nothing to do
	} else {
		// type == RTM_OK
		switch (m_buffer[2U]) {
			case MMDVM_DSTAR_HEADER: {
					if (m_trace)
						CUtils::dump(1U, "RX D-Star Header", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_HEADER;
					m_rxDStarData.addData(&data, 1U);

					m_rxDStarData.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DSTAR_DATA: {
					if (m_trace)
						CUtils::dump(1U, "RX D-Star Data", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_DATA;
					m_rxDStarData.addData(&data, 1U);

					m_rxDStarData.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DSTAR_LOST: {
					if (m_trace)
						CUtils::dump(1U, "RX D-Star Lost", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDStarData.addData(&data, 1U);
				}
				break;

			case MMDVM_DSTAR_EOT: {
					if (m_trace)
						CUtils::dump(1U, "RX D-Star EOT", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_EOT;
					m_rxDStarData.addData(&data, 1U);
				}
				break;

			case MMDVM_DMR_DATA1: {
					if (m_trace)
						CUtils::dump(1U, "RX DMR Data 1", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDMRData1.addData(&data, 1U);

					if (m_buffer[3U] == (DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC))
						data = TAG_EOT;
					else
						data = TAG_DATA;
					m_rxDMRData1.addData(&data, 1U);

					m_rxDMRData1.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DMR_DATA2: {
					if (m_trace)
						CUtils::dump(1U, "RX DMR Data 2", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDMRData2.addData(&data, 1U);

					if (m_buffer[3U] == (DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC))
						data = TAG_EOT;
					else
						data = TAG_DATA;
					m_rxDMRData2.addData(&data, 1U);

					m_rxDMRData2.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DMR_LOST1: {
					if (m_trace)
						CUtils::dump(1U, "RX DMR Lost 1", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDMRData1.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDMRData1.addData(&data, 1U);
				}
				break;

			case MMDVM_DMR_LOST2: {
					if (m_trace)
						CUtils::dump(1U, "RX DMR Lost 2", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDMRData2.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDMRData2.addData(&data, 1U);
				}
				break;

			case MMDVM_YSF_DATA: {
					if (m_trace)
						CUtils::dump(1U, "RX YSF Data", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxYSFData.addData(&data, 1U);

					data = TAG_DATA;
					m_rxYSFData.addData(&data, 1U);

					m_rxYSFData.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_YSF_LOST: {
					if (m_trace)
						CUtils::dump(1U, "RX YSF Lost", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxYSFData.addData(&data, 1U);

					data = TAG_LOST;
					m_rxYSFData.addData(&data, 1U);
				}
				break;

			case MMDVM_P25_HDR: {
				if (m_trace)
					CUtils::dump(1U, "RX P25 Header", m_buffer, m_length);

				unsigned char data = m_length - 2U;
				m_rxP25Data.addData(&data, 1U);

				data = TAG_HEADER;
				m_rxP25Data.addData(&data, 1U);

				m_rxP25Data.addData(m_buffer + 3U, m_length - 3U);
			}
			break;

			case MMDVM_P25_LDU: {
				if (m_trace)
					CUtils::dump(1U, "RX P25 LDU", m_buffer, m_length);

				unsigned char data = m_length - 2U;
				m_rxP25Data.addData(&data, 1U);

				data = TAG_DATA;
				m_rxP25Data.addData(&data, 1U);

				m_rxP25Data.addData(m_buffer + 3U, m_length - 3U);
			}
			break;

			case MMDVM_P25_LOST: {
				if (m_trace)
					CUtils::dump(1U, "RX P25 Lost", m_buffer, m_length);

				unsigned char data = 1U;
				m_rxP25Data.addData(&data, 1U);

				data = TAG_LOST;
				m_rxP25Data.addData(&data, 1U);
			}
			break;

			case MMDVM_NXDN_DATA: {
				if (m_trace)
					CUtils::dump(1U, "RX NXDN Data", m_buffer, m_length);

				unsigned char data = m_length - 2U;
				m_rxNXDNData.addData(&data, 1U);

				data = TAG_DATA;
				m_rxNXDNData.addData(&data, 1U);

				m_rxNXDNData.addData(m_buffer + 3U, m_length - 3U);
			}
			break;

			case MMDVM_NXDN_LOST: {
				if (m_trace)
					CUtils::dump(1U, "RX NXDN Lost", m_buffer, m_length);

				unsigned char data = 1U;
				m_rxNXDNData.addData(&data, 1U);

				data = TAG_LOST;
				m_rxNXDNData.addData(&data, 1U);
			}
			break;

			case MMDVM_GET_STATUS: {
					// if (m_trace)
					//	CUtils::dump(1U, "GET_STATUS", m_buffer, m_length);

					m_p25Space    = 0U;
					m_nxdnSpace   = 0U;
					m_pocsagSpace = 0U;

					m_tx = (m_buffer[5U] & 0x01U) == 0x01U;

					bool adcOverflow = (m_buffer[5U] & 0x02U) == 0x02U;
					if (adcOverflow)
						LogError("MMDVM ADC levels have overflowed");

					bool rxOverflow = (m_buffer[5U] & 0x04U) == 0x04U;
					if (rxOverflow)
						LogError("MMDVM RX buffer has overflowed");

					bool txOverflow = (m_buffer[5U] & 0x08U) == 0x08U;
					if (txOverflow)
						LogError("MMDVM TX buffer has overflowed");

					m_lockout = (m_buffer[5U] & 0x10U) == 0x10U;

					bool dacOverflow = (m_buffer[5U] & 0x20U) == 0x20U;
					if (dacOverflow)
						LogError("MMDVM DAC levels have overflowed");

					m_cd = (m_buffer[5U] & 0x40U) == 0x40U;

					m_dstarSpace  = m_buffer[6U];
					m_dmrSpace1   = m_buffer[7U];
					m_dmrSpace2   = m_buffer[8U];
					m_ysfSpace    = m_buffer[9U];

					if (m_length > 10U)
						m_p25Space    = m_buffer[10U];
					if (m_length > 11U)
						m_nxdnSpace   = m_buffer[11U];
					if (m_length > 12U)
						m_pocsagSpace = m_buffer[12U];

					m_inactivityTimer.start();
					// LogMessage("status=%02X, tx=%d, space=%u,%u,%u,%u,%u,%u,%u lockout=%d, cd=%d", m_buffer[5U], int(m_tx), m_dstarSpace, m_dmrSpace1, m_dmrSpace2, m_ysfSpace, m_p25Space, m_nxdnSpace, m_pocsagSpace, int(m_lockout), int(m_cd));
				}
				break;

			case MMDVM_TRANSPARENT: {
					if (m_trace)
						CUtils::dump(1U, "RX Transparent Data", m_buffer, m_length);

					unsigned char data = m_length - 3U;
					m_rxTransparentData.addData(&data, 1U);

					m_rxTransparentData.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			// These should not be received, but don't complain if we do
			case MMDVM_GET_VERSION:
			case MMDVM_ACK:
				break;

			case MMDVM_NAK:
				LogWarning("Received a NAK from the MMDVM, command = 0x%02X, reason = %u", m_buffer[3U], m_buffer[4U]);
				break;

			case MMDVM_DEBUG1:
			case MMDVM_DEBUG2:
			case MMDVM_DEBUG3:
			case MMDVM_DEBUG4:
			case MMDVM_DEBUG5:
				printDebug();
				break;

			default:
				LogMessage("Unknown message, type: %02X", m_buffer[2U]);
				CUtils::dump("Buffer dump", m_buffer, m_length);
				break;
		}
	}

	// Only feed data to the modem if the playout timer has expired
	m_playoutTimer.clock(ms);
	if (!m_playoutTimer.hasExpired())
		return;

	if (m_dstarSpace > 1U && !m_txDStarData.isEmpty()) {
		unsigned char buffer[4U];
		m_txDStarData.peek(buffer, 4U);

		if ((buffer[3U] == MMDVM_DSTAR_HEADER && m_dstarSpace > 4U) ||
			(buffer[3U] == MMDVM_DSTAR_DATA   && m_dstarSpace > 1U) ||
			(buffer[3U] == MMDVM_DSTAR_EOT    && m_dstarSpace > 1U)) {
			unsigned char len = 0U;
			m_txDStarData.getData(&len, 1U);
			m_txDStarData.getData(m_buffer, len);

			switch (buffer[3U]) {
			case MMDVM_DSTAR_HEADER:
				if (m_trace)
					CUtils::dump(1U, "TX D-Star Header", m_buffer, len);
				m_dstarSpace -= 4U;
				break;
			case MMDVM_DSTAR_DATA:
				if (m_trace)
					CUtils::dump(1U, "TX D-Star Data", m_buffer, len);
				m_dstarSpace -= 1U;
				break;
			default:
				if (m_trace)
					CUtils::dump(1U, "TX D-Star EOT", m_buffer, len);
				m_dstarSpace -= 1U;
				break;
			}

			int ret = m_serial.write(m_buffer, len);
			if (ret != int(len))
				LogWarning("Error when writing D-Star data to the MMDVM");

			m_playoutTimer.start();
		}
	}

	if (m_dmrSpace1 > 1U && !m_txDMRData1.isEmpty()) {
		unsigned char len = 0U;
		m_txDMRData1.getData(&len, 1U);
		m_txDMRData1.getData(m_buffer, len);

		if (m_trace)
			CUtils::dump(1U, "TX DMR Data 1", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing DMR data to the MMDVM");

		m_playoutTimer.start();

		m_dmrSpace1--;
	}

	if (m_dmrSpace2 > 1U && !m_txDMRData2.isEmpty()) {
		unsigned char len = 0U;
		m_txDMRData2.getData(&len, 1U);
		m_txDMRData2.getData(m_buffer, len);

		if (m_trace)
			CUtils::dump(1U, "TX DMR Data 2", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing DMR data to the MMDVM");

		m_playoutTimer.start();

		m_dmrSpace2--;
	}

	if (m_ysfSpace > 1U && !m_txYSFData.isEmpty()) {
		unsigned char len = 0U;
		m_txYSFData.getData(&len, 1U);
		m_txYSFData.getData(m_buffer, len);

		if (m_trace)
			CUtils::dump(1U, "TX YSF Data", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing YSF data to the MMDVM");

		m_playoutTimer.start();

		m_ysfSpace--;
	}

	if (m_p25Space > 1U && !m_txP25Data.isEmpty()) {
		unsigned char len = 0U;
		m_txP25Data.getData(&len, 1U);
		m_txP25Data.getData(m_buffer, len);

		if (m_trace) {
			if (m_buffer[2U] == MMDVM_P25_HDR)
				CUtils::dump(1U, "TX P25 HDR", m_buffer, len);
			else
				CUtils::dump(1U, "TX P25 LDU", m_buffer, len);
		}

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing P25 data to the MMDVM");

		m_playoutTimer.start();

		m_p25Space--;
	}

	if (m_nxdnSpace > 1U && !m_txNXDNData.isEmpty()) {
		unsigned char len = 0U;
		m_txNXDNData.getData(&len, 1U);
		m_txNXDNData.getData(m_buffer, len);

		if (m_trace)
			CUtils::dump(1U, "TX NXDN Data", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing NXDN data to the MMDVM");

		m_playoutTimer.start();

		m_nxdnSpace--;
	}

	if (m_pocsagSpace > 1U && !m_txPOCSAGData.isEmpty()) {
		unsigned char len = 0U;
		m_txPOCSAGData.getData(&len, 1U);
		m_txPOCSAGData.getData(m_buffer, len);

		if (m_trace)
			CUtils::dump(1U, "TX POCSAG Data", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing POCSAG data to the MMDVM");

		m_playoutTimer.start();

		m_pocsagSpace--;
	}

	if (!m_txTransparentData.isEmpty()) {
		unsigned char len = 0U;
		m_txTransparentData.getData(&len, 1U);
		m_txTransparentData.getData(m_buffer, len);

		if (m_trace)
			CUtils::dump(1U, "TX Transparent Data", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing Transparent data to the MMDVM");
	}
}

void CModem::close()
{
	::LogMessage("Closing the MMDVM");

	m_serial.close();
}

unsigned int CModem::readDStarData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDStarData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDStarData.getData(&len, 1U);
	m_rxDStarData.getData(data, len);

	return len;
}

unsigned int CModem::readDMRData1(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDMRData1.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDMRData1.getData(&len, 1U);
	m_rxDMRData1.getData(data, len);

	return len;
}

unsigned int CModem::readDMRData2(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDMRData2.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDMRData2.getData(&len, 1U);
	m_rxDMRData2.getData(data, len);

	return len;
}

unsigned int CModem::readYSFData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxYSFData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxYSFData.getData(&len, 1U);
	m_rxYSFData.getData(data, len);

	return len;
}

unsigned int CModem::readP25Data(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxP25Data.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxP25Data.getData(&len, 1U);
	m_rxP25Data.getData(data, len);

	return len;
}

unsigned int CModem::readNXDNData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxNXDNData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxNXDNData.getData(&len, 1U);
	m_rxNXDNData.getData(data, len);

	return len;
}

unsigned int CModem::readTransparentData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxTransparentData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxTransparentData.getData(&len, 1U);
	m_rxTransparentData.getData(data, len);

	return len;
}

// To be implemented later if needed
unsigned int CModem::readSerial(unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	return 0U;
}

bool CModem::hasDStarSpace() const
{
	unsigned int space = m_txDStarData.freeSpace() / (DSTAR_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeDStarData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;

	switch (data[0U]) {
		case TAG_HEADER:
			buffer[2U] = MMDVM_DSTAR_HEADER;
			break;
		case TAG_DATA:
			buffer[2U] = MMDVM_DSTAR_DATA;
			break;
		case TAG_EOT:
			buffer[2U] = MMDVM_DSTAR_EOT;
			break;
		default:
			CUtils::dump(2U, "Unknown D-Star packet type", data, length);
			return false;
	}

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txDStarData.addData(&len, 1U);
	m_txDStarData.addData(buffer, len);

	return true;
}

bool CModem::hasDMRSpace1() const
{
	unsigned int space = m_txDMRData1.freeSpace() / (DMR_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::hasDMRSpace2() const
{
	unsigned int space = m_txDMRData2.freeSpace() / (DMR_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeDMRData1(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[40U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = MMDVM_DMR_DATA1;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txDMRData1.addData(&len, 1U);
	m_txDMRData1.addData(buffer, len);

	return true;
}

bool CModem::writeDMRData2(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[40U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = MMDVM_DMR_DATA2;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txDMRData2.addData(&len, 1U);
	m_txDMRData2.addData(buffer, len);

	return true;
}

bool CModem::hasYSFSpace() const
{
	unsigned int space = m_txYSFData.freeSpace() / (YSF_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeYSFData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[130U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = MMDVM_YSF_DATA;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txYSFData.addData(&len, 1U);
	m_txYSFData.addData(buffer, len);

	return true;
}

bool CModem::hasP25Space() const
{
	unsigned int space = m_txP25Data.freeSpace() / (P25_LDU_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeP25Data(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_HEADER && data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[250U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = (data[0U] == TAG_HEADER) ? MMDVM_P25_HDR : MMDVM_P25_LDU;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txP25Data.addData(&len, 1U);
	m_txP25Data.addData(buffer, len);

	return true;
}

bool CModem::hasNXDNSpace() const
{
	unsigned int space = m_txNXDNData.freeSpace() / (NXDN_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeNXDNData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[130U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = MMDVM_NXDN_DATA;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txNXDNData.addData(&len, 1U);
	m_txNXDNData.addData(buffer, len);

	return true;
}

bool CModem::hasPOCSAGSpace() const
{
	unsigned int space = m_txPOCSAGData.freeSpace() / (POCSAG_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writePOCSAGData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	unsigned char buffer[130U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 3U;
	buffer[2U] = MMDVM_POCSAG_DATA;

	::memcpy(buffer + 3U, data, length);

	unsigned char len = length + 3U;		// XXX Check lengths
	m_txPOCSAGData.addData(&len, 1U);
	m_txPOCSAGData.addData(buffer, len);

	return true;
}

bool CModem::writeTransparentData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	unsigned char buffer[250U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 3U;
	buffer[2U] = MMDVM_TRANSPARENT;

	::memcpy(buffer + 3U, data, length);

	unsigned char len = length + 3U;
	m_txTransparentData.addData(&len, 1U);
	m_txTransparentData.addData(buffer, len);

	return true;
}

bool CModem::writeSerial(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	unsigned char buffer[250U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 3U;
	buffer[2U] = MMDVM_SERIAL;

	::memcpy(buffer + 3U, data, length);

	int ret = m_serial.write(buffer, length + 3U);

	return ret != int(length + 3U);
}

bool CModem::hasTX() const
{
	return m_tx;
}

bool CModem::hasCD() const
{
	return m_cd;
}

bool CModem::hasLockout() const
{
	return m_lockout;
}

bool CModem::hasError() const
{
	return m_error;
}

bool CModem::readVersion()
{
	CThread::sleep(2000U);	// 2s

	for (unsigned int i = 0U; i < 6U; i++) {
		unsigned char buffer[3U];

		buffer[0U] = MMDVM_FRAME_START;
		buffer[1U] = 3U;
		buffer[2U] = MMDVM_GET_VERSION;

		// CUtils::dump(1U, "Written", buffer, 3U);

		int ret = m_serial.write(buffer, 3U);
		if (ret != 3)
			return false;

#if defined(__APPLE__)
		m_serial.setNonblock(true);
#endif

		for (unsigned int count = 0U; count < MAX_RESPONSES; count++) {
			CThread::sleep(10U);
			RESP_TYPE_MMDVM resp = getResponse();
			if (resp == RTM_OK && m_buffer[2U] == MMDVM_GET_VERSION) {
				if (::memcmp(m_buffer + 4U, "MMDVM ", 6U) == 0)
					m_hwType = HWT_MMDVM;
				else if (::memcmp(m_buffer + 4U, "DVMEGA", 6U) == 0)
					m_hwType = HWT_DVMEGA;
				else if (::memcmp(m_buffer + 4U, "ZUMspot", 7U) == 0)
					m_hwType = HWT_MMDVM_ZUMSPOT;
				else if (::memcmp(m_buffer + 4U, "MMDVM_HS_Hat", 12U) == 0)
					m_hwType = HWT_MMDVM_HS_HAT;
				else if (::memcmp(m_buffer + 4U, "MMDVM_HS_Dual_Hat", 17U) == 0)
					m_hwType = HWT_MMDVM_HS_DUAL_HAT;
				else if (::memcmp(m_buffer + 4U, "Nano_hotSPOT", 12U) == 0)
					m_hwType = HWT_NANO_HOTSPOT;
				else if (::memcmp(m_buffer + 4U, "Nano_DV", 7U) == 0)
					m_hwType = HWT_NANO_DV;
				else if (::memcmp(m_buffer + 4U, "MMDVM_HS-", 9U) == 0)
					m_hwType = HWT_MMDVM_HS;

				LogInfo("MMDVM protocol version: %u, description: %.*s", m_buffer[3U], m_length - 4U, m_buffer + 4U);
				return true;
			}
		}

		CThread::sleep(1500U);
	}

	LogError("Unable to read the firmware version after six attempts");

	return false;
}

bool CModem::readStatus()
{
	unsigned char buffer[3U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 3U;
	buffer[2U] = MMDVM_GET_STATUS;

	// CUtils::dump(1U, "Written", buffer, 3U);

	return m_serial.write(buffer, 3U) == 3;
}

bool CModem::setConfig()
{
	unsigned char buffer[30U];

	buffer[0U] = MMDVM_FRAME_START;

	buffer[1U] = 21U;

	buffer[2U] = MMDVM_SET_CONFIG;

	buffer[3U] = 0x00U;
	if (m_rxInvert)
		buffer[3U] |= 0x01U;
	if (m_txInvert)
		buffer[3U] |= 0x02U;
	if (m_pttInvert)
		buffer[3U] |= 0x04U;
	if (m_ysfLoDev)
		buffer[3U] |= 0x08U;
	if (m_debug)
		buffer[3U] |= 0x10U;
	if (!m_duplex)
		buffer[3U] |= 0x80U;

	buffer[4U] = 0x00U;
	if (m_dstarEnabled)
		buffer[4U] |= 0x01U;
	if (m_dmrEnabled)
		buffer[4U] |= 0x02U;
	if (m_ysfEnabled)
		buffer[4U] |= 0x04U;
	if (m_p25Enabled)
		buffer[4U] |= 0x08U;
	if (m_nxdnEnabled)
		buffer[4U] |= 0x10U;
	if (m_pocsagEnabled)
		buffer[4U] |= 0x20U;

	buffer[5U] = m_txDelay / 10U;		// In 10ms units

	buffer[6U] = MODE_IDLE;

	buffer[7U] = (unsigned char)(m_rxLevel * 2.55F + 0.5F);

	buffer[8U] = (unsigned char)(m_cwIdTXLevel * 2.55F + 0.5F);

	buffer[9U] = m_dmrColorCode;

	buffer[10U] = m_dmrDelay;

	buffer[11U] = 128U;           // Was OscOffset

	buffer[12U] = (unsigned char)(m_dstarTXLevel * 2.55F + 0.5F);
	buffer[13U] = (unsigned char)(m_dmrTXLevel * 2.55F + 0.5F);
	buffer[14U] = (unsigned char)(m_ysfTXLevel * 2.55F + 0.5F);
	buffer[15U] = (unsigned char)(m_p25TXLevel * 2.55F + 0.5F);

	buffer[16U] = (unsigned char)(m_txDCOffset + 128);
	buffer[17U] = (unsigned char)(m_rxDCOffset + 128);

	buffer[18U] = (unsigned char)(m_nxdnTXLevel * 2.55F + 0.5F);

	buffer[19U] = (unsigned char)m_ysfTXHang;

	buffer[20U] = (unsigned char)(m_pocsagTXLevel * 2.55F + 0.5F);

	// CUtils::dump(1U, "Written", buffer, 21U);

	int ret = m_serial.write(buffer, 21U);
	if (ret != 21)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		CThread::sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				LogError("The MMDVM is not responding to the SET_CONFIG command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump(1U, "Response", m_buffer, m_length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_CONFIG command from the modem");
		return false;
	}

	m_playoutTimer.start();

	return true;
}

bool CModem::setFrequency()
{
	unsigned char buffer[20U];
	unsigned char len;

	if (m_hwType == HWT_DVMEGA)
		len = 12U;
	else {
		buffer[12U]  = (unsigned char)(m_rfLevel * 2.55F + 0.5F);

		buffer[13U] = (m_pocsagFrequency >> 0)  & 0xFFU;
		buffer[14U] = (m_pocsagFrequency >> 8)  & 0xFFU;
		buffer[15U] = (m_pocsagFrequency >> 16) & 0xFFU;
		buffer[16U] = (m_pocsagFrequency >> 24) & 0xFFU;

		len = 17U;
	}

	buffer[0U]  = MMDVM_FRAME_START;

	buffer[1U]  = len;

	buffer[2U]  = MMDVM_SET_FREQ;

	buffer[3U]  = 0x00U;

	buffer[4U]  = (m_rxFrequency >> 0) & 0xFFU;
	buffer[5U]  = (m_rxFrequency >> 8) & 0xFFU;
	buffer[6U]  = (m_rxFrequency >> 16) & 0xFFU;
	buffer[7U]  = (m_rxFrequency >> 24) & 0xFFU;

	buffer[8U]  = (m_txFrequency >> 0) & 0xFFU;
	buffer[9U]  = (m_txFrequency >> 8) & 0xFFU;
	buffer[10U] = (m_txFrequency >> 16) & 0xFFU;
	buffer[11U] = (m_txFrequency >> 24) & 0xFFU;

	// CUtils::dump(1U, "Written", buffer, len);

	int ret = m_serial.write(buffer, len);
	if (ret != len)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		CThread::sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				LogError("The MMDVM is not responding to the SET_FREQ command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump(1U, "Response", m_buffer, m_length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_FREQ command from the modem");
		return false;
	}

	return true;
}

RESP_TYPE_MMDVM CModem::getResponse()
{
	if (m_offset == 0U) {
		// Get the start of the frame or nothing at all
		int ret = m_serial.read(m_buffer + 0U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the modem");
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		if (m_buffer[0U] != MMDVM_FRAME_START)
			return RTM_TIMEOUT;

		m_offset = 1U;
	}

	if (m_offset == 1U) {
		// Get the length of the frame
		int ret = m_serial.read(m_buffer + 1U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the modem");
			m_offset = 0U;
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		if (m_buffer[1U] >= 250U) {
			LogError("Invalid length received from the modem - %u", m_buffer[1U]);
			m_offset = 0U;
			return RTM_ERROR;
		}

		m_length = m_buffer[1U];
		m_offset = 2U;
	}

	if (m_offset == 2U) {
		// Get the frame type
		int ret = m_serial.read(m_buffer + 2U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the modem");
			m_offset = 0U;
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		m_offset = 3U;
	}

	if (m_offset >= 3U) {
		// Use later two byte length field
		if (m_length == 0U) {
			int ret = m_serial.read(m_buffer + 3U, 2U);
			if (ret < 0) {
				LogError("Error when reading from the modem");
				m_offset = 0U;
				return RTM_ERROR;
			}

			if (ret == 0)
				return RTM_TIMEOUT;

			m_length = (m_buffer[3U] << 8) | m_buffer[4U];
			m_offset = 5U;
		}

		while (m_offset < m_length) {
			int ret = m_serial.read(m_buffer + m_offset, m_length - m_offset);
			if (ret < 0) {
				LogError("Error when reading from the modem");
				m_offset = 0U;
				return RTM_ERROR;
			}

			if (ret == 0)
				return RTM_TIMEOUT;

			if (ret > 0)
				m_offset += ret;
		}
	}

	m_offset = 0U;

	// CUtils::dump(1U, "Received", m_buffer, m_length);

	return RTM_OK;
}

HW_TYPE CModem::getHWType() const
{
	return m_hwType;
}

bool CModem::setMode(unsigned char mode)
{
	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_SET_MODE;
	buffer[3U] = mode;

	// CUtils::dump(1U, "Written", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CModem::sendCWId(const std::string& callsign)
{
	unsigned int length = callsign.length();
	if (length > 200U)
		length = 200U;

	unsigned char buffer[205U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 3U;
	buffer[2U] = MMDVM_SEND_CWID;

	for (unsigned int i = 0U; i < length; i++)
		buffer[i + 3U] = callsign.at(i);

	// CUtils::dump(1U, "Written", buffer, length + 3U);

	return m_serial.write(buffer, length + 3U) == int(length + 3U);
}

bool CModem::writeDMRStart(bool tx)
{
	if (tx && m_tx)
		return true;
	if (!tx && !m_tx)
		return true;

	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_DMR_START;
	buffer[3U] = tx ? 0x01U : 0x00U;

	// CUtils::dump(1U, "Written", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CModem::writeDMRAbort(unsigned int slotNo)
{
	if (slotNo == 1U)
		m_txDMRData1.clear();
	else
		m_txDMRData2.clear();

	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_DMR_ABORT;
	buffer[3U] = slotNo;

	// CUtils::dump(1U, "Written", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CModem::writeDMRShortLC(const unsigned char* lc)
{
	assert(lc != NULL);

	unsigned char buffer[12U];

	buffer[0U]  = MMDVM_FRAME_START;
	buffer[1U]  = 12U;
	buffer[2U]  = MMDVM_DMR_SHORTLC;
	buffer[3U]  = lc[0U];
	buffer[4U]  = lc[1U];
	buffer[5U]  = lc[2U];
	buffer[6U]  = lc[3U];
	buffer[7U]  = lc[4U];
	buffer[8U]  = lc[5U];
	buffer[9U]  = lc[6U];
	buffer[10U] = lc[7U];
	buffer[11U] = lc[8U];

	// CUtils::dump(1U, "Written", buffer, 12U);

	return m_serial.write(buffer, 12U) == 12;
}

void CModem::printDebug()
{
	if (m_buffer[2U] == MMDVM_DEBUG1) {
		LogMessage("Debug: %.*s", m_length - 3U, m_buffer + 3U);
	} else if (m_buffer[2U] == MMDVM_DEBUG2) {
		short val1 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d", m_length - 5U, m_buffer + 3U, val1);
	} else if (m_buffer[2U] == MMDVM_DEBUG3) {
		short val1 = (m_buffer[m_length - 4U] << 8) | m_buffer[m_length - 3U];
		short val2 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d %d", m_length - 7U, m_buffer + 3U, val1, val2);
	} else if (m_buffer[2U] == MMDVM_DEBUG4) {
		short val1 = (m_buffer[m_length - 6U] << 8) | m_buffer[m_length - 5U];
		short val2 = (m_buffer[m_length - 4U] << 8) | m_buffer[m_length - 3U];
		short val3 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d %d %d", m_length - 9U, m_buffer + 3U, val1, val2, val3);
	} else if (m_buffer[2U] == MMDVM_DEBUG5) {
		short val1 = (m_buffer[m_length - 8U] << 8) | m_buffer[m_length - 7U];
		short val2 = (m_buffer[m_length - 6U] << 8) | m_buffer[m_length - 5U];
		short val3 = (m_buffer[m_length - 4U] << 8) | m_buffer[m_length - 3U];
		short val4 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d %d %d %d", m_length - 11U, m_buffer + 3U, val1, val2, val3, val4);
	}
}
