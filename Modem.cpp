/*
 *   Copyright (C) 2011-2016 by Jonathan Naylor G4KLX
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
#include "Defines.h"
#include "Modem.h"
#include "Utils.h"
#include "Log.h"

#include <cmath>
#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
typedef unsigned int uint32_t;
#else
#include <cstdint>
#include <unistd.h>
#endif

const unsigned char MMDVM_FRAME_START = 0xE0U;

const unsigned char MMDVM_GET_VERSION = 0x00U;
const unsigned char MMDVM_GET_STATUS  = 0x01U;
const unsigned char MMDVM_SET_CONFIG  = 0x02U;
const unsigned char MMDVM_SET_MODE    = 0x03U;

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

const unsigned char MMDVM_YSF_DATA    = 0x20U;
const unsigned char MMDVM_YSF_LOST    = 0x21U;

const unsigned char MMDVM_ACK         = 0x70U;
const unsigned char MMDVM_NAK         = 0x7FU;

const unsigned char MMDVM_DUMP        = 0xF0U;
const unsigned char MMDVM_DEBUG1      = 0xF1U;
const unsigned char MMDVM_DEBUG2      = 0xF2U;
const unsigned char MMDVM_DEBUG3      = 0xF3U;
const unsigned char MMDVM_DEBUG4      = 0xF4U;
const unsigned char MMDVM_DEBUG5      = 0xF5U;
const unsigned char MMDVM_SAMPLES     = 0xF8U;

const unsigned int MAX_RESPONSES = 30U;

const unsigned int BUFFER_LENGTH = 500U;


CModem::CModem(const std::string& port, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int rxLevel, unsigned int txLevel, bool debug) :
m_port(port),
m_colorCode(0U),
m_rxInvert(rxInvert),
m_txInvert(txInvert),
m_pttInvert(pttInvert),
m_txDelay(txDelay),
m_rxLevel(rxLevel),
m_txLevel(txLevel),
m_debug(debug),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_ysfEnabled(false),
m_serial(port, SERIAL_115200, true),
m_buffer(NULL),
m_rxDStarData(1000U),
m_txDStarData(1000U),
m_rxDMRData1(1000U),
m_rxDMRData2(1000U),
m_txDMRData1(1000U),
m_txDMRData2(1000U),
m_rxYSFData(1000U),
m_txYSFData(1000U),
m_statusTimer(1000U, 0U, 100U),
m_dstarSpace(0U),
m_dmrSpace1(0U),
m_dmrSpace2(0U),
m_ysfSpace(0U),
m_tx(false)
{
	assert(!port.empty());

	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CModem::~CModem()
{
	delete[] m_buffer;
}

void CModem::setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled)
{
	m_dstarEnabled = dstarEnabled;
	m_dmrEnabled   = dmrEnabled;
	m_ysfEnabled   = ysfEnabled;
}

void CModem::setDMRParams(unsigned int colorCode)
{
	assert(colorCode < 16U);

	m_colorCode = colorCode;
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
	}

	ret = setConfig();
	if (!ret) {
		m_serial.close();
		return false;
	}

	m_statusTimer.start();

	return true;
}

void CModem::clock(unsigned int ms)
{
	// Poll the modem status every 100ms
	m_statusTimer.clock(ms);
	if (m_statusTimer.hasExpired()) {
		readStatus();
		m_statusTimer.start();
	}

	unsigned int length;
	RESP_TYPE_MMDVM type = getResponse(m_buffer, length);

	if (type == RTM_TIMEOUT) {
		// Nothing to do
	} else if (type == RTM_ERROR) {
		LogError("Error when reading from the MMDVM");
	} else {
		// type == RTM_OK
		switch (m_buffer[2U]) {
			case MMDVM_DSTAR_HEADER: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star Header", m_buffer, length);

					unsigned char data = length - 2U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_HEADER;
					m_rxDStarData.addData(&data, 1U);

					m_rxDStarData.addData(m_buffer + 3U, length - 3U);
				}
				break;

			case MMDVM_DSTAR_DATA: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star Data", m_buffer, length);

					unsigned char data = length - 2U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_DATA;
					m_rxDStarData.addData(&data, 1U);

					m_rxDStarData.addData(m_buffer + 3U, length - 3U);
				}
				break;

			case MMDVM_DSTAR_LOST: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star Lost", m_buffer, length);

					unsigned char data = 1U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDStarData.addData(&data, 1U);
				}
				break;

			case MMDVM_DSTAR_EOT: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star EOT", m_buffer, length);

					unsigned char data = 1U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_EOT;
					m_rxDStarData.addData(&data, 1U);
				}
				break;

			case MMDVM_DMR_DATA1: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Data 1", m_buffer, length);

					unsigned char data = length - 2U;
					m_rxDMRData1.addData(&data, 1U);

					if (m_buffer[3U] == (DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC))
						data = TAG_EOT;
					else
						data = TAG_DATA;
					m_rxDMRData1.addData(&data, 1U);

					m_rxDMRData1.addData(m_buffer + 3U, length - 3U);
				}
				break;

			case MMDVM_DMR_DATA2: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Data 2", m_buffer, length);

					unsigned char data = length - 2U;
					m_rxDMRData2.addData(&data, 1U);

					if (m_buffer[3U] == (DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC))
						data = TAG_EOT;
					else
						data = TAG_DATA;
					m_rxDMRData2.addData(&data, 1U);

					m_rxDMRData2.addData(m_buffer + 3U, length - 3U);
				}
				break;

			case MMDVM_DMR_LOST1: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Lost 1", m_buffer, length);

					unsigned char data = 1U;
					m_rxDMRData1.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDMRData1.addData(&data, 1U);
				}
				break;

			case MMDVM_DMR_LOST2: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Lost 2", m_buffer, length);

					unsigned char data = 1U;
					m_rxDMRData2.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDMRData2.addData(&data, 1U);
				}
				break;

			case MMDVM_YSF_DATA: {
					if (m_debug)
						CUtils::dump(1U, "RX YSF Data", m_buffer, length);

					unsigned char data = length - 2U;
					m_rxYSFData.addData(&data, 1U);

					if ((m_buffer[3U] & (YSF_CKSUM_OK | YSF_FI_MASK)) == (YSF_CKSUM_OK | YSF_DT_TERMINATOR_CHANNEL))
						data = TAG_EOT;
					else
						data = TAG_DATA;
					m_rxYSFData.addData(&data, 1U);

					m_rxYSFData.addData(m_buffer + 3U, length - 3U);
				}
				break;

			case MMDVM_YSF_LOST: {
					if (m_debug)
						CUtils::dump(1U, "RX YSF Lost", m_buffer, length);

					unsigned char data = 1U;
					m_rxYSFData.addData(&data, 1U);

					data = TAG_LOST;
					m_rxYSFData.addData(&data, 1U);
				}
				break;

			case MMDVM_GET_STATUS: {
					// if (m_debug)
					//	CUtils::dump(1U, "GET_STATUS", m_buffer, length);

					m_tx = (m_buffer[5U] & 0x01U) == 0x01U;

					bool adcOverflow = (m_buffer[5U] & 0x02U) == 0x02U;
					if (adcOverflow)
						LogWarning("MMDVM ADC levels have overflowed");

					m_dstarSpace = m_buffer[6U];
					m_dmrSpace1  = m_buffer[7U];
					m_dmrSpace2  = m_buffer[8U];
					m_ysfSpace   = m_buffer[9U];
					// LogMessage("tx=%d, space=%u,%u,%u,%u", int(m_tx), m_dstarSpace, m_dmrSpace1, m_dmrSpace2, m_ysfSpace);
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

			case MMDVM_SAMPLES:
				// printSamples();
				break;

			default:
				LogMessage("Unknown message, type: %02X", m_buffer[2U]);
				CUtils::dump("Buffer dump", m_buffer, length);
				break;
		}
	}

	if (m_dstarSpace > 1U && !m_txDStarData.isEmpty()) {
		unsigned char len = 0U;
		m_txDStarData.getData(&len, 1U);
		m_txDStarData.getData(m_buffer, len);

		if (m_debug) {
			if (len > (DSTAR_FRAME_LENGTH_BYTES + 3U))
				CUtils::dump(1U, "TX D-Star Header", m_buffer, len);
			else if (len == (DSTAR_FRAME_LENGTH_BYTES + 3U))
				CUtils::dump(1U, "TX D-Star Data", m_buffer, len);
			else
				CUtils::dump(1U, "TX D-Star EOT", m_buffer, len);
		}

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing D-Star data to the MMDVM");

		// Headers take four slots in the queues in the modem
		if (len > (DSTAR_FRAME_LENGTH_BYTES + 3U))
			m_dstarSpace -= 4U;
		else
			m_dstarSpace -= 1U;
	}

	if (m_dmrSpace1 > 1U && !m_txDMRData1.isEmpty()) {
		unsigned char len = 0U;
		m_txDMRData1.getData(&len, 1U);
		m_txDMRData1.getData(m_buffer, len);

		if (m_debug)
			CUtils::dump(1U, "TX DMR Data 1", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing DMR data to the MMDVM");

		m_dmrSpace1--;
	}

	if (m_dmrSpace2 > 1U && !m_txDMRData2.isEmpty()) {
		unsigned char len = 0U;
		m_txDMRData2.getData(&len, 1U);
		m_txDMRData2.getData(m_buffer, len);

		if (m_debug)
			CUtils::dump(1U, "TX DMR Data 2", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing DMR data to the MMDVM");

		m_dmrSpace2--;
	}

	if (m_ysfSpace > 1U && !m_txYSFData.isEmpty()) {
		unsigned char len = 0U;
		m_txYSFData.getData(&len, 1U);
		m_txYSFData.getData(m_buffer, len);

		if (m_debug)
			CUtils::dump(1U, "TX YSF Data", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing YSF data to the MMDVM");

		m_ysfSpace--;
	}
}

void CModem::close()
{
	::LogMessage("Closing the MMDVM");

	delete[] m_buffer;
	
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
		case TAG_HEADER: buffer[2U] = MMDVM_DSTAR_HEADER; break;
		case TAG_DATA:   buffer[2U] = MMDVM_DSTAR_DATA;   break;
		case TAG_EOT:    buffer[2U] = MMDVM_DSTAR_EOT;    break;
		default: return false;
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

bool CModem::readVersion()
{
#if defined(_WIN32) || defined(_WIN64)
	::Sleep(2000UL);		// 2s
#else
	::sleep(2);			// 2s
#endif

	for (unsigned int i = 0U; i < 6U; i++) {
		unsigned char buffer[3U];

		buffer[0U] = MMDVM_FRAME_START;
		buffer[1U] = 3U;
		buffer[2U] = MMDVM_GET_VERSION;

		// CUtils::dump("Written", buffer, 3U);

		int ret = m_serial.write(buffer, 3U);
		if (ret != 3)
			return false;

		for (unsigned int count = 0U; count < MAX_RESPONSES; count++) {
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(10UL);
#else
			::usleep(10000UL);
#endif
			unsigned int length;
			RESP_TYPE_MMDVM resp = getResponse(m_buffer, length);
			if (resp == RTM_OK && m_buffer[2U] == MMDVM_GET_VERSION) {
				LogInfo("MMDVM protocol version: %u, description: %.*s", m_buffer[3U], length - 4U, m_buffer + 4U);
				return true;
			}
		}

#if defined(_WIN32) || defined(_WIN64)
		::Sleep(1000UL);		// 1s
#else
		::sleep(1UL);			// 1s
#endif
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

	return m_serial.write(buffer, 3U) == 3;
}

bool CModem::setConfig()
{
	unsigned char buffer[10U];

	buffer[0U] = MMDVM_FRAME_START;

	buffer[1U] = 10U;

	buffer[2U] = MMDVM_SET_CONFIG;

	buffer[3U] = 0x00U;
	if (m_rxInvert)
		buffer[3U] |= 0x01U;
	if (m_txInvert)
		buffer[3U] |= 0x02U;
	if (m_pttInvert)
		buffer[3U] |= 0x04U;

	buffer[4U] = 0x00U;
	if (m_dstarEnabled)
		buffer[4U] |= 0x01U;
	if (m_dmrEnabled)
		buffer[4U] |= 0x02U;
	if (m_ysfEnabled)
		buffer[4U] |= 0x04U;

	buffer[5U] = m_txDelay / 10U;		// In 10ms units

	buffer[6U] = MODE_IDLE;

	buffer[7U] = (m_rxLevel * 255U) / 100U;
	buffer[8U] = (m_txLevel * 255U) / 100U;

	buffer[9U] = m_colorCode;

	// CUtils::dump("Written", buffer, 10U);

	int ret = m_serial.write(buffer, 10U);
	if (ret != 10)
		return false;

	unsigned int count = 0U;
	unsigned int length;
	RESP_TYPE_MMDVM resp;
	do {
#if defined(_WIN32) || defined(_WIN64)
		::Sleep(10UL);
#else
		::usleep(10000UL);
#endif
		resp = getResponse(m_buffer, length);

		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				LogError("The MMDVM is not responding to the SET_CONFIG command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump("Response", m_buffer, length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_CONFIG command from the modem");
		return false;
	}

	return true;
}

RESP_TYPE_MMDVM CModem::getResponse(unsigned char *buffer, unsigned int& length)
{
	// Get the start of the frame or nothing at all
	int ret = m_serial.read(buffer + 0U, 1U);
	if (ret < 0) {
		LogError("Error when reading from the modem");
		return RTM_ERROR;
	}

	if (ret == 0)
		return RTM_TIMEOUT;

	if (buffer[0U] != MMDVM_FRAME_START)
		return RTM_TIMEOUT;

	ret = m_serial.read(buffer + 1U, 1U);
	if (ret < 0) {
		LogError("Error when reading from the modem");
		return RTM_ERROR;
	}

	if (ret == 0)
		return RTM_TIMEOUT;

	length = buffer[1U];

	if (length >= 200U) {
		LogError("Invalid data received from the modem");
		return RTM_ERROR;
	}

	unsigned int offset = 2U;

	while (offset < length) {
		int ret = m_serial.read(buffer + offset, length - offset);
		if (ret < 0) {
			LogError("Error when reading from the modem");
			return RTM_ERROR;
		}

		if (ret > 0)
			offset += ret;

		if (ret == 0)
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(5UL);		// 5ms
#else
			::usleep(5000);		// 5ms
#endif
	}

	// CUtils::dump("Received", buffer, length);

	return RTM_OK;
}

bool CModem::setMode(unsigned char mode)
{
	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_SET_MODE;
	buffer[3U] = mode;

	// CUtils::dump("Written", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
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

	// CUtils::dump("Written", buffer, 4U);

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

	// CUtils::dump("Written", buffer, 12U);

	return m_serial.write(buffer, 12U) == 12;
}

void CModem::printDebug()
{
	unsigned int length = m_buffer[1U];
	if (m_buffer[2U] == 0xF1U) {
		LogMessage("Debug: %.*s", length - 3U, m_buffer + 3U);
	} else if (m_buffer[2U] == 0xF2U) {
		short val1 = (m_buffer[length - 2U] << 8) | m_buffer[length - 1U];
		LogMessage("Debug: %.*s %d", length - 5U, m_buffer + 3U, val1);
	} else if (m_buffer[2U] == 0xF3U) {
		short val1 = (m_buffer[length - 4U] << 8) | m_buffer[length - 3U];
		short val2 = (m_buffer[length - 2U] << 8) | m_buffer[length - 1U];
		LogMessage("Debug: %.*s %d %d", length - 7U, m_buffer + 3U, val1, val2);
	} else if (m_buffer[2U] == 0xF4U) {
		short val1 = (m_buffer[length - 6U] << 8) | m_buffer[length - 5U];
		short val2 = (m_buffer[length - 4U] << 8) | m_buffer[length - 3U];
		short val3 = (m_buffer[length - 2U] << 8) | m_buffer[length - 1U];
		LogMessage("Debug: %.*s %d %d %d", length - 9U, m_buffer + 3U, val1, val2, val3);
	} else if (m_buffer[2U] == 0xF5U) {
		short val1 = (m_buffer[length - 8U] << 8) | m_buffer[length - 7U];
		short val2 = (m_buffer[length - 6U] << 8) | m_buffer[length - 5U];
		short val3 = (m_buffer[length - 4U] << 8) | m_buffer[length - 3U];
		short val4 = (m_buffer[length - 2U] << 8) | m_buffer[length - 1U];
		LogMessage("Debug: %.*s %d %d %d %d", length - 11U, m_buffer + 3U, val1, val2, val3, val4);
	}
}
