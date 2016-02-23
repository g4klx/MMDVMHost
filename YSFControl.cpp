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

#include "YSFControl.h"
#include "YSFFICH.h"
#include "Utils.h"
#include "Sync.h"
#include "Log.h"

#include <cassert>
#include <ctime>

// #define	DUMP_YSF

/*
 * TODO:
 * AMBE FEC reconstruction.
 * Callsign extraction + late entry.
 * Uplink and downlink callsign addition.
 */

CYSFControl::CYSFControl(const std::string& callsign, IDisplay* display, unsigned int timeout, bool duplex, bool parrot) :
m_display(display),
m_duplex(duplex),
m_queue(1000U, "YSF Control"),
m_state(RS_LISTENING),
m_timeoutTimer(1000U, timeout),
m_frames(0U),
m_parrot(NULL),
m_fp(NULL)
{
	assert(display != NULL);

	if (parrot)
		m_parrot = new CYSFParrot(timeout);
}

CYSFControl::~CYSFControl()
{
	delete m_parrot;
}

bool CYSFControl::writeModem(unsigned char *data)
{
	unsigned char type = data[0U];

	if (type == TAG_LOST && m_state == RS_RELAYING_RF_AUDIO) {
		LogMessage("YSF, transmission lost, %.1f seconds", float(m_frames) / 10.0F);

		if (m_parrot != NULL)
			m_parrot->end();

		writeEndOfTransmission();
		return false;
	}

	if (type == TAG_LOST)
		return false;

	bool valid = (data[1U] & YSF_CKSUM_OK) == YSF_CKSUM_OK;
	unsigned char fi = data[1U] & YSF_FI_MASK;
	unsigned char dt = data[1U] & YSF_DT_MASK;

	if (type == TAG_DATA && valid && m_state == RS_LISTENING) {
		m_frames = 0U;
		m_timeoutTimer.start();
		m_display->writeFusion("XXXXXX");
		m_state = RS_RELAYING_RF_AUDIO;
		LogMessage("YSF, received RF header");
#if defined(DUMP_YSF)
		openFile();
#endif
	}

	if (m_state != RS_RELAYING_RF_AUDIO)
		return false;

	if (type == TAG_EOT) {
		CSync::addYSFSync(data + 2U);

		CYSFFICH fich;
		fich.decode(data + 2U);

		m_frames++;

		if (m_duplex) {
			fich.setMR(YSF_MR_BUSY);
			fich.encode(data + 2U);

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;
			writeQueue(data);
		}

		if (m_parrot != NULL) {
			fich.setMR(YSF_MR_NOT_BUSY);
			fich.encode(data + 2U);

			data[0U] = TAG_EOT;
			data[1U] = 0x00U;
			writeParrot(data);
		}

#if defined(DUMP_YSF)
		writeFile(data + 2U);
#endif

		LogMessage("YSF, received RF end of transmission, %.1f seconds", float(m_frames) / 10.0F);
		writeEndOfTransmission();

		return false;
	} else {
		CSync::addYSFSync(data + 2U);

		CYSFFICH fich;
		fich.decode(data + 2U);

		m_frames++;

		if (m_duplex) {
			fich.setMR(YSF_MR_BUSY);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;
			writeQueue(data);
		}

		if (m_parrot != NULL) {
			fich.setMR(YSF_MR_NOT_BUSY);
			fich.encode(data + 2U);

			data[0U] = TAG_DATA;
			data[1U] = 0x00U;
			writeParrot(data);
		}

#if defined(DUMP_YSF)
		writeFile(data + 2U);
#endif
	}

	return true;
}

unsigned int CYSFControl::readModem(unsigned char* data)
{
	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CYSFControl::writeEndOfTransmission()
{
	m_state = RS_LISTENING;

	m_timeoutTimer.stop();

	m_display->clearFusion();

#if defined(DUMP_YSF)
	closeFile();
#endif
}

void CYSFControl::clock(unsigned int ms)
{
	m_timeoutTimer.clock(ms);

	if (m_parrot != NULL) {
		m_parrot->clock(ms);

		unsigned int space = m_queue.freeSpace();
		bool hasData = m_parrot->hasData();

		if (space > (YSF_FRAME_LENGTH_BYTES + 2U) && hasData) {
			unsigned char data[YSF_FRAME_LENGTH_BYTES + 2U];
			m_parrot->read(data);
			writeQueue(data);
		}
	}
}

void CYSFControl::writeQueue(const unsigned char *data)
{
	assert(data != NULL);

	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired())
		return;

	unsigned char len = YSF_FRAME_LENGTH_BYTES + 2U;
	m_queue.addData(&len, 1U);

	m_queue.addData(data, len);
}

void CYSFControl::writeParrot(const unsigned char *data)
{
	assert(data != NULL);

	if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired())
		return;

	m_parrot->write(data);

	if (data[0U] == TAG_EOT)
		m_parrot->end();
}

bool CYSFControl::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "YSF_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("YSF", 1U, 3U, m_fp);

	return true;
}

bool CYSFControl::writeFile(const unsigned char* data)
{
	if (m_fp == NULL)
		return false;

	::fwrite(data, 1U, YSF_FRAME_LENGTH_BYTES, m_fp);

	return true;
}

void CYSFControl::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}
