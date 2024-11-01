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
#include <ctime>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

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
const unsigned char AST_CONTROL_STOP_SOUNDS = 255U;

const unsigned char AST_FORMAT_ULAW     = 4U;

const unsigned short IAX_AUTH_MD5        = 2U;

const unsigned char IAX_COMMAND_NEW       = 1U;
const unsigned char IAX_COMMAND_PING      = 2U;
const unsigned char IAX_COMMAND_PONG      = 3U;
const unsigned char IAX_COMMAND_ACK       = 4U;
const unsigned char IAX_COMMAND_HANGUP    = 5U;
const unsigned char IAX_COMMAND_REJECT    = 6U;
const unsigned char IAX_COMMAND_ACCEPT    = 7U;
const unsigned char IAX_COMMAND_AUTHREQ   = 8U;
const unsigned char IAX_COMMAND_AUTHREP   = 9U;
const unsigned char IAX_COMMAND_INVAL     = 10U;
const unsigned char IAX_COMMAND_LAGRQ     = 11U;
const unsigned char IAX_COMMAND_LAGRP     = 12U;
const unsigned char IAX_COMMAND_REGREQ    = 13U;
const unsigned char IAX_COMMAND_REGAUTH   = 14U;
const unsigned char IAX_COMMAND_REGACK    = 15U;
const unsigned char IAX_COMMAND_REGREJ    = 16U;
const unsigned char IAX_COMMAND_VNAK      = 18U;
const unsigned char IAX_COMMAND_CALLTOKEN = 40U;

const unsigned char IAX_IE_CALLED_NUMBER  = 1U;
const unsigned char IAX_IE_CALLING_NUMBER = 2U;
const unsigned char IAX_IE_CALLING_NAME   = 4U;
const unsigned char IAX_IE_CALLED_CONTEXT = 5U;
const unsigned char IAX_IE_USERNAME       = 6U;
const unsigned char IAX_IE_PASSWORD       = 7U;
const unsigned char IAX_IE_CAPABILITY     = 8U;
const unsigned char IAX_IE_FORMAT         = 9U;
const unsigned char IAX_IE_LANGUAGE       = 10U;
const unsigned char IAX_IE_VERSION        = 11U;
const unsigned char IAX_IE_ADSICPE        = 12U;
const unsigned char IAX_IE_DNID           = 13U;
const unsigned char IAX_IE_AUTHMETHODS    = 14U;
const unsigned char IAX_IE_CHALLENGE      = 15U;
const unsigned char IAX_IE_MD5_RESULT     = 16U;
const unsigned char IAX_IE_RSA_RESULT     = 17U;
const unsigned char IAX_IE_APPARENT_ADDR  = 18U;
const unsigned char IAX_IE_REFRESH        = 19U;
const unsigned char IAX_IE_DPSTATUS       = 20U;
const unsigned char IAX_IE_CALLNO         = 21U;
const unsigned char IAX_IE_CAUSE          = 22U;
const unsigned char IAX_IE_IAX_UNKNOWN    = 23U;
const unsigned char IAX_IE_MSGCOUNT       = 24U;
const unsigned char IAX_IE_AUTOANSWER     = 25U;
const unsigned char IAX_IE_MUSICONHOLD    = 26U;
const unsigned char IAX_IE_TRANSFERID     = 27U;
const unsigned char IAX_IE_RDNIS          = 28U;
const unsigned char IAX_IE_DATETIME       = 31U;
const unsigned char IAX_IE_CALLINGPRES    = 38U;
const unsigned char IAX_IE_CALLINGTON     = 39U;
const unsigned char IAX_IE_CALLINGTNS     = 40U;
const unsigned char IAX_IE_SAMPLINGRATE   = 41U;
const unsigned char IAX_IE_CAUSECODE      = 42U;
const unsigned char IAX_IE_ENCRYPTION     = 43U;
const unsigned char IAX_IE_ENCKEY         = 44U;
const unsigned char IAX_IE_CODEC_PREFS    = 45U;
const unsigned char IAX_IE_RR_JITTER      = 46U;
const unsigned char IAX_IE_RR_LOSS        = 47U;
const unsigned char IAX_IE_RR_PKTS        = 48U;
const unsigned char IAX_IE_RR_DELAY       = 49U;
const unsigned char IAX_IE_RR_DROPPED     = 50U;
const unsigned char IAX_IE_RR_OOO         = 51U;
const unsigned char IAX_IE_OSPTOKEN       = 52U;
const unsigned char IAX_IE_CALLTOKEN      = 54U;

