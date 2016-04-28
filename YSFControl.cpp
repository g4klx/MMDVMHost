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

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

// #define	DUMP_YSF

CYSFControl::CYSFControl(const std::string& callsign, IDisplay* display, unsigned int timeout, bool duplex, bool parrot) :
m_display(display),
m_duplex(duplex),
m_queue(2000U, "YSF Control"),
m_state(RS_RF_LISTENING),
m_timeoutTimer(1000U, timeout),
m_interval(),
m_frames(0U),
m_errs(0U),
m_bits(0U),
m_source(NULL),
m_dest(NULL),
m_payload(),
m_parrot(NULL),
m_fp(NULL)
{
	assert(display != NULL);

	if (parrot)
		m_parrot = new CYSFParrot(timeout);

	m_payload.setUplink(callsign);
	m_payload.setDownlink(callsign);

	m_interval.start();
}

CYSFControl::~CYSFControl()
{
	delete m_parrot;
}

bool CYSFControl::writeModem(unsigned char *data)
{
	assert(data != NULL);

	unsigned char type = data[0U];

	if (type == TAG_LOST && m_state == RS_RF_AUDIO) {
		LogMessage("YSF, transmission lost, %.1f seconds, BER: %.1f%%", float(m_frames) / 10.0F, float(m_errs * 100U) / float(m_bits));

		if (m_parrot != NULL)
			m_parrot->end();

		writeEndOfTransmission();
		return false;
	}

	if (type == TAG_LOST)
		return false;

	CYSFFICH fich;
	bool valid = fich.decode(data + 2U);

	if (valid && m_state == RS_RF_LISTENING) {
		unsigned char fi = fich.getFI();
		if (fi == YSF_FI_TERMINATOR)
			return false;

		m_frames = 0U;
		m_errs = 0U;
		m_bits = 1U;
		m_timeoutTimer.start();
		m_payload.reset();
		m_state = RS_RF_AUDIO;
#if defined(DUMP_YSF)
		openFile();
#endif
	}

	if (m_state != RS_RF_AUDIO)
		return false;

	unsigned char orig[YSF_FRAME_LENGTH_BYTES];
	::memcpy(orig, data + 2U, YSF_FRAME_LENGTH_BYTES);

	unsigned char fi = fich.getFI();
	if (valid && fi == YSF_FI_HEADER) {
		CSync::addYSFSync(data + 2U);

		unsigned char fn = fich.getFN();
		unsigned char ft = fich.getFT();
		unsigned char dt = fich.getDT();
		unsigned char cm = fich.getCM();

		fich.encode(data + 2U);

		unsigned int errs = calculateBER(orig, data + 2U, YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES);
		LogDebug("YSF, FI=%u FN=%u FT=%u DT=%u BER=%.1f%%", fi, fn, ft, dt, float(errs) / 2.4F);

		m_errs += errs;
		m_bits += 240U;

		m_frames++;

		// valid = m_payload.processHeaderData(data + 2U);
		valid = false;

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

		if (valid)
			m_source = m_payload.getSource();

		if (cm == YSF_CM_GROUP) {
			m_dest = (unsigned char*)"CQCQCQ    ";
		} else {
			if (valid)
				m_dest = m_payload.getDest();
		}

#if defined(DUMP_YSF)
		writeFile(data + 2U);
#endif

		if (m_source != NULL && m_dest != NULL)
			LogMessage("YSF, received header from %10.10s to %10.10s", m_source, m_dest);
		else if (m_source == NULL && m_dest != NULL)
			LogMessage("YSF, received header from ?????????? to %10.10s", m_dest);
		else if (m_source != NULL && m_dest == NULL)
			LogMessage("YSF, received header from %10.10s to ??????????", m_source);
		else
			LogMessage("YSF, received header from ?????????? to ??????????");
	} else if (valid && fi == YSF_FI_TERMINATOR) {
		CSync::addYSFSync(data + 2U);

		unsigned char fn = fich.getFN();
		unsigned char ft = fich.getFT();
		unsigned char dt = fich.getDT();

		fich.encode(data + 2U);

		unsigned int errs = calculateBER(orig, data + 2U, YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES);
		LogDebug("YSF, FI=%u FN=%u FT=%u DT=%u BER=%.1f%%", fi, fn, ft, dt, float(errs) / 2.4F);

		m_errs += errs;
		m_bits += 240U;

		m_frames++;

		// m_payload.processHeaderData(data + 2U);

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

		LogMessage("YSF, received RF end of transmission, %.1f seconds, BER: %.1f%%", float(m_frames) / 10.0F, float(m_errs * 100U) / float(m_bits));
		writeEndOfTransmission();

		return false;
	} else if (valid) {
		CSync::addYSFSync(data + 2U);

		unsigned char cm = fich.getCM();
		unsigned char fn = fich.getFN();
		unsigned char ft = fich.getFT();
		unsigned char dt = fich.getDT();

		fich.encode(data + 2U);

		unsigned int errs = calculateBER(orig, data + 2U, YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES);
		LogDebug("YSF, FI=%u FN=%u FT=%u DT=%u BER=%.1f%%", fi, fn, ft, dt, float(errs) / 2.4F);

		m_errs += errs;
		m_bits += 240U;

		m_frames++;

		switch (dt) {
		case YSF_DT_VD_MODE1:
			valid = m_payload.processVDMode1Data(data + 2U, fn);
			m_errs += m_payload.processVDMode1Audio(data + 2U);
			m_bits += 235U;
			break;

		case YSF_DT_VD_MODE2:
			valid = m_payload.processVDMode2Data(data + 2U, fn);
			m_errs += m_payload.processVDMode2Audio(data + 2U);
			m_bits += 135U;
			break;

		case YSF_DT_DATA_FR_MODE:
			valid = m_payload.processDataFRModeData(data + 2U, fn);
			break;

		default:		// YSF_DT_VOICE_FR_MODE
			if (fn != 0U || ft != 1U) {
				// The first packet after the header is odd, don't try and regenerate it
				m_errs += m_payload.processVoiceFRModeAudio(data + 2U);
				m_bits += 720U;
			}
			valid = false;
			break;
		}

		bool change = false;

		if (m_dest == NULL) {
			if (cm == YSF_CM_GROUP) {
				m_dest = (unsigned char*)"CQCQCQ    ";
				change = true;
			} else if (valid) {
				m_dest = m_payload.getDest();
				if (m_dest != NULL)
					change = true;
			}
		}

		if (valid && m_source == NULL) {
			m_source = m_payload.getSource();
			if (m_source != NULL)
				change = true;
		}

		if (change) {
			if (m_source != NULL && m_dest != NULL) {
				m_display->writeFusion((char*)m_source, (char*)m_dest);
				LogMessage("YSF, received transmission from %10.10s to %10.10s", m_source, m_dest);
			}
			if (m_source != NULL && m_dest == NULL) {
				m_display->writeFusion((char*)m_source, "??????????");
				LogMessage("YSF, received transmission from %10.10s to ??????????", m_source);
			}
			if (m_source == NULL && m_dest != NULL) {
				m_display->writeFusion("??????????", (char*)m_dest);
				LogMessage("YSF, received transmission from ?????????? to %10.10s", m_dest);
			}
		}

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
	} else {
		CSync::addYSFSync(data + 2U);

		// Only calculate the BER on the sync word
		unsigned int errs = calculateBER(orig, data + 2U, YSF_SYNC_LENGTH_BYTES);
		LogDebug("YSF, invalid FICH, BER=%.1f%%", float(errs) / 0.4F);

		m_errs += errs;
		m_bits += 40U;

		m_frames++;

		if (m_duplex) {
			data[0U] = TAG_DATA;
			data[1U] = 0x00U;
			writeQueue(data);
		}

		if (m_parrot != NULL) {
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
	assert(data != NULL);

	if (m_queue.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_queue.getData(&len, 1U);

	m_queue.getData(data, len);

	return len;
}

void CYSFControl::writeEndOfTransmission()
{
	m_state = RS_RF_LISTENING;

	m_timeoutTimer.stop();

	m_payload.reset();

	m_display->clearFusion();

	// These variables are free'd by YSFPayload
	m_source = NULL;
	m_dest = NULL;

#if defined(DUMP_YSF)
	closeFile();
#endif
}

void CYSFControl::clock()
{
	unsigned int ms = m_interval.elapsed();
	m_interval.start();

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

	unsigned int space = m_queue.freeSpace();
	if (space < (len + 1U)) {
		LogError("YSF, overflow in the System Fusion RF queue");
		return;
	}

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

unsigned int CYSFControl::calculateBER(const unsigned char* orig, const unsigned char *curr, unsigned int length) const
{
	unsigned int errors = 0U;

	for (unsigned int i = 0U; i < length; i++) {
		unsigned char v = orig[i] ^ curr[i];
		while (v != 0U) {
			v &= v - 1U;
			errors++;
		}
	}

	return errors;
}
