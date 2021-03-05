/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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

#include "FMNetwork.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 500U;

CFMNetwork::CFMNetwork(const std::string& localAddress, unsigned int localPort, const std::string& gatewayAddress, unsigned int gatewayPort, unsigned int sampleRate, bool debug) :
m_socket(localAddress, localPort),
m_addr(),
m_addrLen(0U),
m_sampleRate(sampleRate),
m_debug(debug),
m_enabled(false),
m_buffer(2000U, "FM Network"),
m_pollTimer(1000U, 5U)
{
	assert(gatewayPort > 0U);
	assert(!gatewayAddress.empty());
	assert(sampleRate > 0U);

	if (CUDPSocket::lookup(gatewayAddress, gatewayPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;

#if !defined(_WIN32) && !defined(_WIN64)
	int error;
	m_incoming = ::src_new(SRC_SINC_FASTEST, 1, &error);
	m_outgoing = ::src_new(SRC_SINC_FASTEST, 1, &error);

	assert(m_incoming != NULL);
	assert(m_outgoing != NULL);
#endif
}

CFMNetwork::~CFMNetwork()
{
#if !defined(_WIN32) && !defined(_WIN64)
	::src_delete(m_incoming);
	::src_delete(m_outgoing);
#endif
}

bool CFMNetwork::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the FM Gateway");
		return false;
	}

	LogMessage("Opening FM network connection");

	m_pollTimer.start();

	return m_socket.open(m_addr);
}

bool CFMNetwork::writeData(float* data, unsigned int nSamples)
{
	assert(data != NULL);
	assert(nSamples > 0U);

#if !defined(_WIN32) && !defined(_WIN64)
	assert(m_outgoing != NULL);

	float out[1000U];
	SRC_DATA src;

	if (m_sampleRate != 8000U) {
		src.data_in = data;
		src.data_out = out;
		src.input_frames = nSamples;
		src.output_frames = 1000;
		src.end_of_input = 0;
		src.src_ratio = double(m_sampleRate) / 8000.0;

		int ret = ::src_process(m_outgoing, &src);
		if (ret != 0) {
			LogError("Error up/downsampling of the output audio has an error - %s", src_strerror(ret));
			return false;
		}
	} else {
		src.data_out = data;
		src.output_frames_gen = nSamples;
	}
#endif

	unsigned int length = 3U;

	unsigned char buffer[1500U];
	::memset(buffer, 0x00U, 1500U);

	buffer[0U] = 'F';
	buffer[1U] = 'M';
	buffer[2U] = 'D';

#if defined(_WIN32) || defined(_WIN64)
	for (long i = 0L; i < nSamples; i++) {
		 short val = ( short)((data[i] ) * 32767.0F);	// Changing audio format from  U16BE to S16LE
#else
	for (long i = 0L; i < src.output_frames_gen; i++) {
		 short val = ( short)((src.data_out[i] ) * 32767.0F );	// Changing audio format from  U16BE to S16LE
#endif

		buffer[length++] = (val >> 0) & 0xFFU;	// changing from  BE to LE
		buffer[length++] = (val >> 8) & 0xFFU;	// changing from  BE to LE
	}

	if (m_debug)
		CUtils::dump(1U, "FM Network Data Sent", buffer, length);

	return m_socket.write(buffer, length, m_addr, m_addrLen);
}

bool CFMNetwork::writeEOT()
{
	unsigned char buffer[10U];
	::memset(buffer, 0x00U, 10U);

	buffer[0U] = 'F';
	buffer[1U] = 'M';
	buffer[2U] = 'E';

	if (m_debug)
		CUtils::dump(1U, "FM Network End of Transmission Sent", buffer, 3U);

	return m_socket.write(buffer, 3U, m_addr, m_addrLen);
}

void CFMNetwork::clock(unsigned int ms)
{
	m_pollTimer.clock(ms);
	if (m_pollTimer.hasExpired()) {
		writePoll();
		m_pollTimer.start();
	}

	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage addr;
	unsigned int addrlen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, addr, addrlen);
	if (length <= 0)
		return;

	// Check if the data is for us					// does not accept data from USRP 
	//if (!CUDPSocket::match(addr, m_addr)) {
	//	LogMessage("FM packet received from an invalid source");
	//	return;
	//}

	// Ignore incoming polls
	if (::memcmp(buffer, "FMP", 3U) == 0)
		return;

	// Invalid packet type?
	if (::memcmp(buffer, "FMD", 3U) != 0)
		return;

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "FM Network Data Received", buffer, length);

	m_buffer.addData(buffer + 3U, length - 3U);
}

unsigned int CFMNetwork::read(float* data, unsigned int nSamples)
{
	assert(data != NULL);
	assert(nSamples > 0U);

	unsigned int bytes = m_buffer.dataSize() / sizeof(unsigned short);
	if (bytes == 0U)
		return 0U;

	if (bytes < nSamples)
		nSamples = bytes;

	unsigned char buffer[1500U];
	m_buffer.getData(buffer, nSamples * sizeof(unsigned short));

#if !defined(_WIN32) && !defined(_WIN64)
	assert(m_incoming != NULL);

	SRC_DATA src;

	if (m_sampleRate != 8000U) {
		float in[750U];

		for (unsigned int i = 0U; i < nSamples; i++) {
			unsigned short val = ((buffer[i * 2U + 0U] & 0xFFU) << 0) +	// Changing audio format from  U16BE to S16LE
			                     ((buffer[i * 2U + 1U] & 0xFFU) << 8);	// Changing audio format from  U16BE to S16LE
			in[i] = (float(val) / 65536.0F;	// Changing audio format from  U16BE to S16LE
		}

		src.data_in = in;
		src.data_out = data;
		src.input_frames = nSamples;
		src.output_frames = 750;
		src.end_of_input = 0;
		src.src_ratio = 8000.0 / double(m_sampleRate);

		int ret = ::src_process(m_incoming, &src);
		if (ret != 0) {
			LogError("Error up/downsampling of the input audio has an error - %s", src_strerror(ret));
			return 0U;
		}

		return src.output_frames_gen;
	} else {
#endif
		for (unsigned int i = 0U; i < nSamples; i++) {
			unsigned short val = ((buffer[i * 2U + 0U] & 0xFFU) << 0) +	// Changing audio format from  U16BE to S16LE
			                     ((buffer[i * 2U + 1U] & 0xFFU) << 8);	// Changing audio format from  U16BE to S16LE
			data[i] = (float(val) ) / 65536.0F;	// Changing audio format from  U16BE to S16LE
		}

		return nSamples;
#if !defined(_WIN32) && !defined(_WIN64)
	}
#endif
}

void CFMNetwork::reset()
{
#if !defined(_WIN32) && !defined(_WIN64)
	assert(m_incoming != NULL);
	assert(m_outgoing != NULL);

	::src_reset(m_incoming);
	::src_reset(m_outgoing);
#endif

	m_buffer.clear();
}

void CFMNetwork::close()
{
	m_socket.close();

	LogMessage("Closing FM network connection");
}

void CFMNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();
	else if (!enabled && m_enabled)
		reset();

	m_enabled = enabled;
}

bool CFMNetwork::writePoll()
{
	unsigned char buffer[3U];

	buffer[0U] = 'F';
	buffer[1U] = 'M';
	buffer[2U] = 'P';

	if (m_debug)
		CUtils::dump(1U, "FM Network Poll Sent", buffer, 3U);

	return m_socket.write(buffer, 3U, m_addr, m_addrLen);
}
