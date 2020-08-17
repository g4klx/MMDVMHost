/*
 *   Copyright (C) 2011-2018,2020 by Jonathan Naylor G4KLX
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

#include "I2CController.h"
#include "DStarDefines.h"
#include "DMRDefines.h"
#include "YSFDefines.h"
#include "P25Defines.h"
#include "NXDNDefines.h"
#include "POCSAGDefines.h"
#include "Thread.h"
#include "Modem.h"
#include "NullModem.h"
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

const unsigned char MMDVM_FM_PARAMS1  = 0x60U;
const unsigned char MMDVM_FM_PARAMS2  = 0x61U;
const unsigned char MMDVM_FM_PARAMS3  = 0x62U;

const unsigned char MMDVM_ACK         = 0x70U;
const unsigned char MMDVM_NAK         = 0x7FU;

const unsigned char MMDVM_SERIAL      = 0x80U;

const unsigned char MMDVM_TRANSPARENT = 0x90U;
const unsigned char MMDVM_QSO_INFO    = 0x91U;

const unsigned char MMDVM_DEBUG1      = 0xF1U;
const unsigned char MMDVM_DEBUG2      = 0xF2U;
const unsigned char MMDVM_DEBUG3      = 0xF3U;
const unsigned char MMDVM_DEBUG4      = 0xF4U;
const unsigned char MMDVM_DEBUG5      = 0xF5U;

const unsigned int MAX_RESPONSES = 30U;

const unsigned int BUFFER_LENGTH = 2000U;


CModem::CModem(const std::string& port, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, bool useCOSAsLockout, bool trace, bool debug) :
m_port(port),
m_dmrColorCode(0U),
m_ysfLoDev(false),
m_ysfTXHang(4U),
m_p25TXHang(5U),
m_nxdnTXHang(5U),
m_duplex(duplex),
m_rxInvert(rxInvert),
m_txInvert(txInvert),
m_pttInvert(pttInvert),
m_txDelay(txDelay),
m_dmrDelay(dmrDelay),
m_rxLevel(0.0F),
m_cwIdTXLevel(0.0F),
m_dstarTXLevel(0.0F),
m_dmrTXLevel(0.0F),
m_ysfTXLevel(0.0F),
m_p25TXLevel(0.0F),
m_nxdnTXLevel(0.0F),
m_pocsagTXLevel(0.0F),
m_fmTXLevel(0.0F),
m_rfLevel(0.0F),
m_useCOSAsLockout(useCOSAsLockout),
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
m_fmEnabled(false),
m_rxDCOffset(0),
m_txDCOffset(0),
m_serial(NULL),
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
m_sendTransparentDataFrameType(0U),
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
m_mode(MODE_IDLE),
m_hwType(HWT_UNKNOWN),
m_fmCallsign(),
m_fmCallsignSpeed(20U),
m_fmCallsignFrequency(1000U),
m_fmCallsignTime(600U),
m_fmCallsignHoldoff(0U),
m_fmCallsignHighLevel(35.0F),
m_fmCallsignLowLevel(15.0F),
m_fmCallsignAtStart(true),
m_fmCallsignAtEnd(true),
m_fmCallsignAtLatch(true),
m_fmRfAck("K"),
m_fmAckSpeed(20U),
m_fmAckFrequency(1750U),
m_fmAckMinTime(4U),
m_fmAckDelay(1000U),
m_fmAckLevel(80.0F),
m_fmTimeout(120U),
m_fmTimeoutLevel(80.0F),
m_fmCtcssFrequency(88.4F),
m_fmCtcssHighThreshold(30U),
m_fmCtcssLowThreshold(20U),
m_fmCtcssLevel(10.0F),
m_fmKerchunkTime(0U),
m_fmHangTime(5U),
m_fmAccessMode(1U),
m_fmCOSInvert(false),
m_fmRFAudioBoost(1U),
m_fmMaxDevLevel(90.0F)
{
	m_buffer = new unsigned char[BUFFER_LENGTH];

	assert(!port.empty());
}

CModem::~CModem()
{
	delete   m_serial;
	delete[] m_buffer;
}

void CModem::setSerialParams(const std::string& protocol, unsigned int address)
{
	// Create the serial controller instance according the protocol specified in conf.
	if (protocol == "i2c")
		m_serial = new CI2CController(m_port, SERIAL_115200, address, true);
	else
		m_serial = new CSerialController(m_port, SERIAL_115200, true);
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

void CModem::setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled, bool nxdnEnabled, bool pocsagEnabled, bool fmEnabled)
{
	m_dstarEnabled  = dstarEnabled;
	m_dmrEnabled    = dmrEnabled;
	m_ysfEnabled    = ysfEnabled;
	m_p25Enabled    = p25Enabled;
	m_nxdnEnabled   = nxdnEnabled;
	m_pocsagEnabled = pocsagEnabled;
	m_fmEnabled     = fmEnabled;
}

void CModem::setLevels(float rxLevel, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float pocsagTXLevel, float fmTXLevel)
{
	m_rxLevel       = rxLevel;
	m_cwIdTXLevel   = cwIdTXLevel;
	m_dstarTXLevel  = dstarTXLevel;
	m_dmrTXLevel    = dmrTXLevel;
	m_ysfTXLevel    = ysfTXLevel;
	m_p25TXLevel    = p25TXLevel;
	m_nxdnTXLevel   = nxdnTXLevel;
	m_pocsagTXLevel = pocsagTXLevel;
	m_fmTXLevel     = fmTXLevel;
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

void CModem::setP25Params(unsigned int txHang)
{
	m_p25TXHang = txHang;
}

void CModem::setNXDNParams(unsigned int txHang)
{
	m_nxdnTXHang = txHang;
}

void CModem::setTransparentDataParams(unsigned int sendFrameType)
{
    m_sendTransparentDataFrameType = sendFrameType;
}

bool CModem::open()
{
	::LogMessage("Opening the MMDVM");

	bool ret = m_serial->open();
	if (!ret)
		return false;

	ret = readVersion();
	if (!ret) {
		m_serial->close();
		delete m_serial;
		m_serial = NULL;
		return false;
	} else {
		/* Stopping the inactivity timer here when a firmware version has been
		   successfuly read prevents the death spiral of "no reply from modem..." */
		m_inactivityTimer.stop();
	}

	ret = setFrequency();
	if (!ret) {
		m_serial->close();
		delete m_serial;
		m_serial = NULL;
		return false;
	}

	ret = setConfig();
	if (!ret) {
		m_serial->close();
		delete m_serial;
		m_serial = NULL;
		return false;
	}

	if (m_fmEnabled && m_duplex) {
		ret = setFMCallsignParams();
		if (!ret) {
			m_serial->close();
			delete m_serial;
			m_serial = NULL;
			return false;
		}

		ret = setFMAckParams();
		if (!ret) {
			m_serial->close();
			delete m_serial;
			m_serial = NULL;
			return false;
		}

		ret = setFMMiscParams();
		if (!ret) {
			m_serial->close();
			delete m_serial;
			m_serial = NULL;
			return false;
		}
	}

	m_statusTimer.start();

	m_error  = false;
	m_offset = 0U;

	return true;
}

