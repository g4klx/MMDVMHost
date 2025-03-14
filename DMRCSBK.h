/*
 *   Copyright (C) 2015,2016,2020,2021,2022,2025 by Jonathan Naylor G4KLX
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

enum class CSBKO : unsigned char {
	NONE           = 0x00,
	UUVREQ         = 0x04,
	UUANSRSP       = 0x05,
	CTCSBK         = 0x07,
	CALL_ALERT     = 0x1F,
	CALL_ALERT_ACK = 0x20,
	RADIO_CHECK    = 0x24,
	NACKRSP        = 0x26,
	CALL_EMERGENCY = 0x27,
	BSDWNACT       = 0x38,
	PRECCSBK       = 0x3D
};

class CDMRCSBK
{
public:
	CDMRCSBK();
	~CDMRCSBK();

	bool put(const unsigned char* bytes);

	void get(unsigned char* bytes) const;

	// Generic fields
	CSBKO         getCSBKO() const;
	unsigned char getFID() const;

	// Set/Get the OVCM bit in the supported CSBKs
	bool getOVCM() const;
	void setOVCM(bool ovcm);

	// For BS Dwn Act
	unsigned int  getBSId() const;

	// For Pre
	bool getGI() const;

	unsigned int  getSrcId() const;
	unsigned int  getDstId() const;

	bool          getDataContent() const;
	unsigned char getCBF() const;

	void          setCBF(unsigned char cbf);

private:
	unsigned char* m_data;
	CSBKO          m_CSBKO;
	unsigned char  m_FID;
	bool           m_GI;
	unsigned int   m_bsId;
	unsigned int   m_srcId;
	unsigned int   m_dstId;
	bool           m_dataContent;
	unsigned char  m_CBF;
	bool           m_OVCM;
};

#endif
