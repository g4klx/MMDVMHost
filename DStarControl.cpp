/*
 *	Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "DStarControl.h"
#include "Utils.h"
#include "Sync.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <functional>

const unsigned int MAX_SYNC_BIT_ERRORS = 2U;

bool CallsignCompare(const std::string& arg, const unsigned char* my)
{
	for (unsigned int i = 0U; i < (DSTAR_LONG_CALLSIGN_LENGTH - 1U); i++) {
		if (arg.at(i) != my[i])
			return false;
	}

	return true;
}

// #define	DUMP_DSTAR

CDStarControl::CDStarControl(const std::string& callsign, const std::string& module, bool selfOnly, const std::vector<std::string>& blackList, CDStarNetwork* network, IDisplay* display, unsigned int timeout, bool duplex) :
m_callsign(NULL),
m_gateway(NULL),
m_selfOnly(selfOnly),
m_blackList(blackList),
m_network(network),
m_display(display),
m_duplex(duplex),
m_queue(1000U, "D-Star Control"),
m_rfHeader(),
m_netHeader(),
m_rfState(RS_RF_LISTENING),
m_netState(RS_NET_IDLE),
m_net(false),
m_slowData(),
m_rfN(0U),
m_netN(0U),
m_networkWatchdog(1000U, 0U, 1500U),
m_holdoffTimer(1000U, 0U, 500U),
m_rfTimeoutTimer(1000U, timeout),
m_netTimeoutTimer(1000U, timeout),
m_packetTimer(1000U, 0U, 200U),
m_ackTimer(1000U, 0U, 750U),
m_interval(),
m_elapsed(),
m_rfFrames(0U),
m_netFrames(0U),
m_netLost(0U),
m_fec(),
m_rfBits(0U),
m_netBits(0U),
m_rfErrs(0U),
m_netErrs(0U),
m_lastFrame(NULL),
m_fp(NULL)
{
	assert(display != NULL);

	m_callsign = new unsigned char[DSTAR_LONG_CALLSIGN_LENGTH];
	m_gateway  = new unsigned char[DSTAR_LONG_CALLSIGN_LENGTH];

	m_lastFrame = new unsigned char[DSTAR_FRAME_LENGTH_BYTES + 1U];

	std::string call = callsign;
	call.resize(DSTAR_LONG_CALLSIGN_LENGTH - 1U, ' ');
	std::string mod = module;
	mod.resize(1U, ' ');
	call.append(mod);
	
	std::string gate = callsign;
	gate.resize(DSTAR_LONG_CALLSIGN_LENGTH - 1U, ' ');
	gate.append("G");

	for (unsigned int i = 0U; i < DSTAR_LONG_CALLSIGN_LENGTH; i++) {
		m_callsign[i] = call.at(i);
		m_gateway[i]  = gate.at(i);
	}

	m_interval.start();
}

CDStarControl::~CDStarControl()
{
	delete[] m_callsign;
	delete[] m_gateway;
	delete[] m_lastFrame;
}

bool CDStarControl::writeModem(unsigned char *data)
{
	assert(data != NULL);

	unsigned char type = data[0U];

	if (type == TAG_LOST && m_rfState == RS_RF_AUDIO) {
		if (m_rfBits == 0U) m_rfBits = 1U;
		LogMessage("D-Star, transmission lost, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 50.0F, float(m_rfErrs * 100U) / float(m_rfBits));
		writeEndRF();
		return false;
	}

	if (type == TAG_LOST) {
		if (m_rfState == RS_RF_LATE_ENTRY)
			m_rfState = RS_RF_LISTENING;
		return false;
	}

	if (type == TAG_HEADER) {
		CDStarHeader header(data + 1U);

		// Is this a transmission destined for a repeater?
		if (!header.isRepeater()) {
			LogMessage("D-Star, non repeater RF header received");
			return false;
		}

		unsigned char my1[DSTAR_LONG_CALLSIGN_LENGTH];
		header.getMyCall1(my1);

		if (m_selfOnly && ::memcmp(my1, m_callsign, DSTAR_LONG_CALLSIGN_LENGTH - 1U) != 0) {
			LogMessage("D-Star, invalid access attempt from %8.8s", my1);
			return false;
		}

		if (!m_selfOnly && std::find_if(m_blackList.begin(), m_blackList.end(), std::bind(CallsignCompare, std::placeholders::_1, my1)) != m_blackList.end()) {
			LogMessage("D-Star, invalid access attempt from %8.8s", my1);
			return false;
		}

		unsigned char callsign[DSTAR_LONG_CALLSIGN_LENGTH];
		header.getRPTCall1(callsign);

		// Is it for us?
		if (::memcmp(callsign, m_callsign, DSTAR_LONG_CALLSIGN_LENGTH) != 0) {
			LogMessage("D-Star, received RF header for wrong repeater - %8.8s", callsign);
			return false;
		}

		unsigned char gateway[DSTAR_LONG_CALLSIGN_LENGTH];
		header.getRPTCall2(gateway);

		unsigned char my2[DSTAR_SHORT_CALLSIGN_LENGTH];
		header.getMyCall2(my2);

		unsigned char your[DSTAR_LONG_CALLSIGN_LENGTH];
		header.getYourCall(your);

		m_net = ::memcmp(gateway, m_gateway, DSTAR_LONG_CALLSIGN_LENGTH) == 0;

		// Only start the timeout if not already running
		if (!m_rfTimeoutTimer.isRunning())
			m_rfTimeoutTimer.start();

		m_rfHeader = header;

		m_holdoffTimer.stop();
		m_ackTimer.stop();

		m_rfBits = 1U;
		m_rfErrs = 0U;

		m_rfFrames = 1U;
		m_rfN = 0U;

		if (m_duplex) {
			// Modify the header
			header.setRepeater(false);
			header.setRPTCall1(m_callsign);
			header.setRPTCall2(m_callsign);
			header.get(data + 1U);

			writeQueueHeaderRF(data);
		}

		if (m_net) {
			// Modify the header
			header.setRepeater(false);
			header.setRPTCall1(m_callsign);
			header.setRPTCall2(m_gateway);
			header.get(data + 1U);

			writeNetworkHeaderRF(data);
		}

		m_rfState = RS_RF_AUDIO;

		if (m_netState == RS_NET_IDLE)
			m_display->writeDStar((char*)my1, (char*)my2, (char*)your, "R", "        ");

		LogMessage("D-Star, received RF header from %8.8s/%4.4s to %8.8s", my1, my2, your);
	} else if (type == TAG_EOT) {
		if (m_rfState == RS_RF_AUDIO) {
			if (m_net)
				writeNetworkDataRF(DSTAR_END_PATTERN_BYTES, 0U, true);

			if (m_duplex)
				writeQueueEOTRF();

			if (m_rfBits == 0U) m_rfBits = 1U;
			LogMessage("D-Star, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_rfFrames) / 50.0F, float(m_rfErrs * 100U) / float(m_rfBits));

			writeEndRF();
		}

		return false;
	} else if (type == TAG_DATA) {
		if (m_rfState == RS_RF_LISTENING) {
			// The sync is regenerated by the modem so can do exact match
			if (::memcmp(data + 1U + DSTAR_VOICE_FRAME_LENGTH_BYTES, DSTAR_SYNC_BYTES, DSTAR_DATA_FRAME_LENGTH_BYTES) == 0) {
				m_slowData.start();
				m_rfState = RS_RF_LATE_ENTRY;
			}

			return false;
		} else if (m_rfState == RS_RF_AUDIO) {
			unsigned int errors = m_fec.regenerateDStar(data + 1U);

			m_rfErrs += errors;
			m_rfBits += 48U;

			m_rfFrames++;

			// The sync is regenerated by the modem so can do exact match
			if (::memcmp(data + 1U + DSTAR_VOICE_FRAME_LENGTH_BYTES, DSTAR_SYNC_BYTES, DSTAR_DATA_FRAME_LENGTH_BYTES) == 0)
				m_rfN = 0U;

			// Regenerate the sync
			if (m_rfN == 0U)
				CSync::addDStarSync(data + 1U);

			LogDebug("D-Star, audio sequence no. %u, errs: %u/48", m_rfN, errors);

			if (m_net)
				writeNetworkDataRF(data, errors, false);

			if (m_duplex) {
				blankDTMF(data + 1U);
				writeQueueDataRF(data);
			}

			m_rfN = (m_rfN + 1U) % 21U;
		} else if (m_rfState == RS_RF_LATE_ENTRY) {
			// The sync is regenerated by the modem so can do exact match
			if (::memcmp(data + 1U + DSTAR_VOICE_FRAME_LENGTH_BYTES, DSTAR_SYNC_BYTES, DSTAR_DATA_FRAME_LENGTH_BYTES) == 0) {
				m_slowData.reset();
				return false;
			}

			CDStarHeader* header = m_slowData.add(data + 1U);
			if (header == NULL)
				return false;

			// Is this a transmission destined for a repeater?
			if (!header->isRepeater()) {
				LogMessage("D-Star, non repeater RF header received");
				delete header;
				return false;
			}

			unsigned char my1[DSTAR_LONG_CALLSIGN_LENGTH];
			header->getMyCall1(my1);

			if (m_selfOnly && ::memcmp(my1, m_callsign, DSTAR_LONG_CALLSIGN_LENGTH - 1U) != 0) {
				LogMessage("D-Star, invalid access attempt from %8.8s", my1);
				delete header;
				return false;
			}

			if (!m_selfOnly && std::find_if(m_blackList.begin(), m_blackList.end(), std::bind(CallsignCompare, std::placeholders::_1, my1)) != m_blackList.end()) {
				LogMessage("D-Star, invalid access attempt from %8.8s", my1);
				delete header;
				return false;
			}

			unsigned char callsign[DSTAR_LONG_CALLSIGN_LENGTH];
			header->getRPTCall1(callsign);

			// Is it for us?
			if (::memcmp(callsign, m_callsign, DSTAR_LONG_CALLSIGN_LENGTH) != 0) {
				LogMessage("D-Star, received RF header for wrong repeater - %8.8s", callsign);
				delete header;
				return false;
			}

			unsigned char gateway[DSTAR_LONG_CALLSIGN_LENGTH];
			header->getRPTCall2(gateway);

			unsigned char my2[DSTAR_SHORT_CALLSIGN_LENGTH];
			header->getMyCall2(my2);

			unsigned char your[DSTAR_LONG_CALLSIGN_LENGTH];
			header->getYourCall(your);

			m_net = ::memcmp(gateway, m_gateway, DSTAR_LONG_CALLSIGN_LENGTH) == 0;

			// Only reset the timeout if the timeout is not running
			if (!m_rfTimeoutTimer.isRunning())
				m_rfTimeoutTimer.start();

			// Create a dummy start frame to replace the received frame
			m_ackTimer.stop();

			m_rfHeader = *header;

			m_rfBits = 1U;
			m_rfErrs = 0U;

			m_rfN = 0U;
			m_rfFrames = 1U;

			if (m_duplex) {
				unsigned char start[DSTAR_HEADER_LENGTH_BYTES + 1U];
				start[0U] = TAG_HEADER;

				// Modify the header
				header->setRepeater(false);
				header->setRPTCall1(m_callsign);
				header->setRPTCall2(m_callsign);
				header->get(start + 1U);

				writeQueueHeaderRF(start);
			}

			if (m_net) {
				unsigned char start[DSTAR_HEADER_LENGTH_BYTES + 1U];
				start[0U] = TAG_HEADER;

				// Modify the header
				header->setRepeater(false);
				header->setRPTCall1(m_callsign);
				header->setRPTCall2(m_gateway);
				header->get(start + 1U);

				writeNetworkHeaderRF(start);
			}

			delete header;

			unsigned int errors = m_fec.regenerateDStar(data + 1U);

			m_rfErrs += errors;
			m_rfBits += 48U;

			LogDebug("D-Star, audio sequence no. %u, errs: %u/48", m_rfN, errors);

			if (m_net)
				writeNetworkDataRF(data, errors, false);

			if (m_duplex) {
				blankDTMF(data + 1U);
				writeQueueDataRF(data);
			}

			m_rfState = RS_RF_AUDIO;

			m_rfN = (m_rfN + 1U) % 21U;

			if (m_netState == RS_NET_IDLE)
				m_display->writeDStar((char*)my1, (char*)my2, (char*)your, "R", "        ");

			LogMessage("D-Star, received RF late entry from %8.8s/%4.4s to %8.8s", my1, my2, your);
		}
	} else {
		CUtils::dump("D-Star, unknown data from modem", data, DSTAR_FRAME_LENGTH_BYTES + 1U);
	}

	return true;
}

unsigned int CDStarControl::readModem(unsigned char* data)
{
	assert(data != NULL);

	if (m_queue.isEmpty())
		return 0U;

	// Don't relay data until the timer has stopped.
	if (m_holdoffTimer.isRunning())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CDStarControl::writeEndRF()
{
	m_rfState = RS_RF_LISTENING;

	if (m_netState == RS_NET_IDLE) {
		m_display->clearDStar();
		m_ackTimer.start();

		if (m_network != NULL)
			m_network->reset();
	} else {
		m_rfTimeoutTimer.stop();
	}
}

void CDStarControl::writeEndNet()
{
	m_netState = RS_NET_IDLE;

	m_display->clearDStar();

	m_netTimeoutTimer.stop();
	m_networkWatchdog.stop();
	m_packetTimer.stop();

	if (m_network != NULL)
		m_network->reset();

#if defined(DUMP_DSTAR)
	closeFile();
#endif
}

void CDStarControl::writeNetwork()
{
	assert(m_network != NULL);

	unsigned char data[DSTAR_HEADER_LENGTH_BYTES + 2U];
	unsigned int length = m_network->read(data, DSTAR_HEADER_LENGTH_BYTES + 2U);
	if (length == 0U)
		return;

	if (m_rfState != RS_RF_LISTENING && m_netState == RS_NET_IDLE)
		return;

	m_networkWatchdog.start();

	unsigned char type = data[0U];

	if (type == TAG_HEADER) {
		if (m_netState != RS_NET_IDLE)
			return;

		CDStarHeader header(data + 1U);

		unsigned char my1[DSTAR_LONG_CALLSIGN_LENGTH];
		header.getMyCall1(my1);

		unsigned char my2[DSTAR_SHORT_CALLSIGN_LENGTH];
		header.getMyCall2(my2);

		unsigned char your[DSTAR_LONG_CALLSIGN_LENGTH];
		header.getYourCall(your);

		m_netHeader = header;

		m_netTimeoutTimer.start();
		m_packetTimer.start();
		m_elapsed.start();
		m_ackTimer.stop();

		m_netFrames = 0U;
		m_netLost = 0U;

		m_netN = 0U;

		m_netBits = 1U;
		m_netErrs = 0U;

		writeQueueHeaderNet(data);

#if defined(DUMP_DSTAR)
		openFile();
		writeFile(data + 1U, length - 1U);
#endif
		m_netState = RS_NET_AUDIO;

		LINK_STATUS status = LS_NONE;
		unsigned char reflector[DSTAR_LONG_CALLSIGN_LENGTH];
		if (m_network != NULL)
			m_network->getStatus(status, reflector);

		m_display->writeDStar((char*)my1, (char*)my2, (char*)your, "N", (char*) reflector);

		if (strcmp((char*) reflector, "        ") == 0) {
			LogMessage("D-Star, received network header from %8.8s/%4.4s to %8.8s", my1, my2, your);
		} else {
			LogMessage("D-Star, received network header from %8.8s/%4.4s to %8.8s via %8.8s", my1, my2, your, reflector);
		}
	} else if (type == TAG_EOT) {
		if (m_netState != RS_NET_AUDIO)
			return;

		writeQueueEOTNet();

		data[1U] = TAG_EOT;

#if defined(DUMP_DSTAR)
		writeFile(data + 1U, length - 1U);
		closeFile();
#endif
		// We've received the header and EOT haven't we?
		m_netFrames += 2U;
		if (m_netBits == 0U) m_netBits = 1U;
		LogMessage("D-Star, received network end of transmission, %.1f seconds, %u%% packet loss, BER: %.1f%%", float(m_netFrames) / 50.0F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));

		writeEndNet();
	} else if (type == TAG_DATA) {
		if (m_netState != RS_NET_AUDIO)
			return;

		unsigned char n = data[1U];

		insertSilence(data + 2U, n);

		unsigned int errors = m_fec.regenerateDStar(data + 2U);

		m_netErrs += errors;
		m_netBits += 48U;

		blankDTMF(data + 2U);

		// Regenerate the sync
		if (n == 0U)
			CSync::addDStarSync(data + 2U);

		m_netN = n;

		m_packetTimer.start();
		m_netFrames++;

		data[1U] = TAG_DATA;

#if defined(DUMP_DSTAR)
		writeFile(data + 1U, length - 1U);
#endif
		writeQueueDataNet(data + 1U);
	} else {
		CUtils::dump("D-Star, unknown data from network", data, DSTAR_FRAME_LENGTH_BYTES + 1U);
	}
}

void CDStarControl::clock()
{
	unsigned int ms = m_interval.elapsed();
	m_interval.start();

	if (m_network != NULL)
		writeNetwork();

	m_ackTimer.clock(ms);
	if (m_ackTimer.isRunning() && m_ackTimer.hasExpired()) {
		sendAck();
		m_ackTimer.stop();
	}

	m_holdoffTimer.clock(ms);
	if (m_holdoffTimer.isRunning() && m_holdoffTimer.hasExpired())
		m_holdoffTimer.stop();

	m_rfTimeoutTimer.clock(ms);
	m_netTimeoutTimer.clock(ms);

	if (m_netState == RS_NET_AUDIO) {
		m_networkWatchdog.clock(ms);

		if (m_networkWatchdog.hasExpired()) {
			// We're received the header haven't we?
			m_netFrames += 1U;
			if (m_netBits == 0U) m_netBits = 1U;
			LogMessage("D-Star, network watchdog has expired, %.1f seconds,  %u%% packet loss, BER: %.1f%%", float(m_netFrames) / 50.0F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));
			writeEndNet();
#if defined(DUMP_DSTAR)
			closeFile();
#endif
		}
	}

	if (m_netState == RS_NET_AUDIO) {
		m_packetTimer.clock(ms);

		if (m_packetTimer.isRunning() && m_packetTimer.hasExpired()) {
			unsigned int elapsed = m_elapsed.elapsed();
			unsigned int frames  = elapsed / DSTAR_FRAME_TIME;

			if (frames > m_netFrames) {
				unsigned int count = frames - m_netFrames;
				if (count > 5U) {
					LogMessage("D-Star, lost audio for 200ms filling in, elapsed: %ums, expected: %u, received: %u", elapsed, frames, m_netFrames);
					insertSilence(count - 2U);
				}
			}

			m_packetTimer.start();
		}
	}
}

void CDStarControl::writeQueueHeaderRF(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netState != RS_NET_IDLE)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	unsigned char len = DSTAR_HEADER_LENGTH_BYTES + 1U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("D-Star, overflow in the D-Star RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CDStarControl::writeQueueDataRF(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netState != RS_NET_IDLE)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	unsigned char len = DSTAR_FRAME_LENGTH_BYTES + 1U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("D-Star, overflow in the D-Star RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CDStarControl::writeQueueEOTRF()
{
	if (m_netState != RS_NET_IDLE)
		return;

	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	unsigned char len = 1U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("D-Star, overflow in the D-Star RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	unsigned char data = TAG_EOT;
	m_queue.addData(&data, len);
}

void CDStarControl::writeQueueHeaderNet(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired())
		return;

	unsigned char len = DSTAR_HEADER_LENGTH_BYTES + 1U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("D-Star, overflow in the D-Star RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CDStarControl::writeQueueDataNet(const unsigned char *data)
{
	assert(data != NULL);

	if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired())
		return;

	unsigned char len = DSTAR_FRAME_LENGTH_BYTES + 1U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("D-Star, overflow in the D-Star RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CDStarControl::writeQueueEOTNet()
{
	if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired())
		return;

	unsigned char len = 1U;

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("D-Star, overflow in the D-Star RF queue");
		return;
	}

	m_queue.addData(&len, 1U);

	unsigned char data = TAG_EOT;
	m_queue.addData(&data, len);
}

void CDStarControl::writeNetworkHeaderRF(const unsigned char* data)
{
	assert(data != NULL);

	if (m_network == NULL)
		return;

	// Don't send to the network if the timeout has expired
	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	m_network->writeHeader(data + 1U, DSTAR_HEADER_LENGTH_BYTES, m_netState != RS_NET_IDLE);
}

void CDStarControl::writeNetworkDataRF(const unsigned char* data, unsigned int errors, bool end)
{
	assert(data != NULL);

	if (m_network == NULL)
		return;

	// Don't send to the network if the timeout has expired
	if (m_rfTimeoutTimer.isRunning() && m_rfTimeoutTimer.hasExpired())
		return;

	m_network->writeData(data + 1U, DSTAR_FRAME_LENGTH_BYTES, errors, end, m_netState != RS_NET_IDLE);
}

bool CDStarControl::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "DStar_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("DSTAR", 1U, 4U, m_fp);

	return true;
}

bool CDStarControl::writeFile(const unsigned char* data, unsigned int length)
{
	if (m_fp == NULL)
		return false;

	::fwrite(data, 1U, length, m_fp);

	return true;
}

void CDStarControl::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}

void CDStarControl::insertSilence(const unsigned char* data, unsigned char seqNo)
{
	assert(data != NULL);

	// Check to see if we have any spaces to fill?
	unsigned char seq = (m_netN + 1U) % 21U;
	if (seq == seqNo) {
		// Just copy the data, nothing else to do here
		::memcpy(m_lastFrame, data, DSTAR_FRAME_LENGTH_BYTES + 1U);
		return;
	}

	unsigned int oldSeqNo = (m_netN + 1U) % 21U;
	unsigned int newSeqNo = seqNo;

	unsigned int count;
	if (newSeqNo > oldSeqNo)
		count = newSeqNo - oldSeqNo;
	else
		count = (21U + newSeqNo) - oldSeqNo;

	if (count < 10U)
		insertSilence(count);

	::memcpy(m_lastFrame, data, DSTAR_FRAME_LENGTH_BYTES + 1U);
}

void CDStarControl::insertSilence(unsigned int count)
{
	unsigned char n = (m_netN + 1U) % 21U;

	for (unsigned int i = 0U; i < count; i++) {
		if (i < 3U) {
			writeQueueDataNet(m_lastFrame);
		} else {
			if (n == 0U)
				writeQueueDataNet(DSTAR_NULL_FRAME_SYNC_BYTES);
			else
				writeQueueDataNet(DSTAR_NULL_FRAME_DATA_BYTES);
		}

		m_netN = n;

		m_netFrames++;
		m_netLost++;

		n = (n + 1U) % 21U;
	}

	LogMessage("D-Star, inserted %u audio frames", count);
}

void CDStarControl::blankDTMF(unsigned char* data) const
{
	assert(data != NULL);

	// DTMF begins with these byte values
	if ((data[0] & DSTAR_DTMF_MASK[0]) == DSTAR_DTMF_SIG[0] && (data[1] & DSTAR_DTMF_MASK[1]) == DSTAR_DTMF_SIG[1] &&
		(data[2] & DSTAR_DTMF_MASK[2]) == DSTAR_DTMF_SIG[2] && (data[3] & DSTAR_DTMF_MASK[3]) == DSTAR_DTMF_SIG[3] &&
		(data[4] & DSTAR_DTMF_MASK[4]) == DSTAR_DTMF_SIG[4] && (data[5] & DSTAR_DTMF_MASK[5]) == DSTAR_DTMF_SIG[5] &&
		(data[6] & DSTAR_DTMF_MASK[6]) == DSTAR_DTMF_SIG[6] && (data[7] & DSTAR_DTMF_MASK[7]) == DSTAR_DTMF_SIG[7] &&
		(data[8] & DSTAR_DTMF_MASK[8]) == DSTAR_DTMF_SIG[8])
		::memcpy(data, DSTAR_NULL_AMBE_DATA_BYTES, DSTAR_VOICE_FRAME_LENGTH_BYTES);
}

void CDStarControl::sendAck()
{
	m_rfTimeoutTimer.stop();

	unsigned char user[DSTAR_LONG_CALLSIGN_LENGTH];
	m_rfHeader.getMyCall1(user);

	CDStarHeader header;
	header.setUnavailable(true);
	header.setMyCall1(m_callsign);
	header.setYourCall(user);
	header.setRPTCall1(m_gateway);
	header.setRPTCall2(m_callsign);

	unsigned char data[DSTAR_HEADER_LENGTH_BYTES + 1U];
	header.get(data + 1U);
	data[0U] = TAG_HEADER;

	writeQueueHeaderRF(data);

	writeQueueDataRF(DSTAR_NULL_FRAME_SYNC_BYTES);

	LINK_STATUS status = LS_NONE;
	unsigned char reflector[DSTAR_LONG_CALLSIGN_LENGTH];
	if (m_network != NULL)
		m_network->getStatus(status, reflector);

	char text[40U];
	if (status == LS_LINKED_DEXTRA || status == LS_LINKED_DPLUS || status == LS_LINKED_DCS || status == LS_LINKED_CCS || status == LS_LINKED_LOOPBACK)
		::sprintf(text, "%-8.8s  BER: %.1f%%         ", reflector, float(m_rfErrs * 100U) / float(m_rfBits));
	else
		::sprintf(text, "BER: %.1f%%                 ", float(m_rfErrs * 100U) / float(m_rfBits));
	m_slowData.setText(text);

	::memcpy(data, DSTAR_NULL_FRAME_DATA_BYTES, DSTAR_FRAME_LENGTH_BYTES + 1U);

	for (unsigned int i = 0U; i < 19U; i++) {
		m_slowData.get(data + 1U + DSTAR_VOICE_FRAME_LENGTH_BYTES);
		writeQueueDataRF(data);
	}

	writeQueueEOTRF();
}