void CModem::clock(unsigned int ms)
{
	assert(m_serial != NULL);

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

					m_mode = m_buffer[4U];

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

					unsigned char offset = m_sendTransparentDataFrameType;
					if (offset > 1U) offset = 1U;
					unsigned char data = m_length - 3U + offset;
					m_rxTransparentData.addData(&data, 1U);

					m_rxTransparentData.addData(m_buffer + 3U - offset, m_length - 3U + offset);
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

			case MMDVM_SERIAL:
				//MMDVMHost does not process serial data from the display,
				// so we send it to the transparent port if sendFrameType==1
				if (m_sendTransparentDataFrameType > 0U) {
					if (m_trace)
						CUtils::dump(1U, "RX Serial Data", m_buffer, m_length);

					unsigned char offset = m_sendTransparentDataFrameType;
					if (offset > 1U) offset = 1U;
					unsigned char data = m_length - 3U + offset;
					m_rxTransparentData.addData(&data, 1U);

					m_rxTransparentData.addData(m_buffer + 3U - offset, m_length - 3U + offset);
					break; //only break when sendFrameType>0, else message is unknown
				}
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

			int ret = m_serial->write(m_buffer, len);
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

		int ret = m_serial->write(m_buffer, len);
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

		int ret = m_serial->write(m_buffer, len);
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

		int ret = m_serial->write(m_buffer, len);
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

		int ret = m_serial->write(m_buffer, len);
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

		int ret = m_serial->write(m_buffer, len);
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

		int ret = m_serial->write(m_buffer, len);
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

		int ret = m_serial->write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing Transparent data to the MMDVM");
	}
}

