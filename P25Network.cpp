/*
 *   Copyright (C) 2009-2014,2016,2019,2020,2021,2025 by Jonathan Naylor G4KLX
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

#include "P25Network.h"
#include "P25Defines.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char REC62[] = {
	0x62U, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x12U, 0x64U, 0x00U, 0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC63[] = {
	0x63U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC64[] = {
	0x64U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC65[] = {
	0x65U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC66[] = {
	0x66U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC67[] = {
	0x67U, 0xF0U, 0x9DU, 0x6AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC68[] = {
	0x68U, 0x19U, 0xD4U, 0x26U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC69[] = {
	0x69U, 0xE0U, 0xEBU, 0x7BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6A[] = {
	0x6AU, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC6B[] = {
	0x6BU, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x12U, 0x64U, 0x00U, 0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC6C[] = {
	0x6CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6D[] = {
	0x6DU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6E[] = {
	0x6EU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6F[] = {
	0x6FU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC70[] = {
	0x70U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC71[] = {
	0x71U, 0xACU, 0xB8U, 0xA4U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC72[] = {
	0x72U, 0x9BU, 0xDCU, 0x75U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC73[] = {
	0x73U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC80[] = {
	0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned int BUFFER_LENGTH = 100U;

CP25Network::CP25Network(const std::string& gatewayAddress, unsigned short gatewayPort, const std::string& localAddress, unsigned short localPort, bool debug) :
m_socket(localAddress, localPort),
m_addr(),
m_addrLen(0U),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "P25 Network"),
m_audio()
{
	if (CUDPSocket::lookup(gatewayAddress, gatewayPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;
}

CP25Network::~CP25Network()
{
}

bool CP25Network::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the P25 Gateway");
		return false;
	}

	LogMessage("Opening P25 network connection");

	return m_socket.open(m_addr);
}

bool CP25Network::writeLDU1(const unsigned char* ldu1, const CP25Data& control, const CP25LowSpeedData& lsd, bool end)
{
	assert(ldu1 != nullptr);

	unsigned char buffer[22U];

	// The '62' record
	::memcpy(buffer, REC62, 22U);
	m_audio.decode(ldu1, buffer + 10U, 0U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 22U);

	bool ret = m_socket.write(buffer, 22U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '63' record
	::memcpy(buffer, REC63, 14U);
	m_audio.decode(ldu1, buffer + 1U, 1U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 14U);

	ret = m_socket.write(buffer, 14U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '64' record
	::memcpy(buffer, REC64, 17U);
	buffer[1U] = control.getLCF();
	buffer[2U] = control.getMFId();
	m_audio.decode(ldu1, buffer + 5U, 2U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '65' record
	::memcpy(buffer, REC65, 17U);
	unsigned int id = control.getDstId();
	buffer[1U] = (id >> 16) & 0xFFU;
	buffer[2U] = (id >> 8) & 0xFFU;
	buffer[3U] = (id >> 0) & 0xFFU;
	m_audio.decode(ldu1, buffer + 5U, 3U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '66' record
	::memcpy(buffer, REC66, 17U);
	id = control.getSrcId();
	buffer[1U] = (id >> 16) & 0xFFU;
	buffer[2U] = (id >> 8) & 0xFFU;
	buffer[3U] = (id >> 0) & 0xFFU;
	m_audio.decode(ldu1, buffer + 5U, 4U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '67' record
	::memcpy(buffer, REC67, 17U);
	m_audio.decode(ldu1, buffer + 5U, 5U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '68' record
	::memcpy(buffer, REC68, 17U);
	m_audio.decode(ldu1, buffer + 5U, 6U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '69' record
	::memcpy(buffer, REC69, 17U);
	m_audio.decode(ldu1, buffer + 5U, 7U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '6A' record
	::memcpy(buffer, REC6A, 16U);
	buffer[1U] = lsd.getLSD1();
	buffer[2U] = lsd.getLSD2();
	m_audio.decode(ldu1, buffer + 4U, 8U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 16U);

	ret = m_socket.write(buffer, 16U, m_addr, m_addrLen);
	if (!ret)
		return false;

	if (end) {
		if (m_debug)
			CUtils::dump(1U, "P25 Network END Sent", REC80, 17U);

		ret = m_socket.write(REC80, 17U, m_addr, m_addrLen);
		if (!ret)
			return false;
	}

	return true;
}

bool CP25Network::writeLDU2(const unsigned char* ldu2, const CP25Data& control, const CP25LowSpeedData& lsd, bool end)
{
	assert(ldu2 != nullptr);

	unsigned char buffer[22U];

	// The '6B' record
	::memcpy(buffer, REC6B, 22U);
	m_audio.decode(ldu2, buffer + 10U, 0U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 22U);

	bool ret = m_socket.write(buffer, 22U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '6C' record
	::memcpy(buffer, REC6C, 14U);
	m_audio.decode(ldu2, buffer + 1U, 1U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 14U);

	ret = m_socket.write(buffer, 14U, m_addr, m_addrLen);
	if (!ret)
		return false;

	unsigned char mi[P25_MI_LENGTH_BYTES];
	control.getMI(mi);

	// The '6D' record
	::memcpy(buffer, REC6D, 17U);
	buffer[1U] = mi[0U];
	buffer[2U] = mi[1U];
	buffer[3U] = mi[2U];
	m_audio.decode(ldu2, buffer + 5U, 2U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '6E' record
	::memcpy(buffer, REC6E, 17U);
	buffer[1U] = mi[3U];
	buffer[2U] = mi[4U];
	buffer[3U] = mi[5U];
	m_audio.decode(ldu2, buffer + 5U, 3U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '6F' record
	::memcpy(buffer, REC6F, 17U);
	buffer[1U] = mi[6U];
	buffer[2U] = mi[7U];
	buffer[3U] = mi[8U];
	m_audio.decode(ldu2, buffer + 5U, 4U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '70' record
	::memcpy(buffer, REC70, 17U);
	buffer[1U] = control.getAlgId();
	unsigned int id = control.getKId();
	buffer[2U] = (id >> 8) & 0xFFU;
	buffer[3U] = (id >> 0) & 0xFFU;
	m_audio.decode(ldu2, buffer + 5U, 5U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '71' record
	::memcpy(buffer, REC71, 17U);
	m_audio.decode(ldu2, buffer + 5U, 6U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '72' record
	::memcpy(buffer, REC72, 17U);
	m_audio.decode(ldu2, buffer + 5U, 7U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_addr, m_addrLen);
	if (!ret)
		return false;

	// The '73' record
	::memcpy(buffer, REC73, 16U);
	buffer[1U] = lsd.getLSD1();
	buffer[2U] = lsd.getLSD2();
	m_audio.decode(ldu2, buffer + 4U, 8U);

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 16U);

	ret = m_socket.write(buffer, 16U, m_addr, m_addrLen);
	if (!ret)
		return false;

	if (end) {
		if (m_debug)
			CUtils::dump(1U, "P25 Network END Sent", REC80, 17U);

		ret = m_socket.write(REC80, 17U, m_addr, m_addrLen);
		if (!ret)
			return false;
	}

	return true;
}

void CP25Network::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage address;
	unsigned int addrLen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, addrLen);
	if (length <= 0)
		return;

	if (!CUDPSocket::match(m_addr, address)) {
		LogMessage("P25, packet received from an invalid source");
		return;
	}

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Received", buffer, length);

	unsigned char c = length;
	m_buffer.addData(&c, 1U);

	m_buffer.addData(buffer, length);
}

unsigned int CP25Network::read(unsigned char* data, unsigned int length)
{
	assert(data != nullptr);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char c = 0U;
	m_buffer.getData(&c, 1U);

	assert(c <= length);

	m_buffer.getData(data, c);

	return c;
}

bool CP25Network::isConnected() const
{
	return (m_addrLen != 0);
}

void CP25Network::close()
{
	m_socket.close();

	LogMessage("Closing P25 network connection");
}

void CP25Network::enable(bool enabled)
{
	if (!enabled && m_enabled)
		m_buffer.clear();

	m_enabled = enabled;
}
