/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "CSBK.h"
#include "BPTC19696.h"
#include "CRC.h"
#include "Log.h"		// XXXX

#include "Utils.h"

#include <cstdio>
#include <cassert>

CCSBK::CCSBK(const unsigned char* bytes) :
m_CSBKO(CSBKO_NONE),
m_FID(0x00U),
m_bsId(0U),
m_srcId(0U),
m_valid(false)
{
	assert(bytes != NULL);

	CBPTC19696 bptc;

	unsigned char data[12U];
	bptc.decode(bytes, data);

	m_valid = CCRC::checkCSBK(data);

	m_CSBKO = CSBKO(data[0U] & 0x3FU);
	m_FID   = data[1U];

	if (m_CSBKO == CSBKO_BSDWNACT) {
		m_bsId  = data[4U] << 16 | data[5U] << 8 | data[6U];
		m_srcId = data[7U] << 16 | data[8U] << 8 | data[9U]; 
		CUtils::dump("Download activate CSBK", data, 12U);
	} else {
		CUtils::dump("Unhandled CSBK type", data, 12U);
	}
}

CCSBK::~CCSBK()
{
}

bool CCSBK::isValid() const
{
	return m_valid;
}

CSBKO CCSBK::getCSBKO() const
{
	return m_CSBKO;
}

unsigned char CCSBK::getFID() const
{
	return m_FID;
}

unsigned int CCSBK::getBSId() const
{
	return m_bsId;
}

unsigned int CCSBK::getSrcId() const
{
	return m_srcId;
}