void CModem::close()
{
	assert(m_serial != NULL);

	::LogMessage("Closing the MMDVM");

	m_serial->close();
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

	unsigned char len = length + 3U;
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

	if (m_sendTransparentDataFrameType > 0U) {
		::memcpy(buffer + 2U, data, length);
		length--;
		buffer[1U]--;

		//when sendFrameType==1 , only 0x80 and 0x90 (MMDVM_SERIAL and MMDVM_TRANSPARENT) are allowed
		//  and reverted to default (MMDVM_TRANSPARENT) for any other value
		//when >1, frame type is not checked
		if (m_sendTransparentDataFrameType == 1U) {
			if ((buffer[2U] & 0xE0) != 0x80)
				buffer[2U] = MMDVM_TRANSPARENT;
		}
	} else {
		::memcpy(buffer + 3U, data, length);
	}

	unsigned char len = length + 3U;
	m_txTransparentData.addData(&len, 1U);
	m_txTransparentData.addData(buffer, len);

	return true;
}

bool CModem::writeDStarInfo(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(m_serial != NULL);
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 33U;
	buffer[2U] = MMDVM_QSO_INFO;

	buffer[3U] = MODE_DSTAR;

	::memcpy(buffer + 4U,  my1,  DSTAR_LONG_CALLSIGN_LENGTH);
	::memcpy(buffer + 12U, my2,  DSTAR_SHORT_CALLSIGN_LENGTH);

	::memcpy(buffer + 16U, your, DSTAR_LONG_CALLSIGN_LENGTH);

	::memcpy(buffer + 24U, type, 1U);

	::memcpy(buffer + 25U, reflector, DSTAR_LONG_CALLSIGN_LENGTH);

	return m_serial->write(buffer, 33U) != 33;
}

bool CModem::writeDMRInfo(unsigned int slotNo, const std::string& src, bool group, const std::string& dest, const char* type)
{
	assert(m_serial != NULL);
	assert(type != NULL);

	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 47U;
	buffer[2U] = MMDVM_QSO_INFO;

	buffer[3U] = MODE_DMR;

	buffer[4U] = slotNo;

	::sprintf((char*)(buffer + 5U), "%20.20s", src.c_str());

	buffer[25U] = group ? 'G' : 'I';

	::sprintf((char*)(buffer + 26U), "%20.20s", dest.c_str());

	::memcpy(buffer + 46U, type, 1U);

	return m_serial->write(buffer, 47U) != 47;
}

