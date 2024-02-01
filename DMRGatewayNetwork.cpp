/*
 *   Copyright (C) 2015-2021 by Jonathan Naylor G4KLX
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

#include "DMRGatewayNetwork.h"

#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>

const unsigned int BUFFER_LENGTH = 500U;

const unsigned int HOMEBREW_DATA_PACKET_LENGTH = 55U;


CDMRGatewayNetwork::CDMRGatewayNetwork(const std::string& address, unsigned short port, const std::string& localAddress, unsigned short localPort, unsigned int id, bool duplex, const char* version, bool slot1, bool slot2, HW_TYPE hwType, bool debug) :
m_addressStr(address),
m_addr(),
m_addrLen(0U),
m_port(port),
m_id(NULL),
m_duplex(duplex),
m_version(version),
m_debug(debug),
m_socket(localAddress, localPort),
m_enabled(false),
m_slot1(slot1),
m_slot2(slot2),
m_hwType(hwType),
m_buffer(NULL),
m_streamId(NULL),
m_rxData(1000U, "DMR Network"),
m_beacon(false),
m_random(),
m_callsign(),
m_rxFrequency(0U),
m_txFrequency(0U),
m_power(0U),
m_colorCode(0U),
m_pingTimer(1000U, 10U)
{
	assert(!address.empty());
	assert(port > 0U);
	assert(id > 1000U);

	if (CUDPSocket::lookup(m_addressStr, m_port, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;

	m_buffer   = new unsigned char[BUFFER_LENGTH];
	m_id       = new uint8_t[4U];
	m_streamId = new uint32_t[2U];

	m_id[0U] = id >> 24;
	m_id[1U] = id >> 16;
	m_id[2U] = id >> 8;
	m_id[3U] = id >> 0;

	std::random_device rd;
	std::mt19937 mt(rd());
	m_random = mt;

	std::uniform_int_distribution<uint32_t> dist(0x00000001, 0xfffffffe);
	m_streamId[0U] = dist(m_random);
	m_streamId[1U] = dist(m_random);
}

CDMRGatewayNetwork::~CDMRGatewayNetwork()
{
	delete[] m_buffer;
	delete[] m_streamId;
	delete[] m_id;
}

void CDMRGatewayNetwork::setConfig(const std::string & callsign, unsigned int rxFrequency, unsigned int txFrequency, unsigned int power, unsigned int colorCode, float latitude, float longitude, int height, const std::string& location, const std::string& description, const std::string& url)
{
	m_callsign    = callsign;
	m_rxFrequency = rxFrequency;
	m_txFrequency = txFrequency;
	m_power       = power;
	m_colorCode   = colorCode;
}

void CDMRGatewayNetwork::setOptions(const std::string& options)
{
}

bool CDMRGatewayNetwork::open()
{
	if (m_addrLen == 0U) {
		LogError("DMR, Unable to resolve the address of the DMR Network");
		return false;
	}

	LogMessage("DMR, Opening DMR Network");

	bool ret = m_socket.open(m_addr);
	if (ret)
		m_pingTimer.start();

	return ret;
}

void CDMRGatewayNetwork::enable(bool enabled)
{
	if (!enabled && m_enabled)
		m_rxData.clear();

	m_enabled = enabled;
}

bool CDMRGatewayNetwork::read(CDMRData& data)
{
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

bool CDMRGatewayNetwork::write(const CDMRData& data)
{
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

	std::uniform_int_distribution<uint32_t> dist(0x00000001, 0xfffffffe);
	unsigned char dataType = data.getDataType();
	if (dataType == DT_VOICE_SYNC) {
		buffer[15U] |= 0x10U;
	} else if (dataType == DT_VOICE) {
		buffer[15U] |= data.getN();
	} else {
		if (dataType == DT_VOICE_LC_HEADER)
			m_streamId[slotIndex] = dist(m_random);

		if (dataType == DT_CSBK || dataType == DT_DATA_HEADER)
			m_streamId[slotIndex] = dist(m_random);

		buffer[15U] |= (0x20U | dataType);
	}

	buffer[4U] = data.getSeqNo();

	::memcpy(buffer + 16U, m_streamId + slotIndex, 4U);

	data.getData(buffer + 20U);

	buffer[53U] = data.getBER();

	buffer[54U] = data.getRSSI();

	write(buffer, HOMEBREW_DATA_PACKET_LENGTH);

	return true;
}

bool CDMRGatewayNetwork::writeRadioPosition(unsigned int id, const unsigned char* data)
{
	unsigned char buffer[20U];

	::memcpy(buffer + 0U, "DMRG", 4U);

	buffer[4U] = id >> 16;
	buffer[5U] = id >> 8;
	buffer[6U] = id >> 0;

	::memcpy(buffer + 7U, data + 2U, 7U);

	return write(buffer, 14U);
}

bool CDMRGatewayNetwork::writeTalkerAlias(unsigned int id, unsigned char type, const unsigned char* data)
{
	unsigned char buffer[20U];

	::memcpy(buffer + 0U, "DMRA", 4U);

	buffer[4U] = id >> 16;
	buffer[5U] = id >> 8;
	buffer[6U] = id >> 0;

	buffer[7U] = type;

	::memcpy(buffer + 8U, data + 2U, 7U);

	return write(buffer, 15U);
}

bool CDMRGatewayNetwork::isConnected() const
{
	return (m_addrLen != 0);
}

void CDMRGatewayNetwork::close(bool sayGoodbye)
{
	LogMessage("DMR, Closing DMR Network");

	m_socket.close();
}

void CDMRGatewayNetwork::clock(unsigned int ms)
{
	m_pingTimer.clock(ms);
	if (m_pingTimer.isRunning() && m_pingTimer.hasExpired()) {
		writeConfig();
		m_pingTimer.start();
	}

	sockaddr_storage address;
	unsigned int addrLen;
	int length = m_socket.read(m_buffer, BUFFER_LENGTH, address, addrLen);
	if (length <= 0)
		return;

	if (!CUDPSocket::match(m_addr, address)) {
		LogMessage("DMR, packet received from an invalid source");
		return;
	}

	if (m_debug)
		CUtils::dump(1U, "DMR Network Received", m_buffer, length);

	if (::memcmp(m_buffer, "DMRD", 4U) == 0) {
		if (m_enabled) {
			unsigned char len = length;
			m_rxData.addData(&len, 1U);
			m_rxData.addData(m_buffer, len);
		}
	} else if (::memcmp(m_buffer, "DMRP", 4U) == 0) {
		;
	} else if (::memcmp(m_buffer, "DMRB", 4U) == 0) {
		m_beacon = true;
	} else {
		CUtils::dump("DMR, unknown packet from the DMR Network", m_buffer, length);
	}
}

bool CDMRGatewayNetwork::writeConfig()
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
		case HWT_D2RG_MMDVM_HS:
			software = "MMDVM_D2RG_MMDVM_HS";
			break;
		case HWT_MMDVM_HS:
			software = "MMDVM_MMDVM_HS";
			break;
		case HWT_OPENGD77_HS:
			software = "MMDVM_OpenGD77_HS";
			break;
		case HWT_SKYBRIDGE:
			software = "MMDVM_SkyBridge";
			break;
		default:
			software = "MMDVM_Unknown";
			break;
		}
	}

	unsigned int power = m_power;
	if (power > 99U)
		power = 99U;

	char buffer[150U];

	::memcpy(buffer + 0U, "DMRC", 4U);
	::memcpy(buffer + 4U, m_id, 4U);
	::sprintf(buffer + 8U, "%-8.8s%09u%09u%02u%02u%c%-40.40s%-40.40s",
		m_callsign.c_str(), m_rxFrequency, m_txFrequency, power, m_colorCode, slots, m_version,
		software);

	return write((unsigned char*)buffer, 119U);
}

bool CDMRGatewayNetwork::wantsBeacon()
{
	bool beacon = m_beacon;

	m_beacon = false;

	return beacon;
}

bool CDMRGatewayNetwork::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_debug)
		CUtils::dump(1U, "DMR Network Transmitted", data, length);

	bool ret = m_socket.write(data, length, m_addr, m_addrLen);
	if (!ret) {
		LogError("DMR, socket error when writing to the DMR Network");
		return false;
	}

	return true;
}
