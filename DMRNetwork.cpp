/*
 *   Copyright (C) 2015,2016,2017,2018 by Jonathan Naylor G4KLX
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

#include "DMRNetwork.h"

#include "StopWatch.h"
#include "SHA256.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>

const unsigned int BUFFER_LENGTH = 500U;

const unsigned int HOMEBREW_DATA_PACKET_LENGTH = 55U;


CDMRNetwork::CDMRNetwork(const std::string& address, unsigned int port, unsigned int local, unsigned int id, const std::string& password, bool duplex, const char* version, bool debug, bool slot1, bool slot2, HW_TYPE hwType) :
m_addressStr(address),
m_address(),
m_port(port),
m_id(NULL),
m_password(password),
m_duplex(duplex),
m_version(version),
m_debug(debug),
m_socket(local),
m_enabled(false),
m_slot1(slot1),
m_slot2(slot2),
m_hwType(hwType),
m_status(WAITING_CONNECT),
m_retryTimer(1000U, 10U),
m_timeoutTimer(1000U, 60U),
m_buffer(NULL),
m_salt(NULL),
m_streamId(NULL),
m_rxData(1000U, "DMR Network"),
m_options(),
m_callsign(),
m_rxFrequency(0U),
m_txFrequency(0U),
m_power(0U),
m_colorCode(0U),
m_latitude(0.0F),
m_longitude(0.0F),
m_height(0),
m_location(),
m_description(),
m_url(),
m_beacon(false)
{
	assert(!address.empty());
	assert(port > 0U);
	assert(id > 1000U);
	assert(!password.empty());

	m_address = CUDPSocket::lookup(address);

	m_buffer   = new unsigned char[BUFFER_LENGTH];
	m_salt     = new unsigned char[sizeof(uint32_t)];
	m_id       = new uint8_t[4U];
	m_streamId = new uint32_t[2U];

	m_id[0U] = id >> 24;
	m_id[1U] = id >> 16;
	m_id[2U] = id >> 8;
	m_id[3U] = id >> 0;

	CStopWatch stopWatch;
	::srand(stopWatch.start());

	m_streamId[0U] = ::rand() + 1U;
	m_streamId[1U] = ::rand() + 1U;
}

CDMRNetwork::~CDMRNetwork()
{
	delete[] m_buffer;
	delete[] m_salt;
	delete[] m_streamId;
	delete[] m_id;
}

void CDMRNetwork::setOptions(const std::string& options)
{
	m_options = options;
}

void CDMRNetwork::setConfig(const std::string& callsign, unsigned int rxFrequency, unsigned int txFrequency, unsigned int power, unsigned int colorCode, float latitude, float longitude, int height, const std::string& location, const std::string& description, const std::string& url)
{
	m_callsign    = callsign;
	m_rxFrequency = rxFrequency;
	m_txFrequency = txFrequency;
	m_power       = power;
	m_colorCode   = colorCode;
	m_latitude    = latitude;
	m_longitude   = longitude;
	m_height      = height;
	m_location    = location;
	m_description = description;
	m_url         = url;
}

bool CDMRNetwork::open()
{
	LogMessage("DMR, Opening DMR Network");

	if (m_address.s_addr == INADDR_NONE)
		m_address = CUDPSocket::lookup(m_addressStr);

	m_status = WAITING_CONNECT;
	m_timeoutTimer.stop();
	m_retryTimer.start();

	return true;
}

void CDMRNetwork::enable(bool enabled)
{
	m_enabled = enabled;
}

bool CDMRNetwork::read(CDMRData& data)
{
	if (m_status != RUNNING)
		return false;

	if (m_rxData.isEmpty())
		return false;

	unsigned char length = 0U;
	m_rxData.getData(&length, 1U);
	m_rxData.getData(m_buffer, length);

	// Is this a data packet?
	if (::memcmp(m_buffer, "DMRD", 4U) != 0)
		return false;

	unsigned char seqNo = m_buffer[4U];

	unsigned int srcId = (m_buffer[5U] << 16) | (m_buffer[6U] << 8) | (m_buffer[7U] << 0);

	unsigned int dstId = (m_buffer[8U] << 16) | (m_buffer[9U] << 8) | (m_buffer[10U] << 0);

	unsigned int slotNo = (m_buffer[15U] & 0x80U) == 0x80U ? 2U : 1U;

	// DMO mode slot disabling
	if (slotNo == 1U && !m_duplex)
		return false;

	// Individual slot disabling
	if (slotNo == 1U && !m_slot1)
		return false;
	if (slotNo == 2U && !m_slot2)
		return false;

	FLCO flco = (m_buffer[15U] & 0x40U) == 0x40U ? FLCO_USER_USER : FLCO_GROUP;

	data.setSeqNo(seqNo);
	data.setSlotNo(slotNo);
	data.setSrcId(srcId);
	data.setDstId(dstId);
	data.setFLCO(flco);

	bool dataSync = (m_buffer[15U] & 0x20U) == 0x20U;
	bool voiceSync = (m_buffer[15U] & 0x10U) == 0x10U;

	if (dataSync) {
		unsigned char dataType = m_buffer[15U] & 0x0FU;
		data.setData(m_buffer + 20U);
		data.setDataType(dataType);
		data.setN(0U);
	} else if (voiceSync) {
		data.setData(m_buffer + 20U);
		data.setDataType(DT_VOICE_SYNC);
		data.setN(0U);
	} else {
		unsigned char n = m_buffer[15U] & 0x0FU;
		data.setData(m_buffer + 20U);
		data.setDataType(DT_VOICE);
		data.setN(n);
	}

	return true;
}

bool CDMRNetwork::write(const CDMRData& data)
{
	if (m_status != RUNNING)
		return false;

	unsigned char buffer[HOMEBREW_DATA_PACKET_LENGTH];
	::memset(buffer, 0x00U, HOMEBREW_DATA_PACKET_LENGTH);

	buffer[0U]  = 'D';
	buffer[1U]  = 'M';
	buffer[2U]  = 'R';
	buffer[3U]  = 'D';

	unsigned int srcId = data.getSrcId();
	buffer[5U]  = srcId >> 16;
	buffer[6U]  = srcId >> 8;
	buffer[7U]  = srcId >> 0;

	unsigned int dstId = data.getDstId();
	buffer[8U]  = dstId >> 16;
	buffer[9U]  = dstId >> 8;
	buffer[10U] = dstId >> 0;

	::memcpy(buffer + 11U, m_id, 4U);

	unsigned int slotNo = data.getSlotNo();

	// Individual slot disabling
	if (slotNo == 1U && !m_slot1)
		return false;
	if (slotNo == 2U && !m_slot2)
		return false;

	buffer[15U] = slotNo == 1U ? 0x00U : 0x80U;

	FLCO flco = data.getFLCO();
	buffer[15U] |= flco == FLCO_GROUP ? 0x00U : 0x40U;

	unsigned int slotIndex = slotNo - 1U;

	unsigned int count = 1U;

	unsigned char dataType = data.getDataType();
	if (dataType == DT_VOICE_SYNC) {
		buffer[15U] |= 0x10U;
	} else if (dataType == DT_VOICE) {
		buffer[15U] |= data.getN();
	} else {
		if (dataType == DT_VOICE_LC_HEADER) {
			m_streamId[slotIndex] = ::rand() + 1U;
			count = 2U;
		}

		if (dataType == DT_CSBK || dataType == DT_DATA_HEADER) {
			m_streamId[slotIndex] = ::rand() + 1U;
			count = 1U;
		}

		buffer[15U] |= (0x20U | dataType);
	}

	buffer[4U] = data.getSeqNo();

	::memcpy(buffer + 16U, m_streamId + slotIndex, 4U);

	data.getData(buffer + 20U);

	buffer[53U] = data.getBER();

	buffer[54U] = data.getRSSI();

	if (m_debug)
		CUtils::dump(1U, "Network Transmitted", buffer, HOMEBREW_DATA_PACKET_LENGTH);

	for (unsigned int i = 0U; i < count; i++)
		write(buffer, HOMEBREW_DATA_PACKET_LENGTH);

	return true;
}

bool CDMRNetwork::writePosition(unsigned int id, const unsigned char* data)
{
	if (m_status != RUNNING)
		return false;

	unsigned char buffer[20U];

	::memcpy(buffer + 0U, "DMRG", 4U);

	::memcpy(buffer + 4U, m_id, 4U);

	buffer[8U]  = id >> 16;
	buffer[9U]  = id >> 8;
	buffer[10U] = id >> 0;

	::memcpy(buffer + 11U, data + 2U, 7U);

	return write(buffer, 18U);
}

bool CDMRNetwork::writeTalkerAlias(unsigned int id, unsigned char type, const unsigned char* data)
{
	if (m_status != RUNNING)
		return false;

	unsigned char buffer[20U];

	::memcpy(buffer + 0U, "DMRA", 4U);

	::memcpy(buffer + 4U, m_id, 4U);

	buffer[8U]  = id >> 16;
	buffer[9U]  = id >> 8;
	buffer[10U] = id >> 0;

	buffer[11U] = type;

	::memcpy(buffer + 12U, data + 2U, 7U);

	return write(buffer, 19U);
}

void CDMRNetwork::close()
{
	LogMessage("DMR, Closing DMR Network");

	if (m_status == RUNNING) {
		unsigned char buffer[9U];
		::memcpy(buffer + 0U, "RPTCL", 5U);
		::memcpy(buffer + 5U, m_id, 4U);
		write(buffer, 9U);
	}

	m_socket.close();

	m_retryTimer.stop();
	m_timeoutTimer.stop();
}

void CDMRNetwork::clock(unsigned int ms)
{
	if (m_status == WAITING_CONNECT) {
		m_retryTimer.clock(ms);
		if (m_retryTimer.isRunning() && m_retryTimer.hasExpired()) {
			bool ret = m_socket.open();
			if (ret) {
				ret = writeLogin();
				if (!ret)
					return;

				m_status = WAITING_LOGIN;
				m_timeoutTimer.start();
			}

			m_retryTimer.start();
		}

		return;
	}

	in_addr address;
	unsigned int port;
	int length = m_socket.read(m_buffer, BUFFER_LENGTH, address, port);
	if (length < 0) {
		LogError("DMR, Socket has failed, retrying connection to the master");
		close();
		open();
		return;
	}

	// if (m_debug && length > 0)
	//	CUtils::dump(1U, "Network Received", m_buffer, length);

	if (length > 0 && m_address.s_addr == address.s_addr && m_port == port) {
		if (::memcmp(m_buffer, "DMRD", 4U) == 0) {
			if (m_enabled) {
				if (m_debug)
					CUtils::dump(1U, "Network Received", m_buffer, length);

				unsigned char len = length;
				m_rxData.addData(&len, 1U);
				m_rxData.addData(m_buffer, len);
			}
		} else if (::memcmp(m_buffer, "MSTNAK",  6U) == 0) {
			if (m_status == RUNNING) {
				LogWarning("DMR, Login to the master has failed, retrying login ...");
				m_status = WAITING_LOGIN;
				m_timeoutTimer.start();
				m_retryTimer.start();
			} else {
				/* Once the modem death spiral has been prevented in Modem.cpp
				   the Network sometimes times out and reaches here.
				   We want it to reconnect so... */
				LogError("DMR, Login to the master has failed, retrying network ...");
				close();
				open();
				return;
			}
		} else if (::memcmp(m_buffer, "RPTACK",  6U) == 0) {
			switch (m_status) {
				case WAITING_LOGIN:
					LogDebug("DMR, Sending authorisation");
					::memcpy(m_salt, m_buffer + 6U, sizeof(uint32_t));
					writeAuthorisation();
					m_status = WAITING_AUTHORISATION;
					m_timeoutTimer.start();
					m_retryTimer.start();
					break;
				case WAITING_AUTHORISATION:
					LogDebug("DMR, Sending configuration");
					writeConfig();
					m_status = WAITING_CONFIG;
					m_timeoutTimer.start();
					m_retryTimer.start();
					break;
				case WAITING_CONFIG:
					if (m_options.empty()) {
						LogMessage("DMR, Logged into the master successfully");
						m_status = RUNNING;
					} else {
						LogDebug("DMR, Sending options");
						writeOptions();
						m_status = WAITING_OPTIONS;
					}
					m_timeoutTimer.start();
					m_retryTimer.start();
					break;
				case WAITING_OPTIONS:
					LogMessage("DMR, Logged into the master successfully");
					m_status = RUNNING;
					m_timeoutTimer.start();
					m_retryTimer.start();
					break;
				default:
					break;
			}
		} else if (::memcmp(m_buffer, "MSTCL",   5U) == 0) {
			LogError("DMR, Master is closing down");
			close();
			open();
		} else if (::memcmp(m_buffer, "MSTPONG", 7U) == 0) {
			m_timeoutTimer.start();
		} else if (::memcmp(m_buffer, "RPTSBKN", 7U) == 0) {
			m_beacon = true;
		} else {
			CUtils::dump("Unknown packet from the master", m_buffer, length);
		}
	}

	m_retryTimer.clock(ms);
	if (m_retryTimer.isRunning() && m_retryTimer.hasExpired()) {
		switch (m_status) {
			case WAITING_LOGIN:
				writeLogin();
				break;
			case WAITING_AUTHORISATION:
				writeAuthorisation();
				break;
			case WAITING_OPTIONS:
				writeOptions();
				break;
			case WAITING_CONFIG:
				writeConfig();
				break;
			case RUNNING:
				writePing();
				break;
			default:
				break;
		}

		m_retryTimer.start();
	}

	m_timeoutTimer.clock(ms);
	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired()) {
		LogError("DMR, Connection to the master has timed out, retrying connection");
		close();
		open();
	}
}

