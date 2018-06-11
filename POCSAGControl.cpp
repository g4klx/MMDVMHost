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

const unsigned char ASCII_EOT = 0x04U;

const unsigned char BIT_MASK_TABLE8[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT8(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE8[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE8[(i)&7])
#define READ_BIT8(p,i)    (p[(i)>>3] & BIT_MASK_TABLE8[(i)&7])

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

	unsigned char data[40U];
	unsigned int length = m_network->read(data);
	if (length == 0U)
		return false;

	m_ric = 0U;
	m_ric |= (data[0U] << 16) & 0x00FF0000U;
	m_ric |= (data[1U] << 8) & 0x0000FF00U;
	m_ric |= (data[2U] << 0) & 0x000000FFU;

	m_text = std::string((char*)(data + 3U), length - 3U);

	m_buffer.clear();
	addAddress();
	packASCII();

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
			uint32_t w1 = m_buffer.front();
			m_buffer.pop_front();
			uint32_t w2 = m_buffer.front();
			m_buffer.pop_front();

			m_output.push_back(w1);
			m_output.push_back(w2);

			if (m_buffer.empty()) {
				bool ret = processData();
				if (ret) {
					m_display->writePOCSAG(m_ric, m_text);
					m_state = PS_WAITING;
					m_count++;
				} else {
					m_state = PS_ENDING;
				}
			}
		} else {		// PS_ENDING
			m_output.push_back(POCSAG_IDLE_WORD);
			m_output.push_back(POCSAG_IDLE_WORD);
		}
	}

	writeQueue();
	m_frames++;

	if (m_state == PS_ENDING) {
		LogMessage("POCSAG, transmitted %u frames of data from %u messages", m_frames, m_count);
		m_display->clearPOCSAG();
		m_state = PS_NONE;
	}
}

void CPOCSAGControl::addAddress()
{
	uint32_t word = 0x00001800U;

	word |= (m_ric / POCSAG_FRAME_ADDRESSES) << 13;

	addBCHAndParity(word);

	m_buffer.push_back(word);
}

void CPOCSAGControl::packASCII()
{
	uint32_t word = 0x80000000U;

	unsigned int n = 30U;
	for (std::string::const_iterator it = m_text.cbegin(); it != m_text.cend(); ++it) {
		unsigned char MASK = 0x40U;
		for (unsigned int j = 0U; j < 7U; j++, MASK >>= 1) {
			bool b = (*it & MASK) == MASK;
			if (b)
				word |= (0x01U << n);
			n--;

			if (n == 10U) {
				addBCHAndParity(word);
				m_buffer.push_back(word);
				word = 0x80000000U;
				n = 30U;
			}
		}
	}

	// Add at least one EOT to the message
	do {
		unsigned char MASK = 0x70U;
		for (unsigned int j = 0U; j < 7U; j++, MASK >>= 1) {
			bool b = (ASCII_EOT & MASK) == MASK;
			if (b)
				word |= (0x01U << n);
			n--;

			if (n == 10U) {
				addBCHAndParity(word);
				m_buffer.push_back(word);
				word = 0x80000000U;
				n = 30U;
			}
		}
	} while (n != 30U);
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
	unsigned int n = 0U;
	unsigned char data[POCSAG_FRAME_LENGTH_BYTES];
	for (unsigned int i = 0U; i < POCSAG_FRAME_LENGTH_WORDS; i++) {
		uint32_t w = m_output.front();
		m_output.pop_front();

		for (unsigned int j = 0U; j < 31U; j++, w <<= 1) {
			bool b = (w & 0x80000000U) == 0x80000000U;
			WRITE_BIT8(data, n, b);
			n++;
		}
	}

	unsigned char len = POCSAG_FRAME_LENGTH_BYTES;

	CUtils::dump(1U, "Data to MMDVM", data, len);

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
