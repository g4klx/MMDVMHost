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

#include <md5.h>

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

const unsigned char IAX_IE_CALLED_NUMBER  = 1U;
const unsigned char IAX_IE_CALLING_NUMBER = 2U;
const unsigned char IAX_IE_CALLING_NAME   = 4U;
const unsigned char IAX_IE_CALLED_CONTEXT = 5U;
const unsigned char IAX_IE_USERNAME       = 6U;
const unsigned char IAX_IE_PASSWORD       = 7U;
const unsigned char IAX_IE_CAPABILITY     = 8U;
const unsigned char IAX_IE_FORMAT         = 9U;
const unsigned char IAX_IE_VERSION        = 11U;
const unsigned char IAX_IE_DNID           = 13U;
const unsigned char IAX_IE_AUTHMETHODS    = 14U;
const unsigned char IAX_IE_CHALLENGE      = 15U;
const unsigned char IAX_IE_MD5_RESULT     = 16U;
const unsigned char IAX_IE_APPARENT_ADDR  = 18U;
const unsigned char IAX_IE_REFRESH        = 19U;
const unsigned char IAX_IE_CAUSE          = 22U;
const unsigned char IAX_IE_DATETIME       = 31U;

const unsigned char IAX_IE_RR_JITTER    = 46U;
const unsigned char IAX_IE_RR_LOSS      = 47U;
const unsigned char IAX_IE_RR_PKTS      = 48U;
const unsigned char IAX_IE_RR_DELAY     = 49U;
const unsigned char IAX_IE_RR_DROPPED   = 50U;
const unsigned char IAX_IE_RR_OOO       = 51U;

const unsigned int BUFFER_LENGTH = 1500U;

CFMIAXNetwork::CFMIAXNetwork(const std::string& callsign, const std::string& username, const std::string& password, const std::string& node, const std::string& localAddress, unsigned short localPort, const std::string& gatewayAddress, unsigned short gatewayPort, bool debug) :
m_callsign(callsign),
m_username(username),
m_password(password),
m_node(node),
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
	assert(!username.empty());
	assert(!password.empty());
	assert(!node.empty());
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

bool CFMIAXNetwork::writeStart()
{
	return true;
}

bool CFMIAXNetwork::writeData(const float* data, unsigned int nSamples)
{
	assert(data != NULL);
	assert(nSamples > 0U);

	return true;
}

bool CFMIAXNetwork::writeEnd()
{
	return true;
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

bool CFMIAXNetwork::writeCall()
{
	unsigned short sCall = ++m_sCallNo | 0x8000U;

	m_timestamp.start();

	m_oSeqNo = m_iSeqNo = 0U;

	unsigned int length = 0U;

	unsigned char buffer[100U];

	buffer[length++] = (sCall << 8) & 0xFFU;
	buffer[length++] = (sCall << 0) & 0xFFU;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;

	buffer[length++] = 0U;

	buffer[length++] = 0U;

	buffer[length++] = AST_FRAME_IAX;

	buffer[length++] = IAX_COMMAND_NEW;

	buffer[length++] = IAX_IE_VERSION;
	buffer[length++] = sizeof(unsigned short);
	buffer[length++] = 0x00U;
	buffer[length++] = IAX_PROTO_VERSION;

	buffer[length++] = IAX_IE_CALLED_NUMBER;
	buffer[length++] = m_node.size();
	for (std::string::const_iterator it = m_node.cbegin(); it != m_node.cend(); ++it)
		buffer[length++] = *it;

	buffer[length++] = IAX_IE_CALLING_NUMBER;
	buffer[length++] = 0U;

	buffer[length++] = IAX_IE_CALLING_NAME;
	buffer[length++] = m_callsign.size();
	for (std::string::const_iterator it = m_callsign.cbegin(); it != m_callsign.cend(); ++it)
		buffer[length++] = *it;

	buffer[length++] = IAX_IE_USERNAME;
	buffer[length++] = m_username.size();
	for (std::string::const_iterator it = m_username.cbegin(); it != m_username.cend(); ++it)
		buffer[length++] = *it;

	buffer[length++] = IAX_IE_FORMAT;
	buffer[length++] = sizeof(unsigned int);
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = AST_FORMAT_ULAW;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

	m_oSeqNo++;

	return m_socket.write(buffer, length, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeAuth()
{
	char hash[MD5_DIGEST_STRING_LENGTH];
	::MD5Data((unsigned char*)m_password.c_str(), m_password.size(), hash);

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

	buffer[11U] = IAX_COMMAND_AUTHREP;

	buffer[12U] = IAX_IE_MD5_RESULT;
	buffer[13U] = MD5_DIGEST_STRING_LENGTH;
	::memcpy(buffer + 14U, hash, MD5_DIGEST_STRING_LENGTH);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 14U + MD5_DIGEST_STRING_LENGTH);

	m_oSeqNo++;

	return m_socket.write(buffer, 14U + MD5_DIGEST_STRING_LENGTH, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeKey(bool key)
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

	buffer[10U] = AST_FRAME_CONTROL;

	buffer[11U] = key ? AST_CONTROL_KEY : AST_CONTROL_UNKEY;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	m_oSeqNo++;

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
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

	m_oSeqNo++;

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writePong(unsigned int ts)
{
	unsigned short sCall = m_sCallNo | 0x8000U;

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
	buffer[20U] = (m_rxLoss * 100U) / m_rxFrames;
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

	m_oSeqNo++;

	return m_socket.write(buffer, 46U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeAck(unsigned short sCallNo, unsigned short dCallNo, unsigned int ts, unsigned char oSeqNo, unsigned char iSeqNo)
{
	unsigned short sCall = sCallNo | 0x8000U;

	unsigned char buffer[15U];

	buffer[0U] = (sCall << 8) & 0xFFU;
	buffer[1U] = (sCall << 0) & 0xFFU;

	buffer[2U] = (dCallNo << 8) & 0xFFU;
	buffer[3U] = (dCallNo << 0) & 0xFFU;

	buffer[4U] = (ts << 24) & 0xFFU;
	buffer[5U] = (ts << 16) & 0xFFU;
	buffer[6U] = (ts << 8)  & 0xFFU;
	buffer[7U] = (ts << 0)  & 0xFFU;

	buffer[8U] = oSeqNo;

	buffer[9U] = iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_ACK;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	m_oSeqNo++;

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeDisconnect()
{
	const char* REASON = "MMDVM Out";

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

	buffer[11U] = IAX_COMMAND_HANGUP;

	buffer[12U] = IAX_IE_CAUSE;
	buffer[13U] = ::strlen(REASON);
	::memcpy(buffer + 14U, REASON, ::strlen(REASON));

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 14U + ::strlen(REASON));

	m_oSeqNo++;

	return m_socket.write(buffer, 14U + ::strlen(REASON), m_addr, m_addrLen);
}

