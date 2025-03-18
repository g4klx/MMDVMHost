/*
 *   Copyright (C) 2020,2021,2023,2024,2025 by Jonathan Naylor G4KLX
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const unsigned int MMDVM_SAMPLERATE = 8000U;

const unsigned int BUFFER_LENGTH = 1500U;

CFMNetwork::CFMNetwork(const std::string& callsign, const std::string& protocol, const std::string& localAddress, unsigned short localPort, const std::string& gatewayAddress, unsigned short gatewayPort, unsigned int sampleRate, const std::string& squelchFile, bool debug) :
m_callsign(callsign),
m_protocol(FM_NETWORK_PROTOCOL::USRP),
m_socket(localAddress, localPort),
m_addr(),
m_addrLen(0U),
m_sampleRate(sampleRate),
m_squelchFile(squelchFile),
m_debug(debug),
m_enabled(false),
m_buffer(2000U, "FM Network"),
m_seqNo(0U),
#if defined(HAS_SRC)
m_resampler(nullptr),
#endif
m_error(0),
m_fp(nullptr)
{
	assert(!callsign.empty());
	assert(gatewayPort > 0U);
	assert(!gatewayAddress.empty());
	assert(sampleRate > 0U);

	if (CUDPSocket::lookup(gatewayAddress, gatewayPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;

	// Remove any trailing letters in the callsign
	size_t pos = callsign.find_first_of(' ');
	if (pos != std::string::npos)
		m_callsign = callsign.substr(0U, pos);

	if (protocol == "RAW")
		m_protocol = FM_NETWORK_PROTOCOL::RAW;
	else
		m_protocol = FM_NETWORK_PROTOCOL::USRP;

#if defined(HAS_SRC)
	m_resampler = ::src_new(SRC_SINC_FASTEST, 1, &m_error);
#endif
}

CFMNetwork::~CFMNetwork()
{
#if defined(HAS_SRC)
	::src_delete(m_resampler);
#endif
}

bool CFMNetwork::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the FM Gateway");
		return false;
	}

	LogMessage("Opening FM network connection");

	if (m_protocol == FM_NETWORK_PROTOCOL::RAW) {
		if (!m_squelchFile.empty()) {
			m_fp = ::fopen(m_squelchFile.c_str(), "wb");
			if (m_fp == nullptr) {
#if !defined(_WIN32) && !defined(_WIN64)
				LogError("Cannot open the squelch file: %s, errno=%d", m_squelchFile.c_str(), errno);
#else
				LogError("Cannot open the squelch file: %s, errno=%lu", m_squelchFile.c_str(), ::GetLastError());
#endif
				return false;
			}
		}
	}

#if !defined(HAS_SRC)
	if ((m_protocol == FM_NETWORK_PROTOCOL::RAW) && (m_sampleRate != MMDVM_SAMPLERATE)) {
		LogError("The resampler needed for non-native sample rates has not been included");
		return false;
	}
#endif

	return m_socket.open(m_addr);
}

bool CFMNetwork::writeData(const float* data, unsigned int nSamples)
{
	assert(data != nullptr);
	assert(nSamples > 0U);

	if (m_protocol == FM_NETWORK_PROTOCOL::USRP)
		return writeUSRPData(data, nSamples);
	else if (m_protocol == FM_NETWORK_PROTOCOL::RAW)
		return writeRawData(data, nSamples);
	else
		return false;
}

bool CFMNetwork::writeUSRPData(const float* data, unsigned int nSamples)
{
	assert(data != nullptr);
	assert(nSamples > 0U);

	if (m_seqNo == 0U) {
		bool ret = writeUSRPStart();
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
		CUtils::dump(1U, "FM Network Data Sent", buffer, length);

	m_seqNo++;

	return m_socket.write(buffer, length, m_addr, m_addrLen);
}

bool CFMNetwork::writeRawData(const float* in, unsigned int nIn)
{
	assert(in != nullptr);
	assert(nIn > 0U);

	if (m_seqNo == 0U) {
		bool ret = writeRawStart();
		if (!ret)
			return false;
	}

	unsigned char buffer[2000U];

	unsigned int length = 0U;

#if defined(HAS_SRC)
	if (m_sampleRate != MMDVM_SAMPLERATE) {
		unsigned int nOut = (nIn * m_sampleRate) / MMDVM_SAMPLERATE;

		float out[1000U];

		SRC_DATA data;
		data.data_in       = in;
		data.data_out      = out;
		data.input_frames  = nIn;
		data.output_frames = nOut;
		data.end_of_input  = 0;
		data.src_ratio     = float(m_sampleRate) / float(MMDVM_SAMPLERATE);

		int ret = ::src_process(m_resampler, &data);
		if (ret != 0) {
			LogError("Error from the write resampler - %d - %s", ret, ::src_strerror(ret));
			return false;
		}

		for (unsigned int i = 0U; i < nOut; i++) {
			short val = short(out[i] * 32767.0F + 0.5F);	// Changing audio format from float to S16LE

			buffer[length++] = (val >> 0) & 0xFFU;
			buffer[length++] = (val >> 8) & 0xFFU;
		}
	} else {
#endif
		for (unsigned int i = 0U; i < nIn; i++) {
			short val = short(in[i] * 32767.0F + 0.5F);	// Changing audio format from float to S16LE

			buffer[length++] = (val >> 0) & 0xFFU;
			buffer[length++] = (val >> 8) & 0xFFU;
		}
#if defined(HAS_SRC)
	}
#endif

	if (m_debug)
		CUtils::dump(1U, "FM Network Data Sent", buffer, length);

	m_seqNo++;

	return m_socket.write(buffer, length, m_addr, m_addrLen);
}

bool CFMNetwork::writeEnd()
{
	if (m_protocol == FM_NETWORK_PROTOCOL::USRP)
		return writeUSRPEnd();
	else
		return writeRawEnd();
}

bool CFMNetwork::writeUSRPEnd()
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
			CUtils::dump(1U, "FM Network Data Sent", buffer, length);

		return m_socket.write(buffer, length, m_addr, m_addrLen);
	} else {
		return true;
	}
}

bool CFMNetwork::writeRawEnd()
{
	m_seqNo = 0U;

	if (m_fp != nullptr) {
		size_t n = ::fwrite("Z", 1, 1, m_fp);
		if (n != 1) {
#if !defined(_WIN32) && !defined(_WIN64)
			LogError("Cannot write to the squelch file: %s, errno=%d", m_squelchFile.c_str(), errno);
#else
			LogError("Cannot write to the squelch file: %s, errno=%lu", m_squelchFile.c_str(), ::GetLastError());
#endif
			return false;
		}

		::fflush(m_fp);
	}

	return true;
}

void CFMNetwork::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage addr;
	unsigned int addrlen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, addr, addrlen);
	if (length <= 0)
		return;

	// Check if the data is for us
	if (m_protocol == FM_NETWORK_PROTOCOL::USRP) {
		if (!CUDPSocket::match(addr, m_addr, IPMATCHTYPE::ADDRESS_AND_PORT)) {
			LogMessage("FM packet received from an invalid source");
			return;
		}
	} else {
		if (!CUDPSocket::match(addr, m_addr, IPMATCHTYPE::ADDRESS_ONLY)) {
			LogMessage("FM packet received from an invalid source");
			return;
		}
	}

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "FM Network Data Received", buffer, length);

	if (m_protocol == FM_NETWORK_PROTOCOL::USRP) {
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
	} else if (m_protocol == FM_NETWORK_PROTOCOL::RAW) {
		m_buffer.addData(buffer, length);
	}
}

unsigned int CFMNetwork::readData(float* out, unsigned int nOut)
{
	assert(out != nullptr);
	assert(nOut > 0U);

	unsigned int bytes = m_buffer.dataSize() / sizeof(unsigned short);
	if (bytes == 0U)
		return 0U;

#if defined(HAS_SRC)
	if ((m_protocol == FM_NETWORK_PROTOCOL::RAW) && (m_sampleRate != MMDVM_SAMPLERATE)) {
		unsigned int nIn = (nOut * m_sampleRate) / MMDVM_SAMPLERATE;

		if (bytes < nIn) {
			nIn = bytes;
			nOut = (nIn * MMDVM_SAMPLERATE) / m_sampleRate;
		}

		unsigned char buffer[2000U];
		m_buffer.getData(buffer, nIn * sizeof(unsigned short));

		float in[1000U];

		for (unsigned int i = 0U; i < nIn; i++) {
			short val = ((buffer[i * 2U + 0U] & 0xFFU) << 0) + ((buffer[i * 2U + 1U] & 0xFFU) << 8);
			in[i] = float(val) / 65536.0F;
		}

		SRC_DATA data;
		data.data_in       = in;
		data.data_out      = out;
		data.input_frames  = nIn;
		data.output_frames = nOut;
		data.end_of_input  = 0;
		data.src_ratio     = float(MMDVM_SAMPLERATE) / float(m_sampleRate);

		int ret = ::src_process(m_resampler, &data);
		if (ret != 0) {
			LogError("Error from the read resampler - %d - %s", ret, ::src_strerror(ret));
			return false;
		}
	} else {
#endif
		if (bytes < nOut)
			nOut = bytes;

		unsigned char buffer[1500U];
		m_buffer.getData(buffer, nOut * sizeof(unsigned short));

		for (unsigned int i = 0U; i < nOut; i++) {
			short val = ((buffer[i * 2U + 0U] & 0xFFU) << 0) + ((buffer[i * 2U + 1U] & 0xFFU) << 8);
			out[i] = float(val) / 65536.0F;
		}
#if defined(HAS_SRC)
	}
#endif

	return nOut;
}

void CFMNetwork::reset()
{
	m_buffer.clear();
}

void CFMNetwork::close()
{
	m_socket.close();

	if (m_fp != nullptr) {
		::fclose(m_fp);
		m_fp = nullptr;
	}

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

bool CFMNetwork::writeUSRPStart()
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
	buffer[length++] = 3U + 4U + 3U + 1U + 1U + (unsigned char)m_callsign.size() + 1U;

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
			CUtils::dump(1U, "FM Network Data Sent", buffer, length);

		return m_socket.write(buffer, length, m_addr, m_addrLen);
	} else {
		return true;
	}
}

bool CFMNetwork::writeRawStart()
{
	if (m_fp != nullptr) {
		size_t n = ::fwrite("O", 1, 1, m_fp);
		if (n != 1) {
#if !defined(_WIN32) && !defined(_WIN64)
			LogError("Cannot write to the squelch file: %s, errno=%d", m_squelchFile.c_str(), errno);
#else
			LogError("Cannot write to the squelch file: %s, errno=%lu", m_squelchFile.c_str(), ::GetLastError());
#endif
			return false;
		}

		::fflush(m_fp);
	}

	return true;
}

