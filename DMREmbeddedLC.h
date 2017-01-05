/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

#ifndef DMREmbeddedLC_H
#define DMREmbeddedLC_H

#include "DMRDefines.h"
#include "DMRLC.h"

enum LC_STATE {
	LCS_NONE,
	LCS_FIRST,
	LCS_SECOND,
	LCS_THIRD
};

class CDMREmbeddedLC
{
public:
	CDMREmbeddedLC(unsigned int slotNo);
	~CDMREmbeddedLC();

	bool addData(const unsigned char* data, unsigned char lcss);
	CDMRLC* getLC() const;

	void setData(const CDMRLC& lc);

	unsigned char getData(unsigned char* data, unsigned char n) const;

	void reset();

private:
	unsigned int m_slotNo;
	bool*        m_raw;
	LC_STATE     m_state;
	bool*        m_data;
	FLCO         m_FLCO;
	bool         m_valid;

	bool processEmbeddedData();
};

#endif