const unsigned int BUFFER_LENGTH = 1500U;


CFMIAXNetwork::CFMIAXNetwork(const std::string& hostFile1, const std::string& hostFile2, const std::string& authAddress, unsigned short authPort, unsigned short localPort, const std::string& password, const std::string& sourceId, const std::string& destinationId, bool debug) :
m_hostFile1(hostFile1),
m_hostFile2(hostFile2),
m_password(password),
m_authAddr(),
m_authAddrLen(0U),
m_sourceId(sourceId),
m_destinationId(destinationId),
m_socket(localPort),
m_serverAddr(),
m_serverAddrLen(0U),
m_debug(debug),
m_enabled(false),
m_buffer(2000U, "FM Network"),
m_status(IAXS_DISCONNECTED),
m_retryTimer(1000U, 5U),
m_pingTimer(1000U, 2U),
m_seed(),
m_timestamp(),
m_sAuthCallNo(0U),
m_dAuthCallNo(0U),
m_sLinkCallNo(0U),
m_dLinkCallNo(0U),
m_iSeqNo(0U),
m_oSeqNo(0U),
m_callToken(),
m_rxJitter(0U),
m_rxLoss(0U),
m_rxFrames(0U),
m_rxDelay(0U),
m_rxDropped(0U),
m_rxOOO(0U),
m_keyed(false),
m_random(),
m_hosts()
#if defined(_WIN32) || defined(_WIN64)
, m_provider(0UL)
#endif
{
	assert(!hostFile1.empty());
	assert(!authAddress.empty());
	assert(!password.empty());
	assert(!sourceId.empty());
	assert(!destinationId.empty());

	if (CUDPSocket::lookup(authAddress, authPort, m_authAddr, m_authAddrLen) != 0)
		m_authAddrLen = 0U;

	std::random_device rd;
	std::mt19937 mt(rd());
	m_random = mt;
}

CFMIAXNetwork::~CFMIAXNetwork()
{
}

