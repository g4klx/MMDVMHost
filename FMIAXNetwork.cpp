/*
 *   Copyright (C) 2020,2021,2023,2024 by Jonathan Naylor G4KLX
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

#include "FMIAXNetwork.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

const unsigned char IAX_PROTO_VERSION   = 2U;

const unsigned char AST_FRAME_DTMF      = 1U;
const unsigned char AST_FRAME_VOICE     = 2U;
const unsigned char AST_FRAME_CONTROL   = 4U;
const unsigned char AST_FRAME_IAX       = 6U;
const unsigned char AST_FRAME_TEXT      = 7U;

const unsigned char AST_CONTROL_HANGUP  = 1U;
const unsigned char AST_CONTROL_RING    = 2U;
const unsigned char AST_CONTROL_RINGING = 3U;
const unsigned char AST_CONTROL_ANSWER  = 4U;
const unsigned char AST_CONTROL_OPTION  = 11U;
const unsigned char AST_CONTROL_KEY     = 12U;
const unsigned char AST_CONTROL_UNKEY   = 13U;

const unsigned char AST_FORMAT_ULAW     = 4U;

const unsigned char IAX_AUTH_MD5        = 2U;

const unsigned char IAX_COMMAND_NEW     = 1U;
const unsigned char IAX_COMMAND_PING    = 2U;
const unsigned char IAX_COMMAND_PONG    = 3U;
const unsigned char IAX_COMMAND_ACK     = 4U;
const unsigned char IAX_COMMAND_HANGUP  = 5U;
const unsigned char IAX_COMMAND_REJECT  = 6U;
const unsigned char IAX_COMMAND_ACCEPT  = 7U;
const unsigned char IAX_COMMAND_AUTHREQ = 8U;
const unsigned char IAX_COMMAND_AUTHREP = 9U;
const unsigned char IAX_COMMAND_INVAL   = 10U;
const unsigned char IAX_COMMAND_LAGRQ   = 11U;
const unsigned char IAX_COMMAND_LAGRP   = 12U;
const unsigned char IAX_COMMAND_REGREQ  = 13U;
const unsigned char IAX_COMMAND_REGAUTH = 14U;
const unsigned char IAX_COMMAND_REGACK  = 15U;
const unsigned char IAX_COMMAND_REGREJ  = 16U;
const unsigned char IAX_COMMAND_VNAK    = 18U;

const unsigned char IAX_IE_RR_JITTER    = 46U;
const unsigned char IAX_IE_RR_LOSS      = 47U;
const unsigned char IAX_IE_RR_PKTS      = 48U;
const unsigned char IAX_IE_RR_DELAY     = 49U;
const unsigned char IAX_IE_RR_DROPPED   = 50U;
const unsigned char IAX_IE_RR_OOO       = 51U;

const unsigned int BUFFER_LENGTH = 1500U;

CFMIAXNetwork::CFMIAXNetwork(const std::string& callsign, const std::string& localAddress, unsigned short localPort, const std::string& gatewayAddress, unsigned short gatewayPort, bool debug) :
m_callsign(callsign),
m_socket(localAddress, localPort),
m_addr(),
m_addrLen(0U),
m_debug(debug),
m_enabled(false),
m_buffer(2000U, "FM Network"),
m_timestamp(),
m_sCallNo(0U),
m_dCallNo(0U),
m_iSeqNo(0U),
m_oSeqNo(0U),
m_rxJitter(0U),
m_rxLoss(0U),
m_rxFrames(0U),
m_rxDelay(0U),
m_rxDropped(0U),
m_rxOOO(0U)
{
	assert(!callsign.empty());
	assert(gatewayPort > 0U);
	assert(!gatewayAddress.empty());

	if (CUDPSocket::lookup(gatewayAddress, gatewayPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;

	// Remove any trailing letters in the callsign
	size_t pos = callsign.find_first_of(' ');
	if (pos != std::string::npos)
		m_callsign = callsign.substr(0U, pos);
}

CFMIAXNetwork::~CFMIAXNetwork()
{
}

bool CFMIAXNetwork::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the FM Gateway");
		return false;
	}

	LogMessage("Opening FM IAX network connection");

	return m_socket.open(m_addr);
}

bool CFMIAXNetwork::writeData(const float* data, unsigned int nSamples)
{
	assert(data != NULL);
	assert(nSamples > 0U);

	if (m_seqNo == 0U) {
		bool ret = writeStart();
		if (!ret)
			return false;
	}

	unsigned char buffer[500U];
	::memset(buffer, 0x00U, 500U);

	unsigned int length = 0U;

	buffer[length++] = 'U';
	buffer[length++] = 'S';
	buffer[length++] = 'R';
	buffer[length++] = 'P';

	// Sequence number
	buffer[length++] = (m_seqNo >> 24) & 0xFFU;
	buffer[length++] = (m_seqNo >> 16) & 0xFFU;
	buffer[length++] = (m_seqNo >> 8)  & 0xFFU;
	buffer[length++] = (m_seqNo >> 0)  & 0xFFU;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// PTT on
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x01U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// Type, 0 for audio
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	for (unsigned int i = 0U; i < nSamples; i++) {
		short val = short(data[i] * 32767.0F + 0.5F);			// Changing audio format from float to S16LE

		buffer[length++] = (val >> 0) & 0xFFU;
		buffer[length++] = (val >> 8) & 0xFFU;
	}

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

	m_seqNo++;

	return m_socket.write(buffer, length, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeEnd()
{
	unsigned char buffer[500U];
	::memset(buffer, 0x00U, 500U);

	unsigned int length = 0U;

	buffer[length++] = 'U';
	buffer[length++] = 'S';
	buffer[length++] = 'R';
	buffer[length++] = 'P';

	// Sequence number
	buffer[length++] = (m_seqNo >> 24) & 0xFFU;
	buffer[length++] = (m_seqNo >> 16) & 0xFFU;
	buffer[length++] = (m_seqNo >> 8) & 0xFFU;
	buffer[length++] = (m_seqNo >> 0) & 0xFFU;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// PTT off
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// Type, 0 for audio
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	length += 320U;

	m_seqNo = 0U;

	if (length > 0U) {
		if (m_debug)
			CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

		return m_socket.write(buffer, length, m_addr, m_addrLen);
	} else {
		return true;
	}
}

void CFMIAXNetwork::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage addr;
	unsigned int addrlen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, addr, addrlen);
	if (length <= 0)
		return;

	// Check if the data is for us
	if (!CUDPSocket::match(addr, m_addr, IMT_ADDRESS_AND_PORT)) {
		LogMessage("FM IAX packet received from an invalid source");
		return;
	}

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);

	// Invalid packet type?
	if (::memcmp(buffer, "USRP", 4U) != 0)
		return;

	if (length < 32)
		return;

	// The type is a big-endian 4-byte integer
	unsigned int type = (buffer[20U] << 24) +
			    (buffer[21U] << 16) +
			    (buffer[22U] << 8)  +
			    (buffer[23U] << 0);

	if (type == 0U)
		m_buffer.addData(buffer + 32U, length - 32U);
}

unsigned int CFMIAXNetwork::readData(float* out, unsigned int nOut)
{
	assert(out != NULL);
	assert(nOut > 0U);

	unsigned int bytes = m_buffer.dataSize() / sizeof(unsigned short);
	if (bytes == 0U)
		return 0U;

	if (bytes < nOut)
		nOut = bytes;

	unsigned char buffer[1500U];
	m_buffer.getData(buffer, nOut * sizeof(unsigned short));

	for (unsigned int i = 0U; i < nOut; i++) {
		short val = ((buffer[i * 2U + 0U] & 0xFFU) << 0) + ((buffer[i * 2U + 1U] & 0xFFU) << 8);
		out[i] = float(val) / 65536.0F;
	}

	return nOut;
}

void CFMIAXNetwork::reset()
{
	m_buffer.clear();
}

void CFMIAXNetwork::close()
{
	m_socket.close();

	LogMessage("Closing FM IAX network connection");
}

void CFMIAXNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();
	else if (!enabled && m_enabled)
		reset();

	m_enabled = enabled;
}

bool CFMIAXNetwork::writeStart()
{
	unsigned char buffer[500U];
	::memset(buffer, 0x00U, 500U);

	unsigned int length = 0U;

	buffer[length++] = 'U';
	buffer[length++] = 'S';
	buffer[length++] = 'R';
	buffer[length++] = 'P';

	// Sequence number
	buffer[length++] = (m_seqNo >> 24) & 0xFFU;
	buffer[length++] = (m_seqNo >> 16) & 0xFFU;
	buffer[length++] = (m_seqNo >> 8) & 0xFFU;
	buffer[length++] = (m_seqNo >> 0) & 0xFFU;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// PTT off
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// Type, 2 for metadata
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x02U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// TLV TAG for Metadata
	buffer[length++] = 0x08U;

	// TLV Length
	buffer[length++] = 3U + 4U + 3U + 1U + 1U + m_callsign.size() + 1U;

	// DMR Id
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// Rpt Id
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// Talk Group
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	// Time Slot
	buffer[length++] = 0x00U;

	// Color Code
	buffer[length++] = 0x00U;

	// Callsign
	for (std::string::const_iterator it = m_callsign.cbegin(); it != m_callsign.cend(); ++it)
		buffer[length++] = *it;

	// End of Metadata
	buffer[length++] = 0x00U;

	length = 70U;

	if (length > 0U) {
		if (m_debug)
			CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

		return m_socket.write(buffer, length, m_addr, m_addrLen);
	} else {
		return true;
	}
}

bool CFMIAXNetwork::writePing()
{
	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[15U];

	buffer[0U] = (sCall << 8) & 0xFFU;
	buffer[1U] = (sCall << 0) & 0xFFU;

	buffer[2U] = (m_dCallNo << 8) & 0xFFU;
	buffer[3U] = (m_dCallNo << 0) & 0xFFU;

	buffer[4U] = (ts << 24) & 0xFFU;
	buffer[5U] = (ts << 16) & 0xFFU;
	buffer[6U] = (ts << 8)  & 0xFFU;
	buffer[7U] = (ts << 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_PING;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writePong()
{
	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[50U];

	buffer[0U] = (sCall << 8) & 0xFFU;
	buffer[1U] = (sCall << 0) & 0xFFU;

	buffer[2U] = (m_dCallNo << 8) & 0xFFU;
	buffer[3U] = (m_dCallNo << 0) & 0xFFU;

	buffer[4U] = (ts << 24) & 0xFFU;
	buffer[5U] = (ts << 16) & 0xFFU;
	buffer[6U] = (ts << 8)  & 0xFFU;
	buffer[7U] = (ts << 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_PONG;

	buffer[12U] = IAX_IE_RR_JITTER;
	buffer[13U] = sizeof(unsigned int);
	buffer[14U] = (m_rxJitter << 24) & 0xFFU;
	buffer[15U] = (m_rxJitter << 16) & 0xFFU;
	buffer[16U] = (m_rxJitter << 8)  & 0xFFU;
	buffer[17U] = (m_rxJitter << 0)  & 0xFFU;

	buffer[18U] = IAX_IE_RR_LOSS;
	buffer[19U] = sizeof(unsigned int);
	buffer[20U] = (m_rxLoss << 24) & 0xFFU;
	buffer[21U] = (m_rxLoss << 16) & 0xFFU;
	buffer[22U] = (m_rxLoss << 8)  & 0xFFU;
	buffer[23U] = (m_rxLoss << 0)  & 0xFFU;

	buffer[24U] = IAX_IE_RR_PKTS;
	buffer[25U] = sizeof(unsigned int);
	buffer[26U] = (m_rxFrames << 24) & 0xFFU;
	buffer[27U] = (m_rxFrames << 16) & 0xFFU;
	buffer[28U] = (m_rxFrames << 8)  & 0xFFU;
	buffer[29U] = (m_rxFrames << 0)  & 0xFFU;

	buffer[30U] = IAX_IE_RR_DELAY;
	buffer[31U] = sizeof(unsigned short);
	buffer[32U] = (m_rxDelay << 8)  & 0xFFU;
	buffer[33U] = (m_rxDelay << 0)  & 0xFFU;

	buffer[34U] = IAX_IE_RR_DROPPED;
	buffer[35U] = sizeof(unsigned int);
	buffer[36U] = (m_rxDropped << 24) & 0xFFU;
	buffer[37U] = (m_rxDropped << 16) & 0xFFU;
	buffer[38U] = (m_rxDropped << 8)  & 0xFFU;
	buffer[39U] = (m_rxDropped << 0)  & 0xFFU;

	buffer[40U] = IAX_IE_RR_OOO;
	buffer[41U] = sizeof(unsigned int);
	buffer[42U] = (m_rxOOO << 24) & 0xFFU;
	buffer[43U] = (m_rxOOO << 16) & 0xFFU;
	buffer[44U] = (m_rxOOO << 8)  & 0xFFU;
	buffer[45U] = (m_rxOOO << 0)  & 0xFFU;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 46U);

	return m_socket.write(buffer, 46U, m_addr, m_addrLen);
}

