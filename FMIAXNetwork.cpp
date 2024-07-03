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

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#define	DEBUG_IAX

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

#if !defined(MD5_DIGEST_STRING_LENGTH)
#define	MD5_DIGEST_STRING_LENGTH	16
#endif

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
m_status(IAXS_DISCONNECTED),
m_retryTimer(1000U, 0U, 500U),
m_pingTimer(1000U, 20U),
m_seed(),
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
m_rxOOO(0U),
m_keyed(false)
#if defined(_WIN32) || defined(_WIN64)
, m_provider(0UL)
#endif
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

#if defined(_WIN32) || defined(_WIN64)
	if (!::CryptAcquireContext(&m_provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		LogError("CryptAcquireContext failed: %ld", ::GetLastError());
		return false;
	}
#endif

	bool ret = m_socket.open(m_addr);
	if (!ret)
		return false;

	m_dCallNo  = 0U;
	m_rxFrames = 0U;
	m_keyed    = false;

	ret = writeNew(false);
	if (!ret) {
		m_socket.close();
		return false;
	}

	m_status = IAXS_CONNECTING;
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

#if defined(DEBUG_IAX)
	LogDebug("IAX audio sent");
#endif
	unsigned short ts = m_timestamp.elapsed();

	unsigned char buffer[300U];

	buffer[0U] = (m_sCallNo >> 8) & 0xFFU;
	buffer[1U] = (m_sCallNo >> 0) & 0xFFU;

	buffer[2U] = (ts >> 8) & 0xFFU;
	buffer[3U] = (ts >> 0) & 0xFFU;

	uLawEncode(audio, buffer + 4U, nSamples);

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 4U + nSamples);

	return m_socket.write(buffer, 4U + nSamples, m_addr, m_addrLen);
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
			case IAXS_CONNECTING:
				writeNew(true);
				break;
			case IAXS_REGISTERING:
				writeRegReq(true);
				break;
			default:
				break;
		}

		m_retryTimer.start();
	}

	m_pingTimer.clock(ms);
	if (m_pingTimer.isRunning() && m_pingTimer.hasExpired()) {
		writePing();
		m_pingTimer.start();
	}

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

	if (m_debug)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);

	unsigned int ts = (buffer[4U] << 24) | (buffer[5U] << 16) | (buffer[6U] << 8) | (buffer[7U] << 0);
	unsigned char iSeqNo = buffer[8U];

	if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_ACK)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX ACK received");
#endif
		// Grab the destination call number if we don't have it already
		if (m_dCallNo == 0U)
			m_dCallNo = ((buffer[0U] << 8) | (buffer[1U] << 0)) & 0x7FFFU;
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_PING)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX PING received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
		writePong(ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_PONG)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX PONG received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_ACCEPT)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX ACCEPT received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		m_status = IAXS_CONNECTED;
		m_retryTimer.stop();
		m_pingTimer.start();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REGREJ)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX REGREJ received");
#endif
		LogError("Registraton rejected by the IAX gateway");

		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		m_status = IAXS_DISCONNECTED;
		m_keyed  = false;

		m_retryTimer.stop();
		m_pingTimer.stop();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REJECT)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX REJECT received");
#endif
		LogError("Command rejected by the IAX gateway");

		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		m_status = IAXS_DISCONNECTED;
		m_keyed  = false;

		m_retryTimer.stop();
		m_pingTimer.stop();
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_RINGING)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX RINGING received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REGAUTH)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX REGAUTH received");
#endif
		m_rxFrames++;

		if ((buffer[12U] == IAX_IE_AUTHMETHODS) &&
		    (buffer[15U] == IAX_AUTH_MD5) &&
		    (buffer[16U] == IAX_IE_CHALLENGE)) {
			m_seed = std::string((char*)(buffer + 18U), buffer[17U]);

			m_status = IAXS_REGISTERING;
			m_iSeqNo = iSeqNo + 1U;

			m_retryTimer.start();
			writeRegReq(false);
		}
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_AUTHREQ)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX AUTHREQ received");
#endif
		m_rxFrames++;

		if ((buffer[12U] == IAX_IE_AUTHMETHODS) &&
		    (buffer[15U] == IAX_AUTH_MD5) &&
		    (buffer[16U] == IAX_IE_CHALLENGE)) {
			m_seed = std::string((char*)(buffer + 18U), buffer[17U]);

			m_iSeqNo = iSeqNo + 1U;

			writeAuthRep();
		}
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_REGACK)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX REGACK received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		m_status = IAXS_CONNECTED;
		m_retryTimer.stop();
		m_pingTimer.start();
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_HANGUP)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX HANGUP received");
#endif
		LogError("Hangup from the IAX gateway");
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		m_status = IAXS_DISCONNECTED;
		m_keyed  = false;

		m_retryTimer.stop();
		m_pingTimer.stop();
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_ANSWER)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX ANSWER received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_VNAK)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX VNAK received");
#endif
		LogError("Messages rejected by the IAX gateway");

		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_STOP_SOUNDS)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX STOP SOUNDS received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_OPTION)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX OPTION received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_TEXT, 0U)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX TEXT received - %s", buffer + 12U);