bool CFMIAXNetwork::open()
{
	if (m_authAddrLen == 0U) {
		LogError("Unable to resolve the address of the Authorisation Server");
		return false;
	}

	bool ret = loadHostsFile(m_hostFile1);
	if (!ret)
		return false;

	if (!m_hostFile2.empty()) {
		ret = loadHostsFile(m_hostFile2);
		if (!ret)
			return false;
	}

	LogMessage("Opening FM IAX network connection");

#if defined(_WIN32) || defined(_WIN64)
	if (!::CryptAcquireContext(&m_provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		printf("CryptAcquireContext failed: %ld\n", ::GetLastError());
		return false;
	}
#endif

	ret = lookupHost();
	if (!ret) {
		LogError("Cannot find the destination id of %s in the IAX hosts file", m_destinationId.c_str());
		return false;
	}

	ret = m_socket.open(m_authAddr);
	if (!ret)
		return false;

	m_dAuthCallNo = 0U;
	m_dLinkCallNo = 0U;
	m_rxFrames    = 0U;
	m_keyed       = false;

	std::uniform_int_distribution<uint16_t> dist(0x0001U, 0x7FFFU);
	m_sAuthCallNo = dist(m_random);
	m_sLinkCallNo = dist(m_random);

	ret = writeRegReq();
	if (!ret) {
		m_socket.close();
		return false;
	}

	m_status = IAXS_REGISTERING;
	m_retryTimer.start();

	return true;
}

bool CFMIAXNetwork::writeStart()
{
	if (m_status != IAXS_CONNECTED)
		return false;

	if (!m_enabled)
		return false;

	bool ret = writeKey(true);
	if (!ret)
		return false;

	short audio[160U];
	::memset(audio, 0x00U, 160U * sizeof(short));

	return writeAudio(audio, 160U);
}

bool CFMIAXNetwork::writeData(const float* data, unsigned int nSamples)
{
	assert(data != NULL);
	assert(nSamples > 0U);

	if (m_status != IAXS_CONNECTED)
		return false;

	if (!m_enabled)
		return false;

	short audio[300U];
	for (unsigned int i = 0U; i < nSamples; i++)
		audio[i] = short(data[i] * 32767.0F + 0.5F);		// Changing audio format from float to S16LE

	return writeMiniFrame(audio, nSamples);
}

bool CFMIAXNetwork::writeEnd()
{
	if (m_status != IAXS_CONNECTED)
		return false;

	if (!m_enabled)
		return false;

	return writeKey(false);
}

void CFMIAXNetwork::clock(unsigned int ms)
{
	m_retryTimer.clock(ms);
	if (m_retryTimer.isRunning() && m_retryTimer.hasExpired()) {
		switch (m_status) {
			case IAXS_REGISTERING:
				writeRegReq();
				break;
			case IAXS_CONNECTING:
				writeNew();
				break;
			default:
				break;
		}

		m_retryTimer.start();
	}

	m_pingTimer.clock(ms);
	if (m_pingTimer.isRunning() && m_pingTimer.hasExpired()) {
		writePing(m_serverAddr, m_serverAddrLen);
		m_pingTimer.start();
	}

	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage addr;
	unsigned int addrLen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, addr, addrLen);
	if (length <= 0)
		return;

	// Check if the data is for us
	if (!CUDPSocket::match(addr, m_authAddr, IMT_ADDRESS_AND_PORT) &&
		!CUDPSocket::match(addr, m_serverAddr, IMT_ADDRESS_AND_PORT)) {
		LogMessage("FM IAX packet received from an invalid source");
		return;
	}

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);

	unsigned int ts = (buffer[4U] << 24) | (buffer[5U] << 16) | (buffer[6U] << 8) | (buffer[7U] << 0);
	unsigned char iSeqNo = buffer[8U];

	if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_ACK)) {
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_PING)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);
		writePong(addr, addrLen, ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_PONG)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_CALLTOKEN)) {
		m_dLinkCallNo = ((buffer[0U] << 8) | (buffer[1U] << 0)) & 0x7FFFU;

		m_callToken = getIEString(buffer, length, IAX_IE_CALLTOKEN);

		LogMessage("IAX CALLTOKEN received: \"%s\"", m_callToken.c_str());

		writeNew();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_ACCEPT)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		LogMessage("IAX ACCEPT received");

		writeAck(addr, addrLen, ts);

		m_status = IAXS_CONNECTED;
		m_retryTimer.stop();
		m_pingTimer.start();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REGREJ)) {
		LogError("Registraton rejected by the IAX gateway");

		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);

		m_status = IAXS_DISCONNECTED;
		m_keyed = false;

		m_retryTimer.stop();
		m_pingTimer.stop();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REJECT)) {
		LogError("Command rejected by the IAX gateway");

		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);

		m_status = IAXS_DISCONNECTED;
		m_keyed  = false;

		m_retryTimer.stop();
		m_pingTimer.stop();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REGAUTH)) {
		m_dAuthCallNo = ((buffer[0U] << 8) | (buffer[1U] << 0)) & 0x7FFFU;

		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		uint16_t method = getIEUInt16(buffer, length, IAX_IE_AUTHMETHODS);
		if ((method & IAX_AUTH_MD5) == 0x00U) {
			LogError("IAX Gateway wanted something other than MD5 authentication = 0x%02X", method);
			m_status = IAXS_DISCONNECTED;
			m_retryTimer.stop();
			m_pingTimer.stop();
		} else {
			m_seed = getIEString(buffer, length, IAX_IE_CHALLENGE);

			m_status = IAXS_REGISTERING;
			m_iSeqNo = iSeqNo + 1U;

			m_retryTimer.start();
			m_pingTimer.stop();
			writeRegReq();
		}
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REGACK)) {
		LogError("Registraton accepted by the IAX gateway");

		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);

		m_status = IAXS_REGISTERED;

		writeNew();

		m_status = IAXS_CONNECTING;
		m_retryTimer.start();
		m_pingTimer.stop();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_HANGUP)) {
		LogError("Hangup from the IAX gateway");
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);

		m_status = IAXS_DISCONNECTED;
		m_keyed  = false;

		m_retryTimer.stop();
		m_pingTimer.stop();
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_ANSWER)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		LogMessage("IAX ANSWER received");

		writeAck(addr, addrLen, ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_VNAK)) {
		LogError("Messages rejected by the IAX gateway");

		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);
	} else if (compareFrame(buffer, AST_FRAME_TEXT, 0U)) {
		m_iSeqNo = iSeqNo + 1U;

		LogMessage("IAX TEXT received - %.*s", length - 12U, buffer + 12U);

		writeAck(addr, addrLen, ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_LAGRQ)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeLagRq(addr, addrLen);
		writeLagRp(addr, addrLen, ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_LAGRP)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_KEY)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);

		m_keyed = true;
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_UNKEY)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);

		m_keyed = false;
	} else if (compareFrame(buffer, AST_FRAME_VOICE, AST_FORMAT_ULAW)) {
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);

		if (!m_enabled)
			return;

		if (!m_keyed)
			return;

		m_buffer.addData(buffer + 12U, length - 12U);
	} else if ((buffer[0U] & 0x80U) == 0x00U) {
		if (!m_enabled)
			return;

		if (!m_keyed)
			return;

		m_buffer.addData(buffer + 4U, length - 4U);
	} else {
		CUtils::dump(2U, "Unknown IAX message received", buffer, length);

		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(addr, addrLen, ts);
	}
}

