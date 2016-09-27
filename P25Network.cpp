/*
 *   Copyright (C) 2009-2014,2016 by Jonathan Naylor G4KLX
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

const unsigned char STARTICW[] = {
	0x00U, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char VHDR1[] = {
	0x60U, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x36U};

const unsigned char ENDICW[] = {
	0x00U, 0x02U, 0x02U, 0x25U, 0x0BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned int BUFFER_LENGTH = 100U;

CP25Network::CP25Network(const std::string& gatewayAddress, unsigned int gatewayPort, unsigned int localPort, bool debug) :
m_socket(localPort),
m_address(),
m_port(gatewayPort),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "P25 Network"),
m_audio()
{
	m_address = CUDPSocket::lookup(gatewayAddress);
}

CP25Network::~CP25Network()
{
}

bool CP25Network::open()
{
	LogMessage("Opening P25 network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	return m_socket.open();
}

bool CP25Network::writeHeader(const CP25Data& control)
{
	if (m_debug)
		CUtils::dump(1U, "P25 Network ICW Sent", STARTICW, 10U);

	bool ret = m_socket.write(STARTICW, 10U, m_address, m_port);
	if (!ret)
		return false;

	if (m_debug)
		CUtils::dump(1U, "P25 Network VHDR1 Sent", VHDR1, 30U);

	ret = m_socket.write(VHDR1, 30U, m_address, m_port);
	if (!ret)
		return false;

	unsigned int tgid = 0x0000U;

	if (control.getLCF() == P25_LCF_GROUP)
		tgid = control.getDstId();

	// The VHDR2 record
	unsigned char buffer[22U];
	::memset(buffer, 0x00U, 22U);
	buffer[0U] = 0x61U;
	buffer[1U] = (tgid << 8) & 0xFFU;
	buffer[2U] = (tgid << 0) & 0xFFU;
	buffer[21U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network VHDR2 Sent", buffer, 22U);

	ret = m_socket.write(buffer, 22U, m_address, m_port);
	if (!ret)
		return false;

	return true;
}

bool CP25Network::writeLDU1(const unsigned char* ldu1, const CP25Data& control, const CP25LowSpeedData& lsd)
{
	assert(ldu1 != NULL);

	unsigned char buffer[22U];

	// The '62' record
	::memset(buffer, 0x00U, 22U);
	::memcpy(buffer, STARTICW, 10U);
	buffer[0U] = 0x62U;
	m_audio.decode(ldu1, buffer + 10U, 0U);
	buffer[21U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 22U);

	bool ret = m_socket.write(buffer, 22U, m_address, m_port);
	if (!ret)
		return false;

	// The '63' record
	::memset(buffer, 0x00U, 14U);
	buffer[0U] = 0x63U;
	m_audio.decode(ldu1, buffer + 1U, 1U);
	buffer[13U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 14U);

	ret = m_socket.write(buffer, 14U, m_address, m_port);
	if (!ret)
		return false;

	// The '64' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x64U;
	buffer[1U] = control.getLCF();
	buffer[2U] = control.getMFId();
	m_audio.decode(ldu1, buffer + 5U, 2U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '65' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x65U;
	unsigned int id = control.getDstId();
	buffer[1U] = (id >> 16) & 0xFFU;
	buffer[2U] = (id >> 8) & 0xFFU;
	buffer[3U] = (id >> 0) & 0xFFU;
	m_audio.decode(ldu1, buffer + 5U, 3U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '66' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x66U;
	id = control.getSrcId();
	buffer[1U] = (id >> 16) & 0xFFU;
	buffer[2U] = (id >> 8) & 0xFFU;
	buffer[3U] = (id >> 0) & 0xFFU;
	m_audio.decode(ldu1, buffer + 5U, 4U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '67' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x67U;
	m_audio.decode(ldu1, buffer + 5U, 5U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '68' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x68U;
	m_audio.decode(ldu1, buffer + 5U, 6U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '69' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x69U;
	m_audio.decode(ldu1, buffer + 5U, 7U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '6A' record
	::memset(buffer, 0x00U, 16U);
	buffer[0U] = 0x6AU;
	buffer[1U] = lsd.getLSD1();
	buffer[2U] = lsd.getLSD2();
	m_audio.decode(ldu1, buffer + 4U, 8U);
	buffer[15U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 16U);

	ret = m_socket.write(buffer, 16U, m_address, m_port);
	if (!ret)
		return false;

	return true;
}

bool CP25Network::writeLDU2(const unsigned char* ldu2, const CP25Data& control, const CP25LowSpeedData& lsd)
{
	assert(ldu2 != NULL);

	unsigned char buffer[22U];

	// The '6B' record
	::memset(buffer, 0x00U, 22U);
	::memcpy(buffer, STARTICW, 10U);
	buffer[0U] = 0x6BU;
	m_audio.decode(ldu2, buffer + 10U, 0U);
	buffer[21U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 22U);

	bool ret = m_socket.write(buffer, 22U, m_address, m_port);
	if (!ret)
		return false;

	// The '6C' record
	::memset(buffer, 0x00U, 14U);
	buffer[0U] = 0x6CU;
	m_audio.decode(ldu2, buffer + 1U, 1U);
	buffer[13U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 14U);

	ret = m_socket.write(buffer, 14U, m_address, m_port);
	if (!ret)
		return false;

	unsigned char mi[P25_MI_LENGTH_BYTES];
	control.getMI(mi);

	// The '6D' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x6DU;
	buffer[1U] = mi[0U];
	buffer[2U] = mi[1U];
	buffer[3U] = mi[2U];
	m_audio.decode(ldu2, buffer + 5U, 2U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '6E' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x6EU;
	buffer[1U] = mi[3U];
	buffer[2U] = mi[4U];
	buffer[3U] = mi[5U];
	m_audio.decode(ldu2, buffer + 5U, 3U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '6F' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x6FU;
	buffer[1U] = mi[6U];
	buffer[2U] = mi[7U];
	buffer[3U] = mi[8U];
	m_audio.decode(ldu2, buffer + 5U, 4U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '70' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x70U;
	buffer[1U] = control.getAlgId();
	unsigned int id = control.getKId();
	buffer[2U] = (id >> 8) & 0xFFU;
	buffer[3U] = (id >> 0) & 0xFFU;
	m_audio.decode(ldu2, buffer + 5U, 5U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '71' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x71U;
	m_audio.decode(ldu2, buffer + 5U, 6U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '72' record
	::memset(buffer, 0x00U, 17U);
	buffer[0U] = 0x72U;
	m_audio.decode(ldu2, buffer + 5U, 7U);
	buffer[16U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 17U);

	ret = m_socket.write(buffer, 17U, m_address, m_port);
	if (!ret)
		return false;

	// The '73' record
	::memset(buffer, 0x00U, 16U);
	buffer[0U] = 0x73U;
	buffer[1U] = lsd.getLSD1();
	buffer[2U] = lsd.getLSD2();
	m_audio.decode(ldu2, buffer + 4U, 8U);
	buffer[15U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 16U);

	ret = m_socket.write(buffer, 16U, m_address, m_port);
	if (!ret)
		return false;

	return true;
}

bool CP25Network::writeTerminator()
{
	if (m_debug)
		CUtils::dump(1U, "P25 Network ICW Sent", ENDICW, 10U);

	bool ret = m_socket.write(ENDICW, 10U, m_address, m_port);
	if (!ret)
		return false;

	ret = m_socket.write(ENDICW, 10U, m_address, m_port);
	if (!ret)
		return false;

	return true;
}

void CP25Network::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr || m_port != port) {
		LogMessage("P25 packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return;
	}

	if (!m_enabled)
		return;

	unsigned char c = length;
	m_buffer.addData(&c, 1U);

	m_buffer.addData(buffer, length);
}

unsigned int CP25Network::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char c = 0U;
	m_buffer.getData(&c, 1U);

	assert(c <= length);

	m_buffer.getData(data, c);

	return c;
}

void CP25Network::close()
{
	m_socket.close();

	LogMessage("Closing P25 network connection");
}

void CP25Network::enable(bool enabled)
{
	m_enabled = enabled;
}