#endif
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_LAGRQ)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX LAGRQ received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeLagRq();
		writeLagRp(ts);
	} else if (compareFrame(buffer, AST_FRAME_IAX, IAX_COMMAND_LAGRP)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX LAGRP received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_KEY)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX KEY received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		m_keyed = true;
	} else if (compareFrame(buffer, AST_FRAME_CONTROL, AST_CONTROL_UNKEY)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX UNKEY received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		m_keyed = false;
	} else if (compareFrame(buffer, AST_FRAME_VOICE, AST_FORMAT_ULAW)) {
#if defined(DEBUG_IAX)
		CUtils::dump(1U, "FM IAX Network Data Received", buffer, length);
		LogDebug("IAX ULAW received");
#endif
		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);

		if (!m_enabled)
			return;

		if (!m_keyed)
			return;

		m_buffer.addData(buffer + 12U, length - 12U);
	} else if ((buffer[0U] & 0x80U) == 0x00U) {
#if defined(DEBUG_IAX)
		LogDebug("IAX audio received");
#endif
		if (!m_enabled)
			return;

		if (!m_keyed)
			return;

		m_buffer.addData(buffer + 4U, length - 4U);
	} else {
		CUtils::dump(2U, "Unknown IAX message received", buffer, length);

		m_rxFrames++;
		m_iSeqNo = iSeqNo + 1U;

		writeAck(ts);
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

bool CFMIAXNetwork::writeNew(bool retry)
{
#if defined(DEBUG_IAX)
	LogDebug("IAX NEW sent");
#endif
	if (!retry)
		m_sCallNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;

	m_timestamp.start();

	m_oSeqNo  = m_iSeqNo = 0U;
	m_dCallNo = 0U;

	unsigned int length = 0U;

	unsigned char buffer[100U];

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

	buffer[length++] = IAX_IE_VERSION;
	buffer[length++] = sizeof(unsigned short);
	buffer[length++] = 0x00U;
	buffer[length++] = IAX_PROTO_VERSION;

	buffer[length++] = IAX_IE_CALLED_NUMBER;
	buffer[length++] = (unsigned char)m_node.size();
	for (std::string::const_iterator it = m_node.cbegin(); it != m_node.cend(); ++it)
		buffer[length++] = *it;

	buffer[length++] = IAX_IE_CALLING_NUMBER;
	buffer[length++] = 0U;

	buffer[length++] = IAX_IE_CALLING_NAME;
	buffer[length++] = (unsigned char)m_callsign.size();
	for (std::string::const_iterator it = m_callsign.cbegin(); it != m_callsign.cend(); ++it)
		buffer[length++] = *it;

	buffer[length++] = IAX_IE_USERNAME;
	buffer[length++] = (unsigned char)m_username.size();
	for (std::string::const_iterator it = m_username.cbegin(); it != m_username.cend(); ++it)
		buffer[length++] = *it;

	buffer[length++] = IAX_IE_FORMAT;
	buffer[length++] = sizeof(unsigned int);
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = 0x00U;
	buffer[length++] = AST_FORMAT_ULAW;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, length);

	return m_socket.write(buffer, length, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeAuthRep()
{
#if defined(DEBUG_IAX)
	LogDebug("IAX AUTHREP sent");
#endif
	m_oSeqNo++;

	std::string password = m_seed + m_password;

	char hash[MD5_DIGEST_STRING_LENGTH];

#if defined(_WIN32) || defined(_WIN64)
	HCRYPTHASH hHash = 0;
	if (!::CryptCreateHash(m_provider, CALG_MD5, 0, 0, &hHash)) {
		LogError("CryptCreateHash failed: %ld", ::GetLastError());
		return false;
	}

	if (!::CryptHashData(hHash, (BYTE*)password.c_str(), DWORD(password.size()), 0)) {
		LogError("CryptHashData failed: %ld", ::GetLastError());
		return false;
	}

	DWORD cbHash = MD5_DIGEST_STRING_LENGTH;
	if (!::CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)hash, &cbHash, 0)) {
		LogError("CryptGetHashParam failed: %ld", ::GetLastError());
		return false;
	}

	::CryptDestroyHash(hHash);
#else
	::MD5Data((unsigned char*)password.c_str(), password.size(), hash);
#endif

	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[50U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_AUTHREP;

	buffer[12U] = IAX_IE_MD5_RESULT;
	buffer[13U] = MD5_DIGEST_STRING_LENGTH;
	::memcpy(buffer + 14U, hash, MD5_DIGEST_STRING_LENGTH);

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 14U + MD5_DIGEST_STRING_LENGTH);

	return m_socket.write(buffer, 14U + MD5_DIGEST_STRING_LENGTH, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeKey(bool key)
{
#if defined(DEBUG_IAX)
	LogDebug("IAX KEY/UNKEY sent");
#endif
	m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_CONTROL;

	buffer[11U] = key ? AST_CONTROL_KEY : AST_CONTROL_UNKEY;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writePing()
{
#if defined(DEBUG_IAX)
	LogDebug("IAX PING sent");
#endif
	m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_PING;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writePong(unsigned int ts)
{
#if defined(DEBUG_IAX)
	LogDebug("IAX PONG sent");
#endif
	m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;

	unsigned char buffer[50U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_PONG;

	buffer[12U] = IAX_IE_RR_JITTER;
	buffer[13U] = sizeof(unsigned int);
	buffer[14U] = (m_rxJitter >> 24) & 0xFFU;
	buffer[15U] = (m_rxJitter >> 16) & 0xFFU;
	buffer[16U] = (m_rxJitter >> 8)  & 0xFFU;
	buffer[17U] = (m_rxJitter >> 0)  & 0xFFU;

	buffer[18U] = IAX_IE_RR_LOSS;
	buffer[19U] = sizeof(unsigned int);
	buffer[20U] = (m_rxLoss * 100U) / m_rxFrames;
	buffer[21U] = (m_rxLoss >> 16) & 0xFFU;
	buffer[22U] = (m_rxLoss >> 8)  & 0xFFU;
	buffer[23U] = (m_rxLoss >> 0)  & 0xFFU;

	buffer[24U] = IAX_IE_RR_PKTS;
	buffer[25U] = sizeof(unsigned int);
	buffer[26U] = (m_rxFrames >> 24) & 0xFFU;
	buffer[27U] = (m_rxFrames >> 16) & 0xFFU;
	buffer[28U] = (m_rxFrames >> 8)  & 0xFFU;
	buffer[29U] = (m_rxFrames >> 0)  & 0xFFU;

	buffer[30U] = IAX_IE_RR_DELAY;
	buffer[31U] = sizeof(unsigned short);
	buffer[32U] = (m_rxDelay >> 8)  & 0xFFU;
	buffer[33U] = (m_rxDelay >> 0)  & 0xFFU;

	buffer[34U] = IAX_IE_RR_DROPPED;
	buffer[35U] = sizeof(unsigned int);
	buffer[36U] = (m_rxDropped >> 24) & 0xFFU;
	buffer[37U] = (m_rxDropped >> 16) & 0xFFU;
	buffer[38U] = (m_rxDropped >> 8)  & 0xFFU;
	buffer[39U] = (m_rxDropped >> 0)  & 0xFFU;

	buffer[40U] = IAX_IE_RR_OOO;
	buffer[41U] = sizeof(unsigned int);
	buffer[42U] = (m_rxOOO >> 24) & 0xFFU;
	buffer[43U] = (m_rxOOO >> 16) & 0xFFU;
	buffer[44U] = (m_rxOOO >> 8)  & 0xFFU;
	buffer[45U] = (m_rxOOO >> 0)  & 0xFFU;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 46U);

	return m_socket.write(buffer, 46U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeAck(unsigned int ts)
{
#if defined(DEBUG_IAX)
	LogDebug("IAX ACK sent");
#endif
	unsigned short sCall = m_sCallNo | 0x8000U;

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_ACK;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeLagRp(unsigned int ts)
{
#if defined(DEBUG_IAX)
	LogDebug("IAX LAGRP sent");
#endif
	m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_LAGRP;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeLagRq()
{
#if defined(DEBUG_IAX)
	LogDebug("IAX LAGRQ sent");
#endif
	m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[15U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_LAGRQ;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 12U);

	return m_socket.write(buffer, 12U, m_addr, m_addrLen);
}

bool CFMIAXNetwork::writeHangup()
{
#if defined(DEBUG_IAX)
	LogDebug("IAX HANGUP sent");
#endif
	const char* REASON = "MMDVM Out";

	m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[50U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_HANGUP;

	buffer[12U] = IAX_IE_CAUSE;
	buffer[13U] = (unsigned char)::strlen(REASON);
	::memcpy(buffer + 14U, REASON, ::strlen(REASON));

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif

	CUtils::dump(1U, "FM IAX Network Data Sent", buffer, 14U + (unsigned int)::strlen(REASON));

	return m_socket.write(buffer, 14U + ::strlen(REASON), m_addr, (unsigned int)m_addrLen);
}

bool CFMIAXNetwork::writeRegReq(bool retry)
{
	const unsigned short REFRESH_TIME = 60U;

#if defined(DEBUG_IAX)
	LogDebug("IAX REGREQ sent");
#endif
	if (!retry)
		m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned short dCall = m_dCallNo;
	if (retry)
		dCall |= 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[70U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (dCall >> 8) & 0xFFU;
	buffer[3U] = (dCall >> 0) & 0xFFU;

	buffer[4U] = (ts >> 24) & 0xFFU;
	buffer[5U] = (ts >> 16) & 0xFFU;
	buffer[6U] = (ts >> 8)  & 0xFFU;
	buffer[7U] = (ts >> 0)  & 0xFFU;

	buffer[8U] = m_oSeqNo;

	buffer[9U] = m_iSeqNo;

	buffer[10U] = AST_FRAME_IAX;

	buffer[11U] = IAX_COMMAND_REGREQ;

	buffer[12U] = IAX_IE_USERNAME;
	buffer[13U] = (unsigned char)m_username.size();
	::memcpy(buffer + 14U, m_username.c_str(), m_username.size());

	unsigned int offset = 14U + (unsigned int)m_username.size();

	if (m_dCallNo > 0U) {
		std::string password = m_seed + m_password;

		char hash[MD5_DIGEST_STRING_LENGTH];

#if defined(_WIN32) || defined(_WIN64)
		HCRYPTHASH hHash = 0;
		if (!::CryptCreateHash(m_provider, CALG_MD5, 0, 0, &hHash)) {
			LogError("CryptCreateHash failed: %ld", ::GetLastError());
			return false;
		}

		if (!::CryptHashData(hHash, (BYTE*)password.c_str(), DWORD(password.size()), 0)) {
			LogError("CryptHashData failed: %ld", ::GetLastError());
			return false;
		}

		DWORD cbHash = MD5_DIGEST_STRING_LENGTH;
		if (!::CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)hash, &cbHash, 0)) {
			LogError("CryptGetHashParam failed: %ld", ::GetLastError());
			return false;
		}

		::CryptDestroyHash(hHash);
#else
		::MD5Data((unsigned char*)password.c_str(), password.size(), hash);
#endif

		buffer[offset++] = IAX_IE_MD5_RESULT;
		buffer[offset++] = MD5_DIGEST_STRING_LENGTH;

		::memcpy(buffer + offset, hash, MD5_DIGEST_STRING_LENGTH);
		offset += MD5_DIGEST_STRING_LENGTH;
	}

	buffer[offset++] = IAX_IE_REFRESH;
	buffer[offset++] = sizeof(unsigned short);
	buffer[offset++] = (REFRESH_TIME >> 8) & 0xFFU;
	buffer[offset++] = (REFRESH_TIME >> 0) & 0xFFU;

#if !defined(DEBUG_IAX)
	if (m_debug)
#endif
		CUtils::dump(1U, "FM IAX Network Data Sent", buffer, offset);

	return m_socket.write(buffer, offset, m_addr, m_addrLen);
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

bool CFMIAXNetwork::writeAudio(const short* audio, unsigned int length)
{
#if defined(DEBUG_IAX)
	LogDebug("IAX ULAW sent");
#endif
	m_oSeqNo++;

	unsigned short sCall = m_sCallNo | 0x8000U;
	unsigned int   ts    = m_timestamp.elapsed();

	unsigned char buffer[300U];

	buffer[0U] = (sCall >> 8) & 0xFFU;
	buffer[1U] = (sCall >> 0) & 0xFFU;

	buffer[2U] = (m_dCallNo >> 8) & 0xFFU;
	buffer[3U] = (m_dCallNo >> 0) & 0xFFU;

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

	return m_socket.write(buffer, 12U + length, m_addr, m_addrLen);
}

bool CFMIAXNetwork::compareFrame(const unsigned char* buffer, unsigned char type1, unsigned char type2) const
{
	assert(buffer != NULL);

	if ((buffer[0U] & 0x80U) == 0x00U)
		return false;

	return (buffer[10U] == type1) && (buffer[11U] == type2);
}

