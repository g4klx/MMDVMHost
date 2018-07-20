/*
*	Copyright (C) 2018 Jonathan Naylor, G4KLX
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

#include "POCSAGControl.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

// #define	DUMP_POCSAG

const struct BCD {
	char     m_c;
	uint32_t m_bcd[5U];
} BCD_VALUES[] = {
	{ '0', {0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U}},
	{ '1', {0x40000000U, 0x04000000U, 0x00400000U, 0x00040000U, 0x00004000U}},
	{ '2', {0x20000000U, 0x02000000U, 0x00200000U, 0x00020000U, 0x00002000U}},
	{ '3', {0x60000000U, 0x06000000U, 0x00600000U, 0x00060000U, 0x00006000U}},
	{ '4', {0x10000000U, 0x01000000U, 0x00100000U, 0x00010000U, 0x00001000U}},
	{ '5', {0x50000000U, 0x05000000U, 0x00500000U, 0x00050000U, 0x00005000U}},
	{ '6', {0x30000000U, 0x03000000U, 0x00300000U, 0x00030000U, 0x00003000U}},
	{ '7', {0x70000000U, 0x07000000U, 0x00700000U, 0x00070000U, 0x00007000U}},
	{ '8', {0x08000000U, 0x00800000U, 0x00080000U, 0x00008000U, 0x00000800U}},
	{ '9', {0x48000000U, 0x04800000U, 0x00480000U, 0x00048000U, 0x00004800U}},
	{ 'U', {0x68000000U, 0x06800000U, 0x00680000U, 0x00068000U, 0x00006800U}},
	{ ' ', {0x18000000U, 0x01800000U, 0x00180000U, 0x00018000U, 0x00001800U}},
	{ '-', {0x58000000U, 0x05800000U, 0x00580000U, 0x00058000U, 0x00005800U}},
	{ ')', {0x38000000U, 0x03800000U, 0x00380000U, 0x00038000U, 0x00003800U}},
	{ '(', {0x78000000U, 0x07800000U, 0x00780000U, 0x00078000U, 0x00007800U}},
	{ 0,   {0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U}}
};

const uint32_t BCD_SPACES[] = {0x19999800U, 0x01999800U, 0x00199800U, 0x00019800U, 0x00001800U};

const uint32_t DATA_MASK[] = {	           0x40000000U, 0x20000000U, 0x10000000U,
			      0x08000000U, 0x04000000U, 0x02000000U, 0x01000000U,
			      0x00800000U, 0x00400000U, 0x00200000U, 0x00100000U,
			      0x00080000U, 0x00040000U, 0x00020000U, 0x00010000U,
			      0x00008000U, 0x00004000U, 0x00002000U, 0x00001000U,
			      0x00000800U};

const unsigned char FUNCTIONAL_NUMERIC      = 0U;
const unsigned char FUNCTIONAL_ALERT1       = 1U;
const unsigned char FUNCTIONAL_ALERT2       = 2U;
const unsigned char FUNCTIONAL_ALPHANUMERIC = 3U;

CPOCSAGControl::CPOCSAGControl(CPOCSAGNetwork* network, CDisplay* display) :
m_network(network),
m_display(display),
m_queue(5000U, "POCSAG Control"),
m_frames(0U),
m_count(0U),
m_output(),
m_buffer(),
m_ric(0U),
m_text(),
m_state(PS_NONE),
m_fp(NULL)
{
	assert(display != NULL);
}

CPOCSAGControl::~CPOCSAGControl()
{
	m_output.clear();
	m_buffer.clear();
}

unsigned int CPOCSAGControl::readModem(unsigned char* data)
{
	assert(data != NULL);

	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

bool CPOCSAGControl::processData()
{
	assert(m_network != NULL);

	unsigned char data[300U];
	unsigned int length = m_network->read(data);
	if (length == 0U)
		return false;

	m_ric = 0U;
	m_ric |= (data[0U] << 16) & 0x00FF0000U;
	m_ric |= (data[1U] << 8)  & 0x0000FF00U;
	m_ric |= (data[2U] << 0)  & 0x000000FFU;

	unsigned char functional = data[3U];

	m_buffer.clear();
	addAddress(functional);

	switch (functional) {
		case FUNCTIONAL_ALPHANUMERIC:
			m_text = std::string((char*)(data + 4U), length - 4U);
			LogDebug("Message to %07u, func Alphanumeric: \"%s\"", m_ric, m_text.c_str());
			packASCII();
			break;
		case FUNCTIONAL_NUMERIC:
			m_text = std::string((char*)(data + 4U), length - 4U);
			LogDebug("Message to %07u, func Numeric: \"%s\"", m_ric, m_text.c_str());
			packNumeric();
			break;
		case FUNCTIONAL_ALERT1:
			m_text.clear();
			LogDebug("Message to %07u, func Alert 1", m_ric);
			break;
		case FUNCTIONAL_ALERT2:
			m_text = std::string((char*)(data + 4U), length - 4U);
			LogDebug("Message to %07u, func Alert 2: \"%s\"", m_ric, m_text.c_str());
			packASCII();
			break;
		default:
			break;
	}

	// Ensure data is an even number of words
	if ((m_buffer.size() % 2U) == 1U)
		m_buffer.push_back(POCSAG_IDLE_WORD);

	return true;
}

void CPOCSAGControl::clock(unsigned int ms)
{
	if (m_state == PS_NONE) {
		bool ret = processData();
		if (!ret)
			return;

		m_display->writePOCSAG(m_ric, m_text);
		m_state  = PS_WAITING;
		m_frames = 0U;
		m_count  = 1U;

#if defined(DUMP_POCSAG)
		openFile();
#endif
	}

	m_output.clear();
	m_output.push_back(POCSAG_SYNC_WORD);

	for (unsigned int i = 0U; i < POCSAG_FRAME_ADDRESSES; i++) {
		if (m_state == PS_WAITING) {
			if (i == (m_ric % POCSAG_FRAME_ADDRESSES)) {
				uint32_t w1 = m_buffer.front();
				m_buffer.pop_front();
				uint32_t w2 = m_buffer.front();
				m_buffer.pop_front();

				m_output.push_back(w1);
				m_output.push_back(w2);

				m_state = PS_SENDING;
			} else {
				m_output.push_back(POCSAG_IDLE_WORD);
				m_output.push_back(POCSAG_IDLE_WORD);
			}
		} else if (m_state == PS_SENDING) {
			if (m_buffer.empty()) {
				m_output.push_back(POCSAG_IDLE_WORD);
				m_output.push_back(POCSAG_IDLE_WORD);

				bool ret = processData();
				if (ret) {
					m_display->writePOCSAG(m_ric, m_text);
					m_state = PS_WAITING;
					m_count++;
				} else {
					m_state = PS_ENDING;
				}
			} else {
				uint32_t w1 = m_buffer.front();
				m_buffer.pop_front();
				uint32_t w2 = m_buffer.front();
				m_buffer.pop_front();

				m_output.push_back(w1);
				m_output.push_back(w2);
			}
		} else {		// PS_ENDING
			m_output.push_back(POCSAG_IDLE_WORD);
			m_output.push_back(POCSAG_IDLE_WORD);
		}
	}

	writeQueue();
	m_frames++;

	if (m_state == PS_ENDING) {
		LogMessage("POCSAG, transmitted %u frame(s) of data from %u message(s)", m_frames, m_count);
		m_display->clearPOCSAG();
		m_state = PS_NONE;

#if defined(DUMP_POCSAG)
		closeFile();
#endif
	}
}

void CPOCSAGControl::addAddress(unsigned char functional)
{
	uint32_t word = 0x00000000U;

	switch (functional) {
		case FUNCTIONAL_ALPHANUMERIC:
			word = 0x00001800U;
			break;
		case FUNCTIONAL_ALERT1:
			word = 0x00000800U;
			break;
		case FUNCTIONAL_ALERT2:
			word = 0x00001000U;
			break;
		case FUNCTIONAL_NUMERIC:
		default:
			break;
	}

	word |= (m_ric / POCSAG_FRAME_ADDRESSES) << 13;

	addBCHAndParity(word);

	m_buffer.push_back(word);
}

void CPOCSAGControl::packASCII()
{
	const unsigned char MASK = 0x01U;

	uint32_t word = 0x80000000U;
	unsigned int n = 0U;

	for (std::string::const_iterator it = m_text.cbegin(); it != m_text.cend(); ++it) {
		unsigned char c = *it;
		for (unsigned int j = 0U; j < 7U; j++, c >>= 1) {
			bool b = (c & MASK) == MASK;
			if (b)
				word |= DATA_MASK[n];
			n++;

			if (n == 20U) {
				addBCHAndParity(word);
				m_buffer.push_back(word);
				word = 0x80000000U;
				n = 0U;
			}
		}
	}

	if (n > 0U) {
		addBCHAndParity(word);
		m_buffer.push_back(word);
	}
}

void CPOCSAGControl::packNumeric()
{
	uint32_t word = 0x80000000U;
	unsigned int n = 0U;

	for (std::string::const_iterator it = m_text.cbegin(); it != m_text.cend(); ++it) {
		char c = *it;

		const BCD* bcd = NULL;
		for (unsigned int i = 0U; BCD_VALUES[i].m_c != 0; i++) {
			if (BCD_VALUES[i].m_c == c) {
				bcd = BCD_VALUES + i;
				break;
			}
		}

		if (bcd != NULL) {
			word |= bcd->m_bcd[n];
			n++;

			if (n == 5U) {
				addBCHAndParity(word); 
				m_buffer.push_back(word); 
				word = 0x80000000U; 
				n = 0U; 
			}
		}
	}

	// Pack the remainder of the word with BCD spaces.
	if (n != 0U) {
		word |= BCD_SPACES[n];

		addBCHAndParity(word); 
		m_buffer.push_back(word); 
	}
}

void CPOCSAGControl::addBCHAndParity(uint32_t& word) const
{
	uint32_t temp = word;

	for (unsigned int i = 0U; i < 21U; i++, temp <<= 1) {
		if (temp & 0x80000000U)
			temp ^= 0xED200000U;
	}

	word |= (temp >> 21);

	temp = word;

	unsigned int parity = 0U;
	for (unsigned int i = 0U; i < 32U; i++, temp <<= 1) {
		if (temp & 0x80000000U)
			parity++;
	}

	if ((parity % 2U) == 1U)
		word |= 0x00000001U;
}

void CPOCSAGControl::writeQueue()
{
	// Convert 32-bit words to bytes
	unsigned char data[POCSAG_FRAME_LENGTH_BYTES];
	unsigned char len = 0U;
	for (std::deque<uint32_t>::const_iterator it = m_output.cbegin(); it != m_output.cend(); ++it) {
		uint32_t word = *it;

		data[len++] = word >> 24;
		data[len++] = word >> 16;
		data[len++] = word >> 8;
		data[len++] = word >> 0;
	}

	m_output.clear();

#if defined(DUMP_POCSAG)
	writeFile(data);
#endif

	CUtils::dump(1U, "Data to MMDVM", data, len);

	assert(len == POCSAG_FRAME_LENGTH_BYTES);

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("POCSAG, overflow in the POCSAG RF queue");
		return;
	}

	m_queue.addData(&len, 1U);
	m_queue.addData(data, len);
}

bool CPOCSAGControl::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "POCSAG_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("POCSAG", 1U, 6U, m_fp);

	return true;
}

bool CPOCSAGControl::writeFile(const unsigned char* data)
{
	if (m_fp == NULL)
		return false;

	::fwrite(data, 1U, POCSAG_FRAME_LENGTH_BYTES, m_fp);

	return true;
}

void CPOCSAGControl::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}