bool CDMRNetwork::writeLogin()
{
	unsigned char buffer[8U];

	::memcpy(buffer + 0U, "RPTL", 4U);
	::memcpy(buffer + 4U, m_id, 4U);

	return write(buffer, 8U);
}

bool CDMRNetwork::writeAuthorisation()
{
	size_t size = m_password.size();

	unsigned char* in = new unsigned char[size + sizeof(uint32_t)];
	::memcpy(in, m_salt, sizeof(uint32_t));
	for (size_t i = 0U; i < size; i++)
		in[i + sizeof(uint32_t)] = m_password.at(i);

	unsigned char out[40U];
	::memcpy(out + 0U, "RPTK", 4U);
	::memcpy(out + 4U, m_id, 4U);

	CSHA256 sha256;
	sha256.buffer(in, (unsigned int)(size + sizeof(uint32_t)), out + 8U);

	delete[] in;

	return write(out, 40U);
}

bool CDMRNetwork::writeOptions()
{
	char buffer[300U];

	::memcpy(buffer + 0U, "RPTO", 4U);
	::memcpy(buffer + 4U, m_id, 4U);
	::strcpy(buffer + 8U, m_options.c_str());

	return write((unsigned char*)buffer, (unsigned int)m_options.length() + 8U);
}

bool CDMRNetwork::writeConfig()
{
	const char* software;
	char slots = '0';
	if (m_duplex) {
		if (m_slot1 && m_slot2)
			slots = '3';
		else if (m_slot1 && !m_slot2)
			slots = '1';
		else if (!m_slot1 && m_slot2)
			slots = '2';

		switch (m_hwType) {
		case HWT_MMDVM:
			software = "MMDVM";
			break;
		case HWT_MMDVM_HS:
			software = "MMDVM_MMDVM_HS";
			break;
		case HWT_MMDVM_HS_DUAL_HAT:
			software = "MMDVM_MMDVM_HS_Dual_Hat";
			break;
		case HWT_NANO_HOTSPOT:
			software = "MMDVM_Nano_hotSPOT";
			break;
		default:
			software = "MMDVM_Unknown";
			break;
		}
	} else {
		slots = '4';

		switch (m_hwType) {
		case HWT_MMDVM:
			software = "MMDVM_DMO";
			break;
		case HWT_DVMEGA:
			software = "MMDVM_DVMega";
			break;
		case HWT_MMDVM_ZUMSPOT:
			software = "MMDVM_ZUMspot";
			break;
		case HWT_MMDVM_HS_HAT:
			software = "MMDVM_MMDVM_HS_Hat";
			break;
		case HWT_MMDVM_HS_DUAL_HAT:
			software = "MMDVM_MMDVM_HS_Dual_Hat";
			break;
		case HWT_NANO_HOTSPOT:
			software = "MMDVM_Nano_hotSPOT";
			break;
		case HWT_NANO_DV:
			software = "MMDVM_Nano_DV";
			break;
		case HWT_MMDVM_HS:
			software = "MMDVM_MMDVM_HS";
			break;
		default:
			software = "MMDVM_Unknown";
			break;
		}
	}

	char buffer[400U];

	::memcpy(buffer + 0U, "RPTC", 4U);
	::memcpy(buffer + 4U, m_id, 4U);

	char latitude[20U];
	::sprintf(latitude, "%08f", m_latitude);

	char longitude[20U];
	::sprintf(longitude, "%09f", m_longitude);

	unsigned int power = m_power;
	if (power > 99U)
		power = 99U;

	int height = m_height;
	if (height > 999)
		height = 999;

	::sprintf(buffer + 8U, "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s", m_callsign.c_str(),
		m_rxFrequency, m_txFrequency, power, m_colorCode, latitude, longitude, height, m_location.c_str(),
		m_description.c_str(), slots, m_url.c_str(), m_version, software);

	return write((unsigned char*)buffer, 302U);
}

bool CDMRNetwork::writePing()
{
	unsigned char buffer[11U];

	::memcpy(buffer + 0U, "RPTPING", 7U);
	::memcpy(buffer + 7U, m_id, 4U);

	return write(buffer, 11U);
}

bool CDMRNetwork::wantsBeacon()
{
	bool beacon = m_beacon;

	m_beacon = false;

	return beacon;
}

bool CDMRNetwork::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	// if (m_debug)
	//	CUtils::dump(1U, "Network Transmitted", data, length);

	bool ret = m_socket.write(data, length, m_address, m_port);
	if (!ret) {
		LogError("DMR, Socket has failed when writing data to the master, retrying connection");
		m_socket.close();
		open();
		return false;
	}

	return true;
}
