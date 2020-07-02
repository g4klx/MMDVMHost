/*
 *   Copyright (C) 2009-2014,2016,2018,2020 by Jonathan Naylor G4KLX
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

#include "NXDNKenwoodNetwork.h"
#include "NXDNCRC.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const unsigned int BUFFER_LENGTH = 200U;

CNXDNKenwoodNetwork::CNXDNKenwoodNetwork(const std::string& localAddress, unsigned int localPort, const std::string& gwyAddress, unsigned int gwyPort, bool debug) :
m_rtpSocket(localAddress, localPort + 0U),
m_rtcpSocket(localAddress, localPort + 1U),
m_address(),
m_rtcpPort(gwyPort + 1U),
m_rtpPort(gwyPort + 0U),
m_enabled(false),
m_headerSeen(false),
m_seen1(false),
m_seen2(false),
m_seen3(false),
m_seen4(false),
m_sacch(NULL),
m_sessionId(1U),
m_seqNo(0U),
m_ssrc(0U),
m_debug(debug),
m_startSecs(0U),
m_startUSecs(0U),
m_rtcpTimer(1000U, 0U, 200U),
m_hangTimer(1000U, 5U),
m_hangType(0U),
m_hangSrc(0U),
m_hangDst(0U),
m_random()
{
	assert(localPort > 0U);
	assert(!gwyAddress.empty());
	assert(gwyPort > 0U);

	m_sacch = new unsigned char[10U];

	m_address = CUDPSocket::lookup(gwyAddress);

	std::random_device rd;
	std::mt19937 mt(rd());
	m_random = mt;
}

CNXDNKenwoodNetwork::~CNXDNKenwoodNetwork()
{
	delete[] m_sacch;
}

bool CNXDNKenwoodNetwork::open()
{
	LogMessage("Opening Kenwood connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	if (!m_rtcpSocket.open())
		return false;

	if (!m_rtpSocket.open()) {
		m_rtcpSocket.close();
		return false;
	}

	std::uniform_int_distribution<unsigned int> dist(0x00000001, 0xfffffffe);
	m_ssrc = dist(m_random);

	return true;
}

bool CNXDNKenwoodNetwork::write(const unsigned char* data, NXDN_NETWORK_MESSAGE_TYPE type)
{
	assert(data != NULL);

	switch (type) {
	case NNMT_VOICE_HEADER:	// Voice header or trailer
	case NNMT_VOICE_TRAILER:
		return processIcomVoiceHeader(data);
	case NNMT_VOICE_BODY:	// Voice data
		return processIcomVoiceData(data);
	case NNMT_DATA_HEADER:	// Data header or trailer
	case NNMT_DATA_TRAILER:
		return processIcomDataHeader(data);
	case NNMT_DATA_BODY:	// Data data
		return processIcomDataData(data);
	default:
		return false;
	}
}

bool CNXDNKenwoodNetwork::processIcomVoiceHeader(const unsigned char* inData)
{
	assert(inData != NULL);

	unsigned char outData[30U];
	::memset(outData, 0x00U, 30U);

	// SACCH
	outData[0U] = inData[2U];
	outData[1U] = inData[1U];
	outData[2U] = inData[4U] & 0xC0U;
	outData[3U] = inData[3U];

	// FACCH 1+2
	outData[4U]  = outData[14U] = inData[6U];
	outData[5U]  = outData[15U] = inData[5U];
	outData[6U]  = outData[16U] = inData[8U];
	outData[7U]  = outData[17U] = inData[7U];
	outData[8U]  = outData[18U] = inData[10U];
	outData[9U]  = outData[19U] = inData[9U];
	outData[10U] = outData[20U] = inData[12U];
	outData[11U] = outData[21U] = inData[11U];

	unsigned short src = (inData[8U] << 8) + (inData[9U] << 0);
	unsigned short dst = (inData[10U] << 8) + (inData[11U] << 0);
	unsigned char type = (inData[7U] >> 5) & 0x07U;

	switch (inData[5U] & 0x3FU) {
	case 0x01U:
		m_hangTimer.stop();
		m_rtcpTimer.start();
		writeRTCPStart();
		return writeRTPVoiceHeader(outData);
	case 0x08U: {
		m_hangTimer.start();
		bool ret = writeRTPVoiceTrailer(outData);
		writeRTCPHang(type, src, dst);
		return ret;
	}
	default:
		return false;
	}
}

bool CNXDNKenwoodNetwork::processIcomVoiceData(const unsigned char* inData)
{
	assert(inData != NULL);

	unsigned char outData[40U], temp[10U];
	::memset(outData, 0x00U, 40U);

	// SACCH
	outData[0U] = inData[2U];
	outData[1U] = inData[1U];
	outData[2U] = inData[4U] & 0xC0U;
	outData[3U] = inData[3U];

	// Audio 1
	::memset(temp, 0x00U, 10U);
	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned int offset = (5U * 8U) + i;
		bool b = READ_BIT(inData, offset);
		WRITE_BIT(temp, i, b);
	}
	outData[4U] = temp[1U];
	outData[5U] = temp[0U];
	outData[6U] = temp[3U];
	outData[7U] = temp[2U];
	outData[8U] = temp[5U];
	outData[9U] = temp[4U];
	outData[10U] = temp[7U];
	outData[11U] = temp[6U];

	// Audio 2
	::memset(temp, 0x00U, 10U);
	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned int offset = (5U * 8U) + 49U + i;
		bool b = READ_BIT(inData, offset);
		WRITE_BIT(temp, i, b);
	}
	outData[12U] = temp[1U];
	outData[13U] = temp[0U];
	outData[14U] = temp[3U];
	outData[15U] = temp[2U];
	outData[16U] = temp[5U];
	outData[17U] = temp[4U];
	outData[18U] = temp[7U];
	outData[19U] = temp[6U];

	// Audio 3
	::memset(temp, 0x00U, 10U);
	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned int offset = (19U * 8U) + i;
		bool b = READ_BIT(inData, offset);
		WRITE_BIT(temp, i, b);
	}
	outData[20U] = temp[1U];
	outData[21U] = temp[0U];
	outData[22U] = temp[3U];
	outData[23U] = temp[2U];
	outData[24U] = temp[5U];
	outData[25U] = temp[4U];
	outData[26U] = temp[7U];
	outData[27U] = temp[6U];

	// Audio 4
	::memset(temp, 0x00U, 10U);
	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned int offset = (19U * 8U) + 49U + i;
		bool b = READ_BIT(inData, offset);
		WRITE_BIT(temp, i, b);
	}
	outData[28U] = temp[1U];
	outData[29U] = temp[0U];
	outData[30U] = temp[3U];
	outData[31U] = temp[2U];
	outData[32U] = temp[5U];
	outData[33U] = temp[4U];
	outData[34U] = temp[7U];
	outData[35U] = temp[6U];

	return writeRTPVoiceData(outData);
}

bool CNXDNKenwoodNetwork::processIcomDataHeader(const unsigned char* inData)
{
	assert(inData != NULL);

	unsigned char outData[30U];
	::memset(outData, 0x00U, 30U);

	// SACCH
	outData[0U] = inData[2U];
	outData[1U] = inData[1U];
	outData[2U] = inData[4U] & 0xC0U;
	outData[3U] = inData[3U];

	// FACCH 1+2
	outData[4U] = outData[14U] = inData[6U];
	outData[5U] = outData[15U] = inData[5U];
	outData[6U] = outData[16U] = inData[8U];
	outData[7U] = outData[17U] = inData[7U];
	outData[8U] = outData[18U] = inData[10U];
	outData[9U] = outData[19U] = inData[9U];
	outData[10U] = outData[20U] = inData[12U];
	outData[11U] = outData[21U] = inData[11U];

	unsigned short src = (inData[8U] << 8) + (inData[9U] << 0);
	unsigned short dst = (inData[10U] << 8) + (inData[11U] << 0);
	unsigned char type = (inData[7U] >> 5) & 0x07U;

	switch (inData[5U] & 0x3FU) {
	case 0x09U:
		m_hangTimer.stop();
		m_rtcpTimer.start();
		writeRTCPStart();
		return writeRTPDataHeader(outData);
	case 0x08U: {
		m_hangTimer.start();
		bool ret = writeRTPDataTrailer(outData);
		writeRTCPHang(type, src, dst);
		return ret;
	}
	default:
		return false;
	}
}

bool CNXDNKenwoodNetwork::processIcomDataData(const unsigned char* inData)
{
	assert(inData != NULL);

	unsigned char outData[40U];
	::memset(outData, 0x00U, 40U);

	outData[0U]  = inData[2U];
	outData[1U]  = inData[1U];
	outData[2U]  = inData[4U];
	outData[3U]  = inData[3U];
	outData[4U]  = inData[6U];
	outData[5U]  = inData[5U];
	outData[6U]  = inData[8U];
	outData[7U]  = inData[7U];
	outData[8U]  = inData[10U];
	outData[9U]  = inData[9U];
	outData[10U] = inData[12U];
	outData[11U] = inData[11U];
	outData[12U] = inData[14U];
	outData[13U] = inData[13U];
	outData[14U] = inData[16U];
	outData[15U] = inData[15U];
	outData[16U] = inData[18U];
	outData[17U] = inData[17U];
	outData[18U] = inData[20U];
	outData[19U] = inData[19U];
	outData[20U] = inData[22U];
	outData[21U] = inData[21U];

	return writeRTPDataData(outData);
}

bool CNXDNKenwoodNetwork::writeRTPVoiceHeader(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[50U];
	::memset(buffer, 0x00U, 50U);

	buffer[0U] = 0x80U;
	buffer[1U] = 0x66U;

	m_seqNo++;
	buffer[2U] = (m_seqNo >> 8) & 0xFFU;
	buffer[3U] = (m_seqNo >> 0) & 0xFFU;

	unsigned long timeStamp = getTimeStamp();
	buffer[4U] = (timeStamp >> 24) & 0xFFU;
	buffer[5U] = (timeStamp >> 16) & 0xFFU;
	buffer[6U] = (timeStamp >> 8) & 0xFFU;
	buffer[7U] = (timeStamp >> 0) & 0xFFU;

	buffer[8U] = (m_ssrc >> 24) & 0xFFU;
	buffer[9U] = (m_ssrc >> 16) & 0xFFU;
	buffer[10U] = (m_ssrc >> 8) & 0xFFU;
	buffer[11U] = (m_ssrc >> 0) & 0xFFU;

	m_sessionId++;
	buffer[12U] = m_sessionId;

	buffer[13U] = 0x00U;
	buffer[14U] = 0x00U;
	buffer[15U] = 0x00U;
	buffer[16U] = 0x03U;
	buffer[17U] = 0x03U;
	buffer[18U] = 0x04U;
	buffer[19U] = 0x04U;
	buffer[20U] = 0x0AU;
	buffer[21U] = 0x05U;
	buffer[22U] = 0x0AU;

	::memcpy(buffer + 23U, data, 24U);

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTP Data Sent", buffer, 47U);

	return m_rtpSocket.write(buffer, 47U, m_address, m_rtpPort);
}

bool CNXDNKenwoodNetwork::writeRTPVoiceTrailer(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[50U];
	::memset(buffer, 0x00U, 50U);

	buffer[0U] = 0x80U;
	buffer[1U] = 0x66U;

	m_seqNo++;
	buffer[2U] = (m_seqNo >> 8) & 0xFFU;
	buffer[3U] = (m_seqNo >> 0) & 0xFFU;

	unsigned long timeStamp = getTimeStamp();
	buffer[4U] = (timeStamp >> 24) & 0xFFU;
	buffer[5U] = (timeStamp >> 16) & 0xFFU;
	buffer[6U] = (timeStamp >> 8)  & 0xFFU;
	buffer[7U] = (timeStamp >> 0)  & 0xFFU;

	buffer[8U]  = (m_ssrc >> 24) & 0xFFU;
	buffer[9U]  = (m_ssrc >> 16) & 0xFFU;
	buffer[10U] = (m_ssrc >> 8) & 0xFFU;
	buffer[11U] = (m_ssrc >> 0) & 0xFFU;

	buffer[12U] = m_sessionId;

	buffer[13U] = 0x00U;
	buffer[14U] = 0x00U;
	buffer[15U] = 0x00U;
	buffer[16U] = 0x03U;
	buffer[17U] = 0x03U;
	buffer[18U] = 0x04U;
	buffer[19U] = 0x04U;
	buffer[20U] = 0x0AU;
	buffer[21U] = 0x05U;
	buffer[22U] = 0x0AU;

	::memcpy(buffer + 23U, data, 24U);

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTP Data Sent", buffer, 47U);

	return m_rtpSocket.write(buffer, 47U, m_address, m_rtpPort);
}

bool CNXDNKenwoodNetwork::writeRTPVoiceData(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[60U];
	::memset(buffer, 0x00U, 60U);

	buffer[0U] = 0x80U;
	buffer[1U] = 0x66U;

	m_seqNo++;
	buffer[2U] = (m_seqNo >> 8) & 0xFFU;
	buffer[3U] = (m_seqNo >> 0) & 0xFFU;

	unsigned long timeStamp = getTimeStamp();
	buffer[4U] = (timeStamp >> 24) & 0xFFU;
	buffer[5U] = (timeStamp >> 16) & 0xFFU;
	buffer[6U] = (timeStamp >> 8)  & 0xFFU;
	buffer[7U] = (timeStamp >> 0)  & 0xFFU;

	buffer[8U]  = (m_ssrc >> 24) & 0xFFU;
	buffer[9U]  = (m_ssrc >> 16) & 0xFFU;
	buffer[10U] = (m_ssrc >> 8)  & 0xFFU;
	buffer[11U] = (m_ssrc >> 0)  & 0xFFU;

	buffer[12U] = m_sessionId;

	buffer[13U] = 0x00U;
	buffer[14U] = 0x00U;
	buffer[15U] = 0x00U;
	buffer[16U] = 0x03U;
	buffer[17U] = 0x02U;
	buffer[18U] = 0x04U;
	buffer[19U] = 0x07U;
	buffer[20U] = 0x10U;
	buffer[21U] = 0x08U;
	buffer[22U] = 0x10U;

	::memcpy(buffer + 23U, data, 36U);

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTP Data Sent", buffer, 59U);

	return m_rtpSocket.write(buffer, 59U, m_address, m_rtpPort);
}

bool CNXDNKenwoodNetwork::writeRTPDataHeader(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[50U];
	::memset(buffer, 0x00U, 50U);

	buffer[0U] = 0x80U;
	buffer[1U] = 0x66U;

	m_seqNo++;
	buffer[2U] = (m_seqNo >> 8) & 0xFFU;
	buffer[3U] = (m_seqNo >> 0) & 0xFFU;

	unsigned long timeStamp = getTimeStamp();
	buffer[4U] = (timeStamp >> 24) & 0xFFU;
	buffer[5U] = (timeStamp >> 16) & 0xFFU;
	buffer[6U] = (timeStamp >> 8) & 0xFFU;
	buffer[7U] = (timeStamp >> 0) & 0xFFU;

	buffer[8U] = (m_ssrc >> 24) & 0xFFU;
	buffer[9U] = (m_ssrc >> 16) & 0xFFU;
	buffer[10U] = (m_ssrc >> 8) & 0xFFU;
	buffer[11U] = (m_ssrc >> 0) & 0xFFU;

	m_sessionId++;
	buffer[12U] = m_sessionId;

	buffer[13U] = 0x00U;
	buffer[14U] = 0x00U;
	buffer[15U] = 0x00U;
	buffer[16U] = 0x01U;
	buffer[17U] = 0x01U;

	::memcpy(buffer + 18U, data, 24U);

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTP Data Sent", buffer, 42U);

	return m_rtpSocket.write(buffer, 42U, m_address, m_rtpPort);
}

bool CNXDNKenwoodNetwork::writeRTPDataTrailer(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[50U];
	::memset(buffer, 0x00U, 50U);

	buffer[0U] = 0x80U;
	buffer[1U] = 0x66U;

	m_seqNo++;
	buffer[2U] = (m_seqNo >> 8) & 0xFFU;
	buffer[3U] = (m_seqNo >> 0) & 0xFFU;

	unsigned long timeStamp = getTimeStamp();
	buffer[4U] = (timeStamp >> 24) & 0xFFU;
	buffer[5U] = (timeStamp >> 16) & 0xFFU;
	buffer[6U] = (timeStamp >> 8) & 0xFFU;
	buffer[7U] = (timeStamp >> 0) & 0xFFU;

	buffer[8U] = (m_ssrc >> 24) & 0xFFU;
	buffer[9U] = (m_ssrc >> 16) & 0xFFU;
	buffer[10U] = (m_ssrc >> 8) & 0xFFU;
	buffer[11U] = (m_ssrc >> 0) & 0xFFU;

	m_sessionId++;
	buffer[12U] = m_sessionId;

	buffer[13U] = 0x00U;
	buffer[14U] = 0x00U;
	buffer[15U] = 0x00U;
	buffer[16U] = 0x01U;
	buffer[17U] = 0x06U;

	::memcpy(buffer + 18U, data, 24U);

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTP Data Sent", buffer, 42U);

	return m_rtpSocket.write(buffer, 42U, m_address, m_rtpPort);
}

bool CNXDNKenwoodNetwork::writeRTPDataData(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[50U];
	::memset(buffer, 0x00U, 50U);

	buffer[0U] = 0x80U;
	buffer[1U] = 0x66U;

	m_seqNo++;
	buffer[2U] = (m_seqNo >> 8) & 0xFFU;
	buffer[3U] = (m_seqNo >> 0) & 0xFFU;

	unsigned long timeStamp = getTimeStamp();
	buffer[4U] = (timeStamp >> 24) & 0xFFU;
	buffer[5U] = (timeStamp >> 16) & 0xFFU;
	buffer[6U] = (timeStamp >> 8) & 0xFFU;
	buffer[7U] = (timeStamp >> 0) & 0xFFU;

	buffer[8U] = (m_ssrc >> 24) & 0xFFU;
	buffer[9U] = (m_ssrc >> 16) & 0xFFU;
	buffer[10U] = (m_ssrc >> 8) & 0xFFU;
	buffer[11U] = (m_ssrc >> 0) & 0xFFU;

	m_sessionId++;
	buffer[12U] = m_sessionId;

	buffer[13U] = 0x00U;
	buffer[14U] = 0x00U;
	buffer[15U] = 0x00U;
	buffer[16U] = 0x01U;
	buffer[17U] = 0x01U;

	::memcpy(buffer + 18U, data, 24U);

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTP Data Sent", buffer, 42U);

	return m_rtpSocket.write(buffer, 42U, m_address, m_rtpPort);
}

bool CNXDNKenwoodNetwork::writeRTCPStart()
{
#if defined(_WIN32) || defined(_WIN64)
	time_t now;
	::time(&now);

	m_startSecs = uint32_t(now);

	SYSTEMTIME st;
	::GetSystemTime(&st);

	m_startUSecs = st.wMilliseconds * 1000U;
#else
	struct timeval tod;
	::gettimeofday(&tod, NULL);

	m_startSecs  = tod.tv_sec;
	m_startUSecs = tod.tv_usec;
#endif

	unsigned char buffer[30U];
	::memset(buffer, 0x00U, 30U);

	buffer[0U] = 0x8AU;
	buffer[1U] = 0xCCU;
	buffer[2U] = 0x00U;
	buffer[3U] = 0x06U;

	buffer[4U] = (m_ssrc >> 24) & 0xFFU;
	buffer[5U] = (m_ssrc >> 16) & 0xFFU;
	buffer[6U] = (m_ssrc >> 8)  & 0xFFU;
	buffer[7U] = (m_ssrc >> 0)  & 0xFFU;

	buffer[8U]  = 'K';
	buffer[9U]  = 'W';
	buffer[10U] = 'N';
	buffer[11U] = 'E';

	buffer[12U] = (m_startSecs >> 24) & 0xFFU;
	buffer[13U] = (m_startSecs >> 16) & 0xFFU;
	buffer[14U] = (m_startSecs >> 8)  & 0xFFU;
	buffer[15U] = (m_startSecs >> 0)  & 0xFFU;

	buffer[16U] = (m_startUSecs >> 24) & 0xFFU;
	buffer[17U] = (m_startUSecs >> 16) & 0xFFU;
	buffer[18U] = (m_startUSecs >> 8)  & 0xFFU;
	buffer[19U] = (m_startUSecs >> 0)  & 0xFFU;

	buffer[22U] = 0x02U;

	buffer[24U] = 0x01U;

	buffer[27U] = 0x0AU;

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTCP Data Sent", buffer, 28U);

	return m_rtcpSocket.write(buffer, 28U, m_address, m_rtcpPort);
}

bool CNXDNKenwoodNetwork::writeRTCPPing()
{
	unsigned char buffer[30U];
	::memset(buffer, 0x00U, 30U);

	buffer[0U] = 0x8AU;
	buffer[1U] = 0xCCU;
	buffer[2U] = 0x00U;
	buffer[3U] = 0x06U;

	buffer[4U] = (m_ssrc >> 24) & 0xFFU;
	buffer[5U] = (m_ssrc >> 16) & 0xFFU;
	buffer[6U] = (m_ssrc >> 8)  & 0xFFU;
	buffer[7U] = (m_ssrc >> 0)  & 0xFFU;

	buffer[8U]  = 'K';
	buffer[9U]  = 'W';
	buffer[10U] = 'N';
	buffer[11U] = 'E';

	buffer[12U] = (m_startSecs >> 24) & 0xFFU;
	buffer[13U] = (m_startSecs >> 16) & 0xFFU;
	buffer[14U] = (m_startSecs >> 8)  & 0xFFU;
	buffer[15U] = (m_startSecs >> 0)  & 0xFFU;

	buffer[16U] = (m_startUSecs >> 24) & 0xFFU;
	buffer[17U] = (m_startUSecs >> 16) & 0xFFU;
	buffer[18U] = (m_startUSecs >> 8)  & 0xFFU;
	buffer[19U] = (m_startUSecs >> 0)  & 0xFFU;

	buffer[22U] = 0x02U;

	buffer[24U] = 0x01U;

	buffer[27U] = 0x7BU;

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTCP Data Sent", buffer, 28U);

	return m_rtcpSocket.write(buffer, 28U, m_address, m_rtcpPort);
}

bool CNXDNKenwoodNetwork::writeRTCPHang(unsigned char type, unsigned short src, unsigned short dst)
{
	m_hangType = type;
	m_hangSrc  = src;
	m_hangDst  = dst;

	return writeRTCPHang();
}

bool CNXDNKenwoodNetwork::writeRTCPHang()
{
	unsigned char buffer[30U];
	::memset(buffer, 0x00U, 30U);

	buffer[0U] = 0x8BU;
	buffer[1U] = 0xCCU;
	buffer[2U] = 0x00U;
	buffer[3U] = 0x04U;

	buffer[4U] = (m_ssrc >> 24) & 0xFFU;
	buffer[5U] = (m_ssrc >> 16) & 0xFFU;
	buffer[6U] = (m_ssrc >> 8)  & 0xFFU;
	buffer[7U] = (m_ssrc >> 0)  & 0xFFU;

	buffer[8U]  = 'K';
	buffer[9U]  = 'W';
	buffer[10U] = 'N';
	buffer[11U] = 'E';

	buffer[12U] = (m_hangSrc >> 8) & 0xFFU;
	buffer[13U] = (m_hangSrc >> 0) & 0xFFU;

	buffer[14U] = (m_hangDst >> 8) & 0xFFU;
	buffer[15U] = (m_hangDst >> 0) & 0xFFU;

	buffer[16U] = m_hangType;

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTCP Data Sent", buffer, 20U);

	return m_rtcpSocket.write(buffer, 20U, m_address, m_rtcpPort);
}

bool CNXDNKenwoodNetwork::read(unsigned char* data)
{
	assert(data != NULL);

	unsigned char dummy[BUFFER_LENGTH];
	readRTCP(dummy);

	unsigned int len = readRTP(data);
	switch (len) {
	case 0U:	// Nothing received
		return false;
	case 35U:	// Voice header or trailer
		return processKenwoodVoiceHeader(data);
	case 47U:	// Voice data
		if (m_headerSeen)
			return processKenwoodVoiceData(data);
		else
			return processKenwoodVoiceLateEntry(data);
	case 31U:	// Data
		return processKenwoodData(data);
	default:
		CUtils::dump(5U, "Unknown data received from the Kenwood network", data, len);
		return false;
	}
}

unsigned int CNXDNKenwoodNetwork::readRTP(unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_rtpSocket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return 0U;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr) {
		LogMessage("Kenwood RTP packet received from an invalid source, %08X != %08X", m_address.s_addr, address.s_addr);
		return 0U;
	}

	if (!m_enabled)
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTP Data Received", buffer, length);

	::memcpy(data, buffer + 12U, length - 12U);

	return length - 12U;
}

unsigned int CNXDNKenwoodNetwork::readRTCP(unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_rtcpSocket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return 0U;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr) {
		LogMessage("Kenwood RTCP packet received from an invalid source, %08X != %08X", m_address.s_addr, address.s_addr);
		return 0U;
	}

	if (!m_enabled)
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "Kenwood Network RTCP Data Received", buffer, length);

	if (::memcmp(buffer + 8U, "KWNE", 4U) != 0) {
		LogError("Missing RTCP KWNE signature");
		return 0U;
	}

	::memcpy(data, buffer + 12U, length - 12U);

	return length - 12U;
}

void CNXDNKenwoodNetwork::reset()
{
	m_rtcpTimer.stop();
	m_hangTimer.stop();

	m_headerSeen = false;
	m_seen1 = false;
	m_seen2 = false;
	m_seen3 = false;
	m_seen4 = false;
}

void CNXDNKenwoodNetwork::close()
{
	m_rtcpSocket.close();
	m_rtpSocket.close();

	LogMessage("Closing Kenwood connection");
}

void CNXDNKenwoodNetwork::clock(unsigned int ms)
{
	m_rtcpTimer.clock(ms);
	if (m_rtcpTimer.isRunning() && m_rtcpTimer.hasExpired()) {
		if (m_hangTimer.isRunning())
			writeRTCPHang();
		else
			writeRTCPPing();
		m_rtcpTimer.start();
	}

	m_hangTimer.clock(ms);
	if (m_hangTimer.isRunning() && m_hangTimer.hasExpired()) {
		m_rtcpTimer.stop();
		m_hangTimer.stop();
	}
}

bool CNXDNKenwoodNetwork::processKenwoodVoiceHeader(unsigned char* inData)
{
	assert(inData != NULL);

	unsigned char outData[50U], temp[20U];
	::memset(outData, 0x00U, 50U);

	// LICH
	outData[0U] = 0x83U;

	// SACCH
	::memset(temp, 0x00U, 20U);
	temp[0U] = inData[12U];
	temp[1U] = inData[11U];
	temp[2U] = inData[14U];
	temp[3U] = inData[13U];
	CNXDNCRC::encodeCRC6(temp, 26U);
	::memcpy(outData + 1U, temp, 4U);

	// FACCH 1+2
	::memset(temp, 0x00U, 20U);
	temp[0U] = inData[16U];
	temp[1U] = inData[15U];
	temp[2U] = inData[18U];
	temp[3U] = inData[17U];
	temp[4U] = inData[20U];
	temp[5U] = inData[19U];
	temp[6U] = inData[22U];
	temp[7U] = inData[21U];
	temp[8U] = inData[24U];
	temp[9U] = inData[23U];
	CNXDNCRC::encodeCRC12(temp, 80U);
	::memcpy(outData + 5U, temp, 12U);
	::memcpy(outData + 19U, temp, 12U);

	switch (outData[5U] & 0x3FU) {
	case 0x01U:
		::memcpy(inData, outData, 33U);
		m_headerSeen = true;
		m_seen1 = false;
		m_seen2 = false;
		m_seen3 = false;
		m_seen4 = false;
		return true;
	case 0x08U:
		::memcpy(inData, outData, 33U);
		m_headerSeen = false;
		m_seen1 = false;
		m_seen2 = false;
		m_seen3 = false;
		m_seen4 = false;
		return true;
	default:
		return false;
	}
}

bool CNXDNKenwoodNetwork::processKenwoodVoiceData(unsigned char* inData)
{
	assert(inData != NULL);

	unsigned char outData[50U], temp[20U];
	::memset(outData, 0x00U, 50U);

	// LICH
	outData[0U] = 0xAEU;

	// SACCH
	::memset(temp, 0x00U, 20U);
	temp[0U] = inData[12U];
	temp[1U] = inData[11U];
	temp[2U] = inData[14U];
	temp[3U] = inData[13U];
	CNXDNCRC::encodeCRC6(temp, 26U);
	::memcpy(outData + 1U, temp, 4U);

	// AMBE 1+2
	unsigned int n = 5U * 8U;

	temp[0U] = inData[16U];
	temp[1U] = inData[15U];
	temp[2U] = inData[18U];
	temp[3U] = inData[17U];
	temp[4U] = inData[20U];
	temp[5U] = inData[19U];
	temp[6U] = inData[22U];
	temp[7U] = inData[21U];

	for (unsigned int i = 0U; i < 49U; i++, n++) {
		bool b = READ_BIT(temp, i);
		WRITE_BIT(outData, n, b);
	}

	temp[0U] = inData[24U];
	temp[1U] = inData[23U];
	temp[2U] = inData[26U];
	temp[3U] = inData[25U];
	temp[4U] = inData[28U];
	temp[5U] = inData[27U];
	temp[6U] = inData[30U];
	temp[7U] = inData[29U];

	for (unsigned int i = 0U; i < 49U; i++, n++) {
		bool b = READ_BIT(temp, i);
		WRITE_BIT(outData, n, b);
	}

	// AMBE 3+4
	n = 19U * 8U;

	temp[0U] = inData[32U];
	temp[1U] = inData[31U];
	temp[2U] = inData[34U];
	temp[3U] = inData[33U];
	temp[4U] = inData[36U];
	temp[5U] = inData[35U];
	temp[6U] = inData[38U];
	temp[7U] = inData[37U];

	for (unsigned int i = 0U; i < 49U; i++, n++) {
		bool b = READ_BIT(temp, i);
		WRITE_BIT(outData, n, b);
	}

	temp[0U] = inData[40U];
	temp[1U] = inData[39U];
	temp[2U] = inData[42U];
	temp[3U] = inData[41U];
	temp[4U] = inData[44U];
	temp[5U] = inData[43U];
	temp[6U] = inData[46U];
	temp[7U] = inData[45U];

	for (unsigned int i = 0U; i < 49U; i++, n++) {
		bool b = READ_BIT(temp, i);
		WRITE_BIT(outData, n, b);
	}

	::memcpy(inData, outData, 33U);

	return true;
}

bool CNXDNKenwoodNetwork::processKenwoodData(unsigned char* inData)
{
	if (inData[7U] != 0x09U && inData[7U] != 0x0BU && inData[7U] != 0x08U)
		return false;

	unsigned char outData[50U];

	if (inData[7U] == 0x09U || inData[7U] == 0x08U) {
		outData[0U]  = 0x90U;
		outData[1U]  = inData[8U];
		outData[2U]  = inData[7U];
		outData[3U]  = inData[10U];
		outData[4U]  = inData[9U];
		outData[5U]  = inData[12U];
		outData[6U]  = inData[11U];
		outData[7U]  = inData[14U];
		outData[8U]  = inData[13U];
		outData[9U]  = inData[16U];
		outData[10U] = inData[15U];
		outData[11U] = inData[18U];
		outData[12U] = inData[17U];
		outData[13U] = inData[20U];
		outData[14U] = inData[19U];
		outData[15U] = inData[22U];
		outData[16U] = inData[21U];
		outData[17U] = inData[24U];
		outData[18U] = inData[23U];
		outData[19U] = inData[26U];
		::memcpy(inData, outData, 20U);
		return true;
	} else {
		outData[0U]  = 0x90U;
		outData[1U]  = inData[8U];
		outData[2U]  = inData[7U];
		outData[3U]  = inData[10U];
		outData[4U]  = inData[9U];
		outData[5U]  = inData[12U];
		outData[6U]  = inData[11U];
		outData[7U]  = inData[14U];
		outData[8U]  = inData[13U];
		outData[9U]  = inData[16U];
		outData[10U] = inData[15U];
		outData[11U] = inData[18U];
		outData[12U] = inData[17U];
		outData[13U] = inData[20U];
		outData[14U] = inData[19U];
		outData[15U] = inData[22U];
		outData[16U] = inData[21U];
		outData[17U] = inData[24U];
		outData[18U] = inData[23U];
		outData[19U] = inData[26U];
		outData[20U] = inData[25U];
		outData[21U] = inData[28U];
		outData[22U] = inData[27U];
		outData[23U] = inData[29U];
		::memcpy(inData, outData, 24U);
		return true;
	}
}

unsigned long CNXDNKenwoodNetwork::getTimeStamp() const
{
	unsigned long timeStamp = 0UL;

#if defined(_WIN32) || defined(_WIN64)
	SYSTEMTIME st;
	::GetSystemTime(&st);

	unsigned int hh = st.wHour;
	unsigned int mm = st.wMinute;
	unsigned int ss = st.wSecond;
	unsigned int ms = st.wMilliseconds;

	timeStamp += hh * 3600U * 1000U * 80U;
	timeStamp += mm * 60U * 1000U * 80U;
	timeStamp += ss * 1000U * 80U;
	timeStamp += ms * 80U;
#else
	struct timeval tod;
	::gettimeofday(&tod, NULL);

	unsigned int ss = tod.tv_sec;
	unsigned int ms = tod.tv_usec / 1000U;

	timeStamp += ss * 1000U * 80U;
	timeStamp += ms * 80U;
#endif

	return timeStamp;
}

bool CNXDNKenwoodNetwork::processKenwoodVoiceLateEntry(unsigned char* inData)
{
	assert(inData != NULL);

	unsigned char sacch[4U];
	sacch[0U] = inData[12U];
	sacch[1U] = inData[11U];
	sacch[2U] = inData[14U];
	sacch[3U] = inData[13U];

	switch (sacch[0U] & 0xC0U) {
	case 0xC0U:
		if (!m_seen1) {
			unsigned int offset = 0U;
			for (unsigned int i = 8U; i < 26U; i++, offset++) {
				bool b = READ_BIT(sacch, i) != 0U;
				WRITE_BIT(m_sacch, offset, b);
			}
			m_seen1 = true;
		}
		break;
	case 0x80U:
		if (!m_seen2) {
			unsigned int offset = 18U;
			for (unsigned int i = 8U; i < 26U; i++, offset++) {
				bool b = READ_BIT(sacch, i) != 0U;
				WRITE_BIT(m_sacch, offset, b);
			}
			m_seen2 = true;
		}
		break;
	case 0x40U:
		if (!m_seen3) {
			unsigned int offset = 36U;
			for (unsigned int i = 8U; i < 26U; i++, offset++) {
				bool b = READ_BIT(sacch, i) != 0U;
				WRITE_BIT(m_sacch, offset, b);
			}
			m_seen3 = true;
		}
		break;
	case 0x00U:
		if (!m_seen4) {
			unsigned int offset = 54U;
			for (unsigned int i = 8U; i < 26U; i++, offset++) {
				bool b = READ_BIT(sacch, i) != 0U;
				WRITE_BIT(m_sacch, offset, b);
			}
			m_seen4 = true;
		}
		break;
	}

	if (!m_seen1 || !m_seen2 || !m_seen3 || !m_seen4)
		return false;

	// Create a dummy header
	// Header SACCH
	inData[11U] = 0x10U;
	inData[12U] = 0x01U;
	inData[13U] = 0x00U;
	inData[14U] = 0x00U;

	// Header FACCH
	inData[15U] = m_sacch[1U];
	inData[16U] = m_sacch[0U];
	inData[17U] = m_sacch[3U];
	inData[18U] = m_sacch[2U];
	inData[19U] = m_sacch[5U];
	inData[20U] = m_sacch[4U];
	inData[21U] = m_sacch[7U];
	inData[22U] = m_sacch[6U];
	inData[23U] = 0x00U;
	inData[24U] = m_sacch[8U];

	return processKenwoodVoiceHeader(inData);
}

void CNXDNKenwoodNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();

	m_enabled = enabled;
}
