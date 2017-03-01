/*
 *   Copyright (C) 2011-2017 by Jonathan Naylor G4KLX
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

#include "DStarDefines.h"
#include "DMRDefines.h"
#include "YSFDefines.h"
#include "P25Defines.h"
#include "Thread.h"
#include "Modem.h"
#include "Utils.h"
#include "Log.h"

#include <cmath>
#include <cstdio>
#include <cassert>
#include <ctime>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <unistd.h>
#endif

const uint8_t BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])

const uint8_t NOAVEPTR = 99U;

CModem::CModem(const std::string& port, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, int oscOffset, const std::string& samplesDir, bool debug) :
m_samplesDir(samplesDir),
m_debug(debug),
m_rxDStarData(1000U, "Modem RX D-Star"),
m_rxDMRData2(1000U, "Modem RX DMR2"),
m_rxYSFData(1000U, "Modem RX YSF"),
m_rxP25Data(1000U, "Modem RX P25"),
m_dmrTimer(1000U, 0U, 60U),
m_ysfTimer(1000U, 0U, 100U),
m_p25Timer(1000U, 0U, 180U),
m_dmrFP(NULL),
m_ysfFP(NULL),
m_p25FP(NULL),
m_thresholdVal(0),
m_centreVal(0),
m_threshold(),
m_centre(),
m_averagePtr(NOAVEPTR)
{
}

CModem::~CModem()
{
}

void CModem::setRFParams(unsigned int rxFrequency, unsigned int txFrequency)
{
}

void CModem::setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled)
{
}

void CModem::setLevels(unsigned int rxLevel, unsigned int cwIdTXLevel, unsigned int dstarTXLevel, unsigned int dmrTXLevel, unsigned int ysfTXLevel, unsigned int p25TXLevel)
{
}

void CModem::setDMRParams(unsigned int colorCode)
{
}

bool CModem::open()
{
	::LogMessage("Opening the MMDVM Simulator");

	m_dmrTimer.start();
	m_ysfTimer.start();
	m_p25Timer.start();

	return true;
}

void CModem::clock(unsigned int ms)
{
	m_dmrTimer.clock(ms);
	if (m_dmrTimer.hasExpired()) {
		processDMR();
		m_dmrTimer.start();
	}

	m_ysfTimer.clock(ms);
	if (m_ysfTimer.hasExpired()) {
		processYSF();
		m_ysfTimer.start();
	}

	m_p25Timer.clock(ms);
	if (m_p25Timer.hasExpired()) {
		processP25();
		m_p25Timer.start();
	}
}

void CModem::close()
{
	::LogMessage("Closing the MMDVM Simulator");
}

unsigned int CModem::readDStarData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDStarData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDStarData.getData(&len, 1U);
	m_rxDStarData.getData(data, len);

	return len;
}

unsigned int CModem::readDMRData1(unsigned char* data)
{
	return 0U;
}

unsigned int CModem::readDMRData2(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDMRData2.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDMRData2.getData(&len, 1U);
	m_rxDMRData2.getData(data, len);

	return len;
}

unsigned int CModem::readYSFData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxYSFData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxYSFData.getData(&len, 1U);
	m_rxYSFData.getData(data, len);

	return len;
}

unsigned int CModem::readP25Data(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxP25Data.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxP25Data.getData(&len, 1U);
	m_rxP25Data.getData(data, len);

	return len;
}

// To be implemented later if needed
unsigned int CModem::readSerial(unsigned char* data, unsigned int length)
{
	return 0U;
}

bool CModem::hasDStarSpace() const
{
	return true;
}

bool CModem::writeDStarData(const unsigned char* data, unsigned int length)
{
	return true;
}

bool CModem::hasDMRSpace1() const
{
	return true;
}

bool CModem::hasDMRSpace2() const
{
	return true;
}

bool CModem::writeDMRData1(const unsigned char* data, unsigned int length)
{
	return true;
}

bool CModem::writeDMRData2(const unsigned char* data, unsigned int length)
{
	return true;
}

bool CModem::hasYSFSpace() const
{
	return true;
}

bool CModem::writeYSFData(const unsigned char* data, unsigned int length)
{
	return true;
}

bool CModem::hasP25Space() const
{
	return true;
}

bool CModem::writeP25Data(const unsigned char* data, unsigned int length)
{
	return true;
}

bool CModem::writeSerial(const unsigned char* data, unsigned int length)
{
	return false;
}

bool CModem::hasTX() const
{
	return false;
}

bool CModem::hasCD() const
{
	return false;
}

bool CModem::hasLockout() const
{
	return false;
}

bool CModem::hasError() const
{
	return false;
}

HW_TYPE CModem::getHWType() const
{
	return HWT_MMDVM;
}

bool CModem::setMode(unsigned char mode)
{
	return true;
}

bool CModem::sendCWId(const std::string& callsign)
{
	return true;
}

bool CModem::writeDMRStart(bool tx)
{
	return true;
}

bool CModem::writeDMRAbort(unsigned int slotNo)
{
	return true;
}

bool CModem::writeDMRShortLC(const unsigned char* lc)
{
	return true;
}

void CModem::processDMR()
{
  if (m_dmrFP == NULL) {
    if (m_samplesDir.empty())
      m_samplesDir = ".";

    char filename[150U];
#if defined(_WIN32) || defined(_WIN64)
    ::sprintf(filename, "%s\\Samples-DMR.dat", m_samplesDir.c_str());
#else
    ::sprintf(filename, "%s/Samples-DMR.dat", m_samplesDir.c_str());
#endif

    FILE* m_dmrFP = ::fopen(filename, "rb");
    if (m_dmrFP == NULL)
      return;

    m_averagePtr = NOAVEPTR;
  }

  if (feof(m_dmrFP))
    return;

  uint8_t control;
  ::fread(&control, sizeof(uint8_t), 1U, m_dmrFP);

  unsigned char bytes[DMR_FRAME_LENGTH_SYMBOLS * sizeof(int16_t)];
  ::fread(bytes, sizeof(int16_t), DMR_FRAME_LENGTH_SYMBOLS, m_dmrFP);

  int16_t symbols[DMR_FRAME_LENGTH_SYMBOLS];
  for (unsigned int i = 0U; i < DMR_FRAME_LENGTH_SYMBOLS; i++) {
    int16_t sample = (bytes[i * 2U + 0U] << 8) | bytes[i * 2U + 1U];
    symbols[i] = sample - 2048;
  }

  dmrCalculateLevels(symbols);

  unsigned char frame[DMR_FRAME_LENGTH_BYTES];
  dmrSamplesToBits(symbols, frame);

  if (m_debug)
    CUtils::dump(1U, "RX DMR Data", frame, DMR_FRAME_LENGTH_BYTES);

  unsigned char data = DMR_FRAME_LENGTH_BYTES + 2U;
  m_rxDMRData2.addData(&data, 1U);

  data = TAG_DATA;
  m_rxDMRData2.addData(&data, 1U);

  data = control;
  m_rxDMRData2.addData(&data, 1U);

  m_rxDMRData2.addData(frame, DMR_FRAME_LENGTH_BYTES);
}

void CModem::processP25()
{
	if (m_p25FP == NULL) {
		if (m_samplesDir.empty())
			m_samplesDir = ".";

		char filename[150U];
#if defined(_WIN32) || defined(_WIN64)
		::sprintf(filename, "%s\\Samples-P25.dat", m_samplesDir.c_str());
#else
		::sprintf(filename, "%s/Samples-P25.dat", m_samplesDir.c_str());
#endif

		FILE* m_p25FP = ::fopen(filename, "rb");
		if (m_p25FP == NULL)
			return;

		m_averagePtr = NOAVEPTR;
	}

	if (feof(m_p25FP))
		return;

	uint8_t control;
	if (::fread(&control, sizeof(uint8_t), 1U, m_p25FP) != 1) {
		unsigned char data = 1U;
		m_rxP25Data.addData(&data, 1U);

		data = TAG_LOST;
		m_rxP25Data.addData(&data, 1U);
		
		return;
	}

	unsigned char bytes[P25_LDU_FRAME_LENGTH_SYMBOLS * sizeof(int16_t)];
	::fread(bytes, sizeof(int16_t), P25_LDU_FRAME_LENGTH_SYMBOLS, m_p25FP);

	int16_t symbols[P25_LDU_FRAME_LENGTH_SYMBOLS];
	for (unsigned int i = 0U; i < P25_LDU_FRAME_LENGTH_SYMBOLS; i++) {
		int16_t sample = (bytes[i * 2U + 0U] << 8) | bytes[i * 2U + 1U];
		symbols[i] = sample - 2048;
	}

	p25CalculateLevels(symbols);

	unsigned char frame[P25_LDU_FRAME_LENGTH_BYTES];
	p25SamplesToBits(symbols, frame);

	if (m_debug)
		CUtils::dump(1U, "RX P25 LDU", frame, P25_LDU_FRAME_LENGTH_BYTES);

	unsigned char data = P25_LDU_FRAME_LENGTH_BYTES + 2U;
	m_rxP25Data.addData(&data, 1U);

	data = TAG_DATA;
	m_rxP25Data.addData(&data, 1U);

	data = control;
	m_rxP25Data.addData(&data, 1U);

	m_rxP25Data.addData(frame, P25_LDU_FRAME_LENGTH_BYTES);
}

void CModem::processYSF()
{
	if (m_ysfFP == NULL) {
		if (m_samplesDir.empty())
			m_samplesDir = ".";

		char filename[150U];
#if defined(_WIN32) || defined(_WIN64)
		::sprintf(filename, "%s\\Samples-YSF.dat", m_samplesDir.c_str());
#else
		::sprintf(filename, "%s/Samples-YSF.dat", m_samplesDir.c_str());
#endif

		FILE* m_ysfFP = ::fopen(filename, "rb");
		if (m_ysfFP == NULL)
			return;

		m_averagePtr = NOAVEPTR;
	}

	if (feof(m_ysfFP))
		return;

	uint8_t control;
	if (::fread(&control, sizeof(uint8_t), 1U, m_ysfFP) != 1) {
		unsigned char data = 1U;
		m_rxYSFData.addData(&data, 1U);

		data = TAG_LOST;
		m_rxYSFData.addData(&data, 1U);
		
		return;
	}

	unsigned char bytes[YSF_FRAME_LENGTH_SYMBOLS * sizeof(int16_t)];
	::fread(bytes, sizeof(int16_t), YSF_FRAME_LENGTH_SYMBOLS, m_ysfFP);

	int16_t symbols[YSF_FRAME_LENGTH_SYMBOLS];
	for (unsigned int i = 0U; i < YSF_FRAME_LENGTH_SYMBOLS; i++) {
		int16_t sample = (bytes[i * 2U + 0U] << 8) | bytes[i * 2U + 1U];
		symbols[i] = sample - 2048;
	}

	ysfCalculateLevels(symbols);

	unsigned char frame[YSF_FRAME_LENGTH_BYTES];
	ysfSamplesToBits(symbols, frame);

	if (m_debug)
		CUtils::dump(1U, "RX YSF Data", frame, YSF_FRAME_LENGTH_BYTES);

	unsigned char data = YSF_FRAME_LENGTH_BYTES + 2U;
	m_rxP25Data.addData(&data, 1U);

	data = TAG_DATA;
	m_rxP25Data.addData(&data, 1U);

	data = control;
	m_rxP25Data.addData(&data, 1U);

	m_rxP25Data.addData(frame, YSF_FRAME_LENGTH_BYTES);
}

void CModem::p25CalculateLevels(const int16_t* symbols)
{
	int16_t maxPos = -16000;
	int16_t minPos = 16000;
	int16_t maxNeg = 16000;
	int16_t minNeg = -16000;

	for (uint16_t i = 0U; i < P25_LDU_FRAME_LENGTH_SYMBOLS; i++) {
		int16_t sample = symbols[i];

		if (sample > 0) {
			if (sample > maxPos)
				maxPos = sample;
			if (sample < minPos)
				minPos = sample;
		} else {
			if (sample < maxNeg)
				maxNeg = sample;
			if (sample > minNeg)
				minNeg = sample;
		}
	}

	int16_t posThresh = (maxPos + minPos) >> 1;
	int16_t negThresh = (maxNeg + minNeg) >> 1;

	int16_t centre = (posThresh + negThresh) >> 1;

	int16_t threshold = posThresh - centre;

	LogMessage("P25RX: pos/neg/centre/threshold %d/%d/%d/%d", posThresh, negThresh, centre, threshold);

	if (m_averagePtr == NOAVEPTR) {
		for (uint8_t i = 0U; i < 16U; i++) {
			m_centre[i]    = centre;
			m_threshold[i] = threshold;
		}

		m_averagePtr = 0U;
	} else {
		m_centre[m_averagePtr]    = centre;
		m_threshold[m_averagePtr] = threshold;

		m_averagePtr++;
		if (m_averagePtr >= 16U)
			m_averagePtr = 0U;
	}

	m_centreVal    = 0;
	m_thresholdVal = 0;

	for (uint8_t i = 0U; i < 16U; i++) {
		m_centreVal    += m_centre[i];
		m_thresholdVal += m_threshold[i];
	}

	m_centreVal    >>= 4;
	m_thresholdVal >>= 4;
}

void CModem::p25SamplesToBits(const int16_t* symbols, unsigned char* buffer)
{
	unsigned int offset = 0U;
	for (uint16_t i = 0U; i < P25_LDU_FRAME_LENGTH_SYMBOLS; i++) {
		int16_t sample = symbols[i] - m_centreVal;

		if (sample < -m_thresholdVal) {
			WRITE_BIT1(buffer, offset, false);
			offset++;
			WRITE_BIT1(buffer, offset, true);
			offset++;
		} else if (sample < 0) {
			WRITE_BIT1(buffer, offset, false);
			offset++;
			WRITE_BIT1(buffer, offset, false);
			offset++;
		} else if (sample < m_thresholdVal) {
			WRITE_BIT1(buffer, offset, true);
			offset++;
			WRITE_BIT1(buffer, offset, false);
			offset++;
		} else {
			WRITE_BIT1(buffer, offset, true);
			offset++;
			WRITE_BIT1(buffer, offset, true);
			offset++;
		}
	}
}

void CModem::ysfCalculateLevels(const int16_t* symbols)
{
	int16_t maxPos = -16000;
	int16_t minPos = 16000;
	int16_t maxNeg = 16000;
	int16_t minNeg = -16000;

	for (uint16_t i = 0U; i < YSF_FRAME_LENGTH_SYMBOLS; i++) {
		int16_t sample = symbols[i];

		if (sample > 0) {
			if (sample > maxPos)
				maxPos = sample;
			if (sample < minPos)
				minPos = sample;
		}
		else {
			if (sample < maxNeg)
				maxNeg = sample;
			if (sample > minNeg)
				minNeg = sample;
		}
	}

	int16_t posThresh = (maxPos + minPos) >> 1;
	int16_t negThresh = (maxNeg + minNeg) >> 1;

	int16_t centre = (posThresh + negThresh) >> 1;

	int16_t threshold = posThresh - centre;

	LogMessage("YSFRX: pos/neg/centre/threshold %d/%d/%d/%d", posThresh, negThresh, centre, threshold);

	if (m_averagePtr == NOAVEPTR) {
		for (uint8_t i = 0U; i < 16U; i++) {
			m_centre[i]    = centre;
			m_threshold[i] = threshold;
		}

		m_averagePtr = 0U;
	} else {
		m_centre[m_averagePtr]    = centre;
		m_threshold[m_averagePtr] = threshold;

		m_averagePtr++;
		if (m_averagePtr >= 16U)
			m_averagePtr = 0U;
	}

	m_centreVal    = 0;
	m_thresholdVal = 0;

	for (uint8_t i = 0U; i < 16U; i++) {
		m_centreVal    += m_centre[i];
		m_thresholdVal += m_threshold[i];
	}

	m_centreVal    >>= 4;
	m_thresholdVal >>= 4;
}

void CModem::ysfSamplesToBits(const int16_t* symbols, unsigned char* buffer)
{
	unsigned int offset = 0U;
	for (uint16_t i = 0U; i < YSF_FRAME_LENGTH_SYMBOLS; i++) {
		int16_t sample = symbols[i] - m_centreVal;

		if (sample < -m_thresholdVal) {
			WRITE_BIT1(buffer, offset, false);
			offset++;
			WRITE_BIT1(buffer, offset, true);
			offset++;
		} else if (sample < 0) {
			WRITE_BIT1(buffer, offset, false);
			offset++;
			WRITE_BIT1(buffer, offset, false);
			offset++;
		} else if (sample < m_thresholdVal) {
			WRITE_BIT1(buffer, offset, true);
			offset++;
			WRITE_BIT1(buffer, offset, false);
			offset++;
		} else {
			WRITE_BIT1(buffer, offset, true);
			offset++;
			WRITE_BIT1(buffer, offset, true);
			offset++;
		}
	}
}


void CModem::dmrCalculateLevels(const int16_t* symbols)
{
  int16_t maxPos = -16000;
  int16_t minPos = 16000;
  int16_t maxNeg = 16000;
  int16_t minNeg = -16000;

  for (uint16_t i = 0U; i < DMR_FRAME_LENGTH_SYMBOLS; i++) {
    int16_t sample = symbols[i];

    if (sample > 0) {
      if (sample > maxPos)
        maxPos = sample;
      if (sample < minPos)
        minPos = sample;
    } else {
      if (sample < maxNeg)
        maxNeg = sample;
      if (sample > minNeg)
        minNeg = sample;
    }
  }

  int16_t posThresh = (maxPos + minPos) >> 1;
  int16_t negThresh = (maxNeg + minNeg) >> 1;

  int16_t centre = (posThresh + negThresh) >> 1;

  int16_t threshold = posThresh - centre;

  LogMessage("DMRRX: pos/neg/centre/threshold %d/%d/%d/%d", posThresh, negThresh, centre, threshold);

  if (m_averagePtr == NOAVEPTR) {
    for (uint8_t i = 0U; i < 16U; i++) {
      m_centre[i]    = centre;
      m_threshold[i] = threshold;
    }

    m_averagePtr = 0U;
  } else {
    m_centre[m_averagePtr]    = centre;
    m_threshold[m_averagePtr] = threshold;

    m_averagePtr++;
    if (m_averagePtr >= 16U)
      m_averagePtr = 0U;
  }

  m_centreVal    = 0;
  m_thresholdVal = 0;

  for (uint8_t i = 0U; i < 16U; i++) {
    m_centreVal    += m_centre[i];
    m_thresholdVal += m_threshold[i];
  }

  m_centreVal >>= 4;
  m_thresholdVal >>= 4;
}

void CModem::dmrSamplesToBits(const int16_t* symbols, unsigned char* buffer)
{
  unsigned int offset = 0U;
  for (uint16_t i = 0U; i < DMR_FRAME_LENGTH_SYMBOLS; i++) {
    int16_t sample = symbols[i] - m_centreVal;

    if (sample < -m_thresholdVal) {
      WRITE_BIT1(buffer, offset, false);
      offset++;
      WRITE_BIT1(buffer, offset, true);
      offset++;
    } else if (sample < 0) {
      WRITE_BIT1(buffer, offset, false);
      offset++;
      WRITE_BIT1(buffer, offset, false);
      offset++;
    } else if (sample < m_thresholdVal) {
      WRITE_BIT1(buffer, offset, true);
      offset++;
      WRITE_BIT1(buffer, offset, false);
      offset++;
    } else {
      WRITE_BIT1(buffer, offset, true);
      offset++;
      WRITE_BIT1(buffer, offset, true);
      offset++;
    }
  }
}