bool CModem::writeYSFInfo(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin)
{
	assert(m_serial != NULL);
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	unsigned char buffer[40U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 36U;
	buffer[2U] = MMDVM_QSO_INFO;

	buffer[3U] = MODE_YSF;

	::memcpy(buffer + 4U,  source, YSF_CALLSIGN_LENGTH);
	::memcpy(buffer + 14U, dest,   YSF_CALLSIGN_LENGTH);

	::memcpy(buffer + 24U, type, 1U);

	::memcpy(buffer + 25U, origin, YSF_CALLSIGN_LENGTH);

	buffer[35U] = dgid;

	return m_serial->write(buffer, 36U) != 36;
}

bool CModem::writeP25Info(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(m_serial != NULL);
	assert(source != NULL);
	assert(type != NULL);

	unsigned char buffer[40U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 31U;
	buffer[2U] = MMDVM_QSO_INFO;

	buffer[3U] = MODE_DMR;

	::sprintf((char*)(buffer + 4U), "%20.20s", source);

	buffer[24U] = group ? 'G' : 'I';

	::sprintf((char*)(buffer + 25U), "%05u", dest);	// 16-bits

	::memcpy(buffer + 30U, type, 1U);

	return m_serial->write(buffer, 31U) != 31;
}

bool CModem::writeNXDNInfo(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(m_serial != NULL);
	assert(source != NULL);
	assert(type != NULL);

	unsigned char buffer[40U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 31U;
	buffer[2U] = MMDVM_QSO_INFO;

	buffer[3U] = MODE_NXDN;

	::sprintf((char*)(buffer + 4U), "%20.20s", source);

	buffer[24U] = group ? 'G' : 'I';

	::sprintf((char*)(buffer + 25U), "%05u", dest);	// 16-bits

	::memcpy(buffer + 30U, type, 1U);

	return m_serial->write(buffer, 31U) != 31;
}

bool CModem::writePOCSAGInfo(unsigned int ric, const std::string& message)
{
	assert(m_serial != NULL);

	size_t length = message.size();

	unsigned char buffer[250U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 11U;
	buffer[2U] = MMDVM_QSO_INFO;

	buffer[3U] = MODE_POCSAG;

	::sprintf((char*)(buffer + 4U), "%07u", ric);	// 21-bits

	::memcpy(buffer + 11U, message.c_str(), length);

	int ret = m_serial->write(buffer, length + 11U);

	return ret != int(length + 11U);
}

bool CModem::writeIPInfo(const std::string& address)
{
	assert(m_serial != NULL);

	size_t length = address.size();

	unsigned char buffer[25U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 4U;
	buffer[2U] = MMDVM_QSO_INFO;

	buffer[3U] = 250U;

	::memcpy(buffer + 4U, address.c_str(), length);

	int ret = m_serial->write(buffer, length + 4U);

	return ret != int(length + 4U);
}

bool CModem::writeSerial(const unsigned char* data, unsigned int length)
{
	assert(m_serial != NULL);
	assert(data != NULL);
	assert(length > 0U);

	unsigned char buffer[250U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 3U;
	buffer[2U] = MMDVM_SERIAL;

	::memcpy(buffer + 3U, data, length);

	int ret = m_serial->write(buffer, length + 3U);

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
	assert(m_serial != NULL);

	CThread::sleep(2000U);	// 2s

	for (unsigned int i = 0U; i < 6U; i++) {
		unsigned char buffer[3U];

		buffer[0U] = MMDVM_FRAME_START;
		buffer[1U] = 3U;
		buffer[2U] = MMDVM_GET_VERSION;

		// CUtils::dump(1U, "Written", buffer, 3U);

		int ret = m_serial->write(buffer, 3U);
		if (ret != 3)
			return false;

#if defined(__APPLE__)
		m_serial->setNonblock(true);
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
				else if (::memcmp(m_buffer + 4U, "D2RG_MMDVM_HS", 13U) == 0)
					m_hwType = HWT_D2RG_MMDVM_HS;
				else if (::memcmp(m_buffer + 4U, "MMDVM_HS-", 9U) == 0)
					m_hwType = HWT_MMDVM_HS;
				else if (::memcmp(m_buffer + 4U, "OpenGD77_HS", 11U) == 0)
					m_hwType = HWT_OPENGD77_HS;

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
	assert(m_serial != NULL);

	unsigned char buffer[3U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 3U;
	buffer[2U] = MMDVM_GET_STATUS;

	// CUtils::dump(1U, "Written", buffer, 3U);

	return m_serial->write(buffer, 3U) == 3;
}

bool CModem::writeConfig()
{
	return setConfig();
}

bool CModem::setConfig()
{
	assert(m_serial != NULL);

	unsigned char buffer[30U];

	buffer[0U] = MMDVM_FRAME_START;

	buffer[1U] = 24U;

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
	if (m_useCOSAsLockout)
		buffer[3U] |= 0x20U;
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
	if (m_fmEnabled && m_duplex)
		buffer[4U] |= 0x40U;

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

	buffer[21U] = (unsigned char)(m_fmTXLevel * 2.55F + 0.5F);

	buffer[22U] = (unsigned char)m_p25TXHang;

	buffer[23U] = (unsigned char)m_nxdnTXHang;

	// CUtils::dump(1U, "Written", buffer, 24U);

	int ret = m_serial->write(buffer, 24U);
	if (ret != 24)
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
	assert(m_serial != NULL);

	unsigned char buffer[20U];
	unsigned char len;
	unsigned int  pocsagFrequency = 433000000U;

	if (m_pocsagEnabled)
		pocsagFrequency = m_pocsagFrequency;

	if (m_hwType == HWT_DVMEGA)
		len = 12U;
	else {
		buffer[12U]  = (unsigned char)(m_rfLevel * 2.55F + 0.5F);

		buffer[13U] = (pocsagFrequency >> 0)  & 0xFFU;
		buffer[14U] = (pocsagFrequency >> 8)  & 0xFFU;
		buffer[15U] = (pocsagFrequency >> 16) & 0xFFU;
		buffer[16U] = (pocsagFrequency >> 24) & 0xFFU;

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

	int ret = m_serial->write(buffer, len);
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
	assert(m_serial != NULL);

	if (m_offset == 0U) {
		// Get the start of the frame or nothing at all
		int ret = m_serial->read(m_buffer + 0U, 1U);
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
		int ret = m_serial->read(m_buffer + 1U, 1U);
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
		int ret = m_serial->read(m_buffer + 2U, 1U);
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
			int ret = m_serial->read(m_buffer + 3U, 2U);
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
			int ret = m_serial->read(m_buffer + m_offset, m_length - m_offset);
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

unsigned char CModem::getMode() const
{
	return m_mode;
}

bool CModem::setMode(unsigned char mode)
{
	assert(m_serial != NULL);

	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_SET_MODE;
	buffer[3U] = mode;

	// CUtils::dump(1U, "Written", buffer, 4U);

	return m_serial->write(buffer, 4U) == 4;
}

bool CModem::sendCWId(const std::string& callsign)
{
	assert(m_serial != NULL);

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

	return m_serial->write(buffer, length + 3U) == int(length + 3U);
}

bool CModem::writeDMRStart(bool tx)
{
	assert(m_serial != NULL);

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

	return m_serial->write(buffer, 4U) == 4;
}

bool CModem::writeDMRAbort(unsigned int slotNo)
{
	assert(m_serial != NULL);

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

	return m_serial->write(buffer, 4U) == 4;
}

bool CModem::writeDMRShortLC(const unsigned char* lc)
{
	assert(m_serial != NULL);
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

	return m_serial->write(buffer, 12U) == 12;
}

void CModem::setFMCallsignParams(const std::string& callsign, unsigned int callsignSpeed, unsigned int callsignFrequency, unsigned int callsignTime, unsigned int callsignHoldoff, float callsignHighLevel, float callsignLowLevel, bool callsignAtStart, bool callsignAtEnd, bool callsignAtLatch)
{
	m_fmCallsign          = callsign;
	m_fmCallsignSpeed     = callsignSpeed;
	m_fmCallsignFrequency = callsignFrequency;
	m_fmCallsignTime      = callsignTime;
	m_fmCallsignHoldoff   = callsignHoldoff;
	m_fmCallsignHighLevel = callsignHighLevel;
	m_fmCallsignLowLevel  = callsignLowLevel;
	m_fmCallsignAtStart   = callsignAtStart;
	m_fmCallsignAtEnd     = callsignAtEnd;
	m_fmCallsignAtLatch   = callsignAtLatch;
}

void CModem::setFMAckParams(const std::string& rfAck, unsigned int ackSpeed, unsigned int ackFrequency, unsigned int ackMinTime, unsigned int ackDelay, float ackLevel)
{
	m_fmRfAck        = rfAck;
	m_fmAckSpeed     = ackSpeed;
	m_fmAckFrequency = ackFrequency;
	m_fmAckMinTime   = ackMinTime;
	m_fmAckDelay     = ackDelay;
	m_fmAckLevel     = ackLevel;
}

void CModem::setFMMiscParams(unsigned int timeout, float timeoutLevel, float ctcssFrequency, unsigned int ctcssHighThreshold, unsigned int ctcssLowThreshold, float ctcssLevel, unsigned int kerchunkTime, unsigned int hangTime, unsigned int accessMode, bool cosInvert, unsigned int rfAudioBoost, float maxDevLevel)
{
	m_fmTimeout      = timeout;
	m_fmTimeoutLevel = timeoutLevel;

	m_fmCtcssFrequency     = ctcssFrequency;
	m_fmCtcssHighThreshold = ctcssHighThreshold;
	m_fmCtcssLowThreshold  = ctcssLowThreshold;
	m_fmCtcssLevel         = ctcssLevel;

	m_fmKerchunkTime = kerchunkTime;
	m_fmHangTime     = hangTime;

	m_fmAccessMode   = accessMode;
	m_fmCOSInvert    = cosInvert;

	m_fmRFAudioBoost = rfAudioBoost;
	m_fmMaxDevLevel  = maxDevLevel;
}

bool CModem::setFMCallsignParams()
{
	assert(m_serial != NULL);

	unsigned char buffer[80U];
	unsigned char len = 10U + m_fmCallsign.size();

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = len;
	buffer[2U] = MMDVM_FM_PARAMS1;

	buffer[3U] = m_fmCallsignSpeed;
	buffer[4U] = m_fmCallsignFrequency / 10U;
	buffer[5U] = m_fmCallsignTime;
	buffer[6U] = m_fmCallsignHoldoff;

	buffer[7U] = (unsigned char)(m_fmCallsignHighLevel * 2.55F + 0.5F);
	buffer[8U] = (unsigned char)(m_fmCallsignLowLevel * 2.55F + 0.5F);

	buffer[9U] = 0x00U;
	if (m_fmCallsignAtStart)
		buffer[9U] |= 0x01U;
	if (m_fmCallsignAtEnd)
		buffer[9U] |= 0x02U;
	if (m_fmCallsignAtLatch)
		buffer[9U] |= 0x04U;

	for (unsigned int i = 0U; i < m_fmCallsign.size(); i++)
		buffer[10U + i] = m_fmCallsign.at(i);

	// CUtils::dump(1U, "Written", buffer, len);

	int ret = m_serial->write(buffer, len);
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
				LogError("The MMDVM is not responding to the SET_FM_PARAMS1 command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump(1U, "Response", m_buffer, m_length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_FM_PARAMS1 command from the modem");
		return false;
	}

	return true;
}

bool CModem::setFMAckParams()
{
	assert(m_serial != NULL);

	unsigned char buffer[80U];
	unsigned char len = 8U + m_fmRfAck.size();

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = len;
	buffer[2U] = MMDVM_FM_PARAMS2;

	buffer[3U] = m_fmAckSpeed;
	buffer[4U] = m_fmAckFrequency / 10U;
	buffer[5U] = m_fmAckMinTime;
	buffer[6U] = m_fmAckDelay / 10U;

	buffer[7U] = (unsigned char)(m_fmAckLevel * 2.55F + 0.5F);

	for (unsigned int i = 0U; i < m_fmRfAck.size(); i++)
		buffer[8U + i] = m_fmRfAck.at(i);

	// CUtils::dump(1U, "Written", buffer, len);

	int ret = m_serial->write(buffer, len);
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
				LogError("The MMDVM is not responding to the SET_FM_PARAMS2 command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump(1U, "Response", m_buffer, m_length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_FM_PARAMS2 command from the modem");
		return false;
	}

	return true;
}

bool CModem::setFMMiscParams()
{
	assert(m_serial != NULL);

	unsigned char buffer[20U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 15U;
	buffer[2U] = MMDVM_FM_PARAMS3;

	buffer[3U] = m_fmTimeout / 5U;
	buffer[4U] = (unsigned char)(m_fmTimeoutLevel * 2.55F + 0.5F);

	buffer[5U] = (unsigned char)m_fmCtcssFrequency;
	buffer[6U] = m_fmCtcssHighThreshold;
	buffer[7U] = m_fmCtcssLowThreshold;
	buffer[8U] = (unsigned char)(m_fmCtcssLevel * 2.55F + 0.5F);

	buffer[9U]  = m_fmKerchunkTime;
	buffer[10U] = m_fmHangTime;

	buffer[11U] = m_fmAccessMode;
	if (m_fmCOSInvert)
		buffer[11U] |= 0x80U;

	buffer[12U] = m_fmRFAudioBoost;

	buffer[13U] = (unsigned char)(m_fmMaxDevLevel * 2.55F + 0.5F);

	buffer[14U] = (unsigned char)(m_rxLevel * 2.55F + 0.5F);

	// CUtils::dump(1U, "Written", buffer, 15U);

	int ret = m_serial->write(buffer, 15U);
	if (ret != 15)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		CThread::sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				LogError("The MMDVM is not responding to the SET_FM_PARAMS3 command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump(1U, "Response", m_buffer, m_length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_FM_PARAMS3 command from the modem");
		return false;
	}

	return true;
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

CModem* CModem::createModem(const std::string& port, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, bool useCOSAsLockout, bool trace, bool debug){
	if (port == "NullModem")
		return new CNullModem(port, duplex, rxInvert, txInvert, pttInvert, txDelay, dmrDelay, useCOSAsLockout, trace, debug);
	else
		return new CModem(port, duplex, rxInvert, txInvert, pttInvert, txDelay, dmrDelay, useCOSAsLockout, trace, debug);
}
