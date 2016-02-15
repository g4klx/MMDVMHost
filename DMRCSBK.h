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

#if !defined(DMRCSBK_H)
#define DMRCSBK_H

#include "DMRDefines.h"

enum CSBKO {
	CSBKO_NONE     = 0x00,
	CSBKO_UUVREQ   = 0x04,
	CSBKO_UUANSRSP = 0x05,
	CSBKO_CTCSBK   = 0x07,
	CSBKO_NACKRSP  = 0x26,
	CSBKO_BSDWNACT = 0x38,
	CSBKO_PRECCSBK = 0x3D
};

class CDMRCSBK
{
public:
	CDMRCSBK(const unsigned char* bytes);
	~CDMRCSBK();

	bool isValid() const;

	// Generic fields
	CSBKO         getCSBKO() const;
	unsigned char getFID() const;

	// For BS Dwn Act
	unsigned int  getBSId() const;

	unsigned int  getSrcId() const;
	unsigned int  getDstId() const;

private:
	CSBKO         m_CSBKO;
	unsigned char m_FID;
	unsigned int  m_bsId;
	unsigned int  m_srcId;
	unsigned int  m_dstId;
	bool          m_valid;
};

#endif