unsigned int CFMIAXNetwork::readData(float* out, unsigned int nOut)
{
	assert(out != NULL);
	assert(nOut > 0U);

	unsigned int bytes = m_buffer.dataSize() / sizeof(unsigned char);
	if (bytes == 0U)
		return 0U;

	if (bytes < nOut)
		nOut = bytes;

	unsigned char buffer[1500U];
	m_buffer.getData(buffer, nOut * sizeof(unsigned char));

	short audio[1500U];
	uLawDecode(buffer, audio, nOut);

	for (unsigned int i = 0U; i < nOut; i++)
		out[i] = float(audio[i]) / 65536.0F;

	return nOut;
}

void CFMIAXNetwork::reset()
{
	m_buffer.clear();

	m_keyed = false;
}

void CFMIAXNetwork::close()
{
	writeHangup();

	m_socket.close();

	m_status = IAXS_DISCONNECTED;

	m_retryTimer.stop();
	m_pingTimer.stop();

#if defined(_WIN32) || defined(_WIN64)
	::CryptReleaseContext(m_provider, 0UL);
	m_provider = 0UL;
#endif

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

bool CFMIAXNetwork::writeNew()
{
	unsigned short sCall = m_sLinkCallNo | 0x8000U;

	m_timestamp.start();

	m_oSeqNo = m_iSeqNo = 0U;

	unsigned int length = 0U;

	unsigned char buffer[150U];

	buffer[length++] = (sCall >> 8) & 0xFFU;
	buffer[length++] = (sCall >> 0) & 0xFFU;

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

	unsigned int pos = 0U;
	setIEUInt16(buffer, pos, IAX_IE_VERSION,        IAX_PROTO_VERSION);
	setIEString(buffer, pos, IAX_IE_CALLED_NUMBER,  m_destinationId);
	setIEString(buffer, pos, IAX_IE_CODEC_PREFS,    "DMLC");
	setIEString(buffer, pos, IAX_IE_CALLING_NUMBER, m_sourceId);
	setIEUInt8(buffer,  pos, IAX_IE_CALLINGPRES,    0U);
	setIEUInt8(buffer,  pos, IAX_IE_CALLINGTON,     0U);
	setIEUInt16(buffer, pos, IAX_IE_CALLINGTNS,     0U);
	setIEString(buffer, pos, IAX_IE_CALLING_NAME,   "");
	setIEString(buffer, pos, IAX_IE_LANGUAGE,       "en");
	setIEString(buffer, pos, IAX_IE_USERNAME,       "radio");
	setIEUInt32(buffer, pos, IAX_IE_FORMAT,         AST_FORMAT_ULAW);
	setIEUInt32(buffer, pos, IAX_IE_CAPABILITY,     AST_FORMAT_ULAW);
	setIEUInt16(buffer, pos, IAX_IE_ADSICPE,        2U);

	unsigned int dt = iaxDateTime();
	setIEUInt32(buffer, pos, IAX_IE_DATETIME,       dt);

	length = setIEString(buffer, pos, IAX_IE_CALLTOKEN, m_callToken);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

	m_oSeqNo++;

	return m_socket.write(buffer, length, m_serverAddr, m_serverAddrLen);
}

bool CFMIAXNetwork::writeKey(bool key)
{
	unsigned short sCall = m_sLinkCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dLinkCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dLinkCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_CONTROL;

	buffer[11U] = key ? AST_CONTROL_KEY : AST_CONTROL_UNKEY;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	m_oSeqNo++;

	return m_socket.write(buffer, 12U, m_serverAddr, m_serverAddrLen);
}

bool CFMIAXNetwork::writePing(const sockaddr_storage& addr, unsigned int addrLen)
{
	assert(addrLen > 0U);

	unsigned short sCall = m_sLinkCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dLinkCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dLinkCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_PING;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	m_oSeqNo++;

	return m_socket.write(buffer, 12U, addr, addrLen);
}

bool CFMIAXNetwork::writePong(const sockaddr_storage& addr, unsigned int addrLen, unsigned int ts)
{
	assert(addrLen > 0U);

	unsigned short sCall = m_sLinkCallNo | 0x8000U;

	unsigned char buffer[50U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dLinkCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dLinkCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_PONG;

	unsigned int  loss   = m_rxLoss & 0xFFFFFFU;
	unsigned char pctage = (m_rxLoss * 100U) / m_rxFrames;

	loss |= (pctage << 24);

	unsigned int pos = 0U;
	setIEUInt32(buffer, pos, IAX_IE_RR_JITTER,  m_rxJitter);
	setIEUInt32(buffer, pos, IAX_IE_RR_LOSS,    loss);
	setIEUInt32(buffer, pos, IAX_IE_RR_PKTS,    m_rxFrames);
	setIEUInt16(buffer, pos, IAX_IE_RR_DELAY,   m_rxDelay);
	setIEUInt32(buffer, pos, IAX_IE_RR_DROPPED, m_rxDropped);
	setIEUInt32(buffer, pos, IAX_IE_RR_OOO,     m_rxOOO);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 46U);

	m_oSeqNo++;

	return m_socket.write(buffer, 46U, addr, addrLen);
}

bool CFMIAXNetwork::writeAck(const sockaddr_storage& addr, unsigned int addrLen, unsigned int ts)
{
	assert(addrLen > 0U);

	unsigned short sCallNo = 0U;
	unsigned short dCallNo = 0U;

	if (m_status == IAXS_REGISTERING) {
		sCallNo = m_sAuthCallNo | 0x8000U;
		dCallNo = m_dAuthCallNo;
	} else {
		sCallNo = m_sLinkCallNo | 0x8000U;
		dCallNo = m_dLinkCallNo;
	}

	unsigned char buffer[15U];

	buffer[0U] = (sCallNo >> 8) & 0xFFU;
	buffer[1U] = (sCallNo >> 0) & 0xFFU;

	buffer[2U] = (dCallNo >> 8) & 0xFFU;
	buffer[3U] = (dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_ACK;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	return m_socket.write(buffer, 12U, addr, addrLen);
}

bool CFMIAXNetwork::writeLagRp(const sockaddr_storage& addr, unsigned int addrLen, unsigned int ts)
{
	assert(addrLen > 0U);

	unsigned short sCall = m_sLinkCallNo | 0x8000U;

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dLinkCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dLinkCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_LAGRP;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	m_oSeqNo++;

	return m_socket.write(buffer, 12U, addr, addrLen);
}

bool CFMIAXNetwork::writeLagRq(const sockaddr_storage& addr, unsigned int addrLen)
{
	assert(addrLen > 0U);

	unsigned short sCall = m_sLinkCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dLinkCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dLinkCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_LAGRQ;

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	m_oSeqNo++;

	return m_socket.write(buffer, 12U, addr, addrLen);
}

bool CFMIAXNetwork::writeHangup()
{
	unsigned short sCall = m_sLinkCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[50U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dLinkCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dLinkCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_HANGUP;

	unsigned int pos = 0U;
	unsigned int length = setIEUInt8(buffer, pos, IAX_IE_CAUSE, 0x00U);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

	m_oSeqNo++;

	return m_socket.write(buffer, length, m_serverAddr, m_serverAddrLen);
}

void CFMIAXNetwork::uLawEncode(const short* audio, unsigned char* buffer, unsigned int length) const
{
	assert(audio != NULL);
	assert(buffer != NULL);

	const unsigned short MULAW_MAX  = 0x1FFFU;
	const unsigned short MULAW_BIAS = 33U;

	for (unsigned int i = 0U; i < length; i++) {
		unsigned short mask = 0x1000U;
		unsigned char  sign;
		unsigned char position = 12U;
		unsigned short number;

		if (audio[i] < 0) {
			number = -audio[i];
			sign   = 0x80U;
		} else {
			number = audio[i];
			sign   = 0x00U;
		}

		number += MULAW_BIAS;
		if (number > MULAW_MAX)
			number = MULAW_MAX;

		for (; ((number & mask) != mask && position >= 5U); mask >>= 1, position--)
			;

		unsigned char lsb = (number >> (position - 4U)) & 0x0FU;
		buffer[i] = ~(sign | ((position - 5U) << 4) | lsb);
	}
}

void CFMIAXNetwork::uLawDecode(const unsigned char* buffer, short* audio, unsigned int length) const
{
	assert(buffer != NULL);
	assert(audio != NULL);

	const unsigned short MULAW_BIAS = 33U;

	for (unsigned int i = 0U; i < length; i++) {
		bool sign = true;

		unsigned char number = ~buffer[i];
		if (number & 0x80U) {
			number &= 0x7FU;
			sign = false;
		}

		unsigned char position = ((number & 0xF0U) >> 4) + 5U;
		short decoded = ((1 << position) | ((number & 0x0FU) << (position - 4U))
			 | (1 << (position - 5U))) - MULAW_BIAS;

		audio[i] = sign ? decoded : -decoded;
	}
}

bool CFMIAXNetwork::writeRegReq()
{
	const unsigned short REFRESH_TIME = 60U;

	unsigned short sCall = m_sAuthCallNo | 0x8000U;
	unsigned short dCall = m_dAuthCallNo;

	unsigned int ts = m_timestamp.elapsed();

	unsigned char buffer[70U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (dCall >> 8) & 0xFFU;
	buffer[3U] = (dCall >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8) & 0xFFU;
	buffer[7U] = (ts >> 0) & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_REGREQ;

	unsigned int pos = 0U;
	setIEString(buffer, pos, IAX_IE_USERNAME, m_sourceId);

	setIEUInt16(buffer, pos, IAX_IE_REFRESH, REFRESH_TIME);

	if (m_dAuthCallNo > 0U) {
		std::string password = m_seed + m_password;

#if defined(_WIN32) || defined(_WIN64)
		uint8_t hash[16U];

		HCRYPTHASH hHash = 0;
		if (!::CryptCreateHash(m_provider, CALG_MD5, 0, 0, &hHash)) {
			LogError("CryptCreateHash failed: %ld", ::GetLastError());
			return false;
		}

		if (!::CryptHashData(hHash, (BYTE*)password.c_str(), DWORD(password.size()), 0)) {
			LogError("CryptHashData failed: %ld", ::GetLastError());
			return false;
		}

		DWORD cbHash = 16U;
		if (!::CryptGetHashParam(hHash, HP_HASHVAL, hash, &cbHash, 0)) {
			LogError("CryptGetHashParam failed: %ld", ::GetLastError());
			return false;
		}

		::CryptDestroyHash(hHash);

		char text[33U];
		::sprintf(text, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			hash[0U], hash[1U], hash[2U], hash[3U],
			hash[4U], hash[5U], hash[6U], hash[7U],
			hash[8U], hash[9U], hash[10U], hash[11U],
			hash[12U], hash[13U], hash[14U], hash[15U]);

		setIEString(buffer, pos, IAX_IE_MD5_RESULT, text, 32U);
#else
		char hash[MD5_DIGEST_STRING_LENGTH];

		::MD5Data((uint8_t*)password.c_str(), password.size(), hash);

		setIEString(buffer, pos, IAX_IE_MD5_RESULT, hash, MD5_DIGEST_STRING_LENGTH - 1U);
#endif
	} else {
		m_oSeqNo++;
	}

	unsigned int length = setIEString(buffer, pos, IAX_IE_CALLTOKEN, m_callToken);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

	return m_socket.write(buffer, length, m_authAddr, m_authAddrLen);
}

bool CFMIAXNetwork::writeAudio(const short* audio, unsigned int length)
{
	assert(audio != NULL);
	assert(length > 0U);

	unsigned short sCall = m_sLinkCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[300U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dLinkCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dLinkCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_VOICE;

	buffer[11U] = AST_FORMAT_ULAW;

	uLawEncode(audio, buffer + 12U, length);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U + length);

	m_oSeqNo++;

	return m_socket.write(buffer, 12U + length, m_serverAddr, m_serverAddrLen);
}

bool CFMIAXNetwork::writeMiniFrame(const short* audio, unsigned int length)
{
	assert(audio != NULL);
	assert(length > 0U);

	unsigned short ts = m_timestamp.elapsed();

	unsigned char buffer[300U];

	buffer[0U] = (m_sLinkCallNo >> 8) & 0xFFU;
	buffer[1U] = (m_sLinkCallNo >> 0) & 0xFFU;

	buffer[2U] = (ts >> 8) & 0xFFU;
	buffer[3U] = (ts >> 0) & 0xFFU;

	uLawEncode(audio, buffer + 4U, length);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 4U + length);

	return m_socket.write(buffer, 4U + length, m_serverAddr, m_serverAddrLen);
}

bool CFMIAXNetwork::compareFrame(const unsigned char* buffer, unsigned char type1, unsigned char type2) const
{
	assert(buffer != NULL);

	if ((buffer[0U] & 0x80U) == 0x00U)
		return false;

	return (buffer[10U] == type1) && (buffer[11U] == type2);
}

unsigned int CFMIAXNetwork::iaxDateTime() const
{
	time_t now = ::time(NULL);

	struct tm* tm = ::gmtime(&now);

	unsigned int dt = 0U;
	dt |= (tm->tm_sec  & 0x0000001FU) << 0;		// 5 bits
	dt |= (tm->tm_min  & 0x0000003FU) << 5;		// 6 bits
	dt |= (tm->tm_hour & 0x0000001FU) << 11;	// 5 bits

	dt |= (tm->tm_mday & 0x0000001FU) << 16;	// 5 bits

	dt |= ((tm->tm_mon + 1) & 0x0000000FU) << 21;	// 4 bits

	dt |= ((tm->tm_year - 100) & 0x0000007FU) << 25;	// 7 bits

	return dt;
}

unsigned int CFMIAXNetwork::setIEString(unsigned char* buffer, unsigned int& pos, unsigned char ie, const std::string& text) const
{
	assert(buffer != NULL);

	const unsigned int length = 12U;

	buffer[length + pos] = ie;
	pos++;

	buffer[length + pos] = (unsigned char)text.size();
	pos++;

	for (const auto it : text) {
		buffer[length + pos] = it;
		pos++;
	}

	return length + pos;
}

unsigned int CFMIAXNetwork::setIEString(unsigned char* buffer, unsigned int& pos, unsigned char ie, const char* text, unsigned char size) const
{
	assert(buffer != NULL);
	assert(text != NULL);

	const unsigned int length = 12U;

	buffer[length + pos] = ie;
	pos++;

	buffer[length + pos] = size;
	pos++;

	for (unsigned char i = 0U; i < size; i++) {
		buffer[length + pos] = text[i];
		pos++;
	}

	return length + pos;
}

unsigned int CFMIAXNetwork::setIEUInt32(unsigned char* buffer, unsigned int& pos, unsigned char ie, uint32_t value) const
{
	assert(buffer != NULL);

	const unsigned int length = 12U;

	buffer[length + pos] = ie;
	pos++;

	buffer[length + pos] = (unsigned char)sizeof(uint32_t);
	pos++;

	buffer[length + pos] = (value >> 24) & 0xFFU;
	pos++;

	buffer[length + pos] = (value >> 16) & 0xFFU;
	pos++;

	buffer[length + pos] = (value >> 8) & 0xFFU;
	pos++;

	buffer[length + pos] = (value >> 0) & 0xFFU;
	pos++;

	return length + pos;
}

unsigned int CFMIAXNetwork::setIEUInt16(unsigned char* buffer, unsigned int& pos, unsigned char ie, uint16_t value) const
{
	assert(buffer != NULL);

	const unsigned int length = 12U;

	buffer[length + pos] = ie;
	pos++;

	buffer[length + pos] = (unsigned char)sizeof(uint16_t);
	pos++;

	buffer[length + pos] = (value >> 8) & 0xFFU;
	pos++;

	buffer[length + pos] = (value >> 0) & 0xFFU;
	pos++;

	return length + pos;
}

unsigned int CFMIAXNetwork::setIEUInt8(unsigned char* buffer, unsigned int& pos, unsigned char ie, uint8_t value) const
{
	assert(buffer != NULL);

	const unsigned int length = 12U;

	buffer[length + pos] = ie;
	pos++;

	buffer[length + pos] = (unsigned char)sizeof(uint8_t);
	pos++;

	buffer[length + pos] = value;
	pos++;

	return length + pos;
}

std::string CFMIAXNetwork::getIEString(const unsigned char* buffer, unsigned int length, unsigned char ie) const
{
	assert(buffer != NULL);
	assert(length > 0U);

	unsigned int pos = 12U;

	while (pos < length) {
		if (buffer[pos] == ie) {
			unsigned char len = buffer[pos + 1U];
			return std::string((char*)(buffer + pos + 2U), len);
		} else {
			unsigned char len = buffer[pos + 1U];
			pos += len + 2U;
		}
	}

	return "";
}

uint32_t CFMIAXNetwork::getIEUInt32(const unsigned char* buffer, unsigned int length, unsigned char ie) const
{
	assert(buffer != NULL);
	assert(length > 0U);

	unsigned int pos = 12U;

	while (pos < length) {
		if (buffer[pos] == ie) {
			unsigned char len = buffer[pos + 1U];
			if (len != sizeof(uint32_t))
				return 0U;

			uint32_t value = 0U;
			value |= buffer[pos + 2U] << 24U;
			value |= buffer[pos + 3U] << 16U;
			value |= buffer[pos + 4U] << 8U;
			value |= buffer[pos + 5U] << 0U;
			return value;
		} else {
			unsigned char len = buffer[pos + 1U];
			pos += len + 2U;
		}
	}

	return 0U;
}

uint16_t CFMIAXNetwork::getIEUInt16(const unsigned char* buffer, unsigned int length, unsigned char ie) const
{
	assert(buffer != NULL);
	assert(length > 0U);

	unsigned int pos = 12U;

	while (pos < length) {
		if (buffer[pos] == ie) {
			unsigned char len = buffer[pos + 1U];
			if (len != sizeof(uint16_t))
				return 0U;

			uint16_t value = 0U;
			value |= buffer[pos + 2U] << 8U;
			value |= buffer[pos + 3U] << 0U;
			return value;
		} else {
			unsigned char len = buffer[pos + 1U];
			pos += len + 2U;
		}
	}

	return 0U;
}

uint8_t CFMIAXNetwork::getIEUInt8(const unsigned char* buffer, unsigned int length, unsigned char ie) const
{
	assert(buffer != NULL);
	assert(length > 0U);

	unsigned int pos = 12U;

	while (pos < length) {
		if (buffer[pos] == ie) {
			unsigned char len = buffer[pos + 1U];
			if (len != sizeof(uint8_t))
				return 0U;

			return buffer[pos + 2U];
		} else {
			unsigned char len = buffer[pos + 1U];
			pos += len + 2U;
		}
	}

	return 0U;
}

std::string CFMIAXNetwork::getAddress()
{
	if (m_serverAddrLen == 0U)
		return "NONE";

	char buffer1[250U];
	char buffer2[250U];

	int ret = ::getnameinfo((sockaddr*)&m_serverAddr, m_serverAddrLen, buffer1, 250, buffer2, 250, NI_NUMERICHOST | NI_NUMERICSERV);
	if (ret != 0)
		return "NONE";

	return std::string(buffer1);
}

bool CFMIAXNetwork::loadHostsFile(const std::string& fileName)
{
	assert(!fileName.empty());

	FILE* fp = ::fopen(fileName.c_str(), "rt");
	if (fp == NULL) {
		LogError("Cannot open the IAX hosts file - %s", fileName.c_str());
		return false;
	}

	char buffer[BUFFER_LENGTH];
	unsigned int n = 0U;

	while (::fgets(buffer, BUFFER_LENGTH, fp) != NULL) {
		if ((buffer[0U] < '0') || (buffer[0U] > '9'))
			continue;

		char* p1 = ::strtok(buffer, "=");
		char* p2 = ::strtok(NULL, "@");
		char* p3 = ::strtok(NULL, ":");
		char* p4 = ::strtok(NULL, "/");
		char* p5 = ::strtok(NULL, ",");
		char* p6 = ::strtok(NULL, "\r\n");

		if ((p1 == NULL) || (p2 == NULL) || (p3 == NULL) || (p4 == NULL) || (p5 == NULL) || (p6 == NULL))
			continue;

		m_hosts[p1] = std::pair<std::string, unsigned short>(std::string(p3), (unsigned short)::atoi(p4));
		n++;
	}

	::fclose(fp);

	LogInfo("Loaded %u IAX hosts from %s, hosts list is %d entries long", n, fileName.c_str(), m_hosts.size());

	return true;
}

bool CFMIAXNetwork::lookupHost()
{
	assert(!m_destinationId.empty());

	const auto& lookup = m_hosts.find(m_destinationId);
	if (lookup == m_hosts.cend()) {
		LogError("Cannot find %s in the host list", m_destinationId.c_str());
		return false;
	}

	std::pair<std::string, unsigned short> entry = lookup->second;

	if (CUDPSocket::lookup(entry.first, entry.second, m_serverAddr, m_serverAddrLen) != 0) {
		LogError("Cannot resolve the hostname/address of %s:%u", entry.first.c_str(), entry.second);
		return false;
	}

	LogMessage("Mapped IAX node id of %s to %s:%u", m_destinationId.c_str(), entry.first.c_str(), entry.second);

	return true;
}
