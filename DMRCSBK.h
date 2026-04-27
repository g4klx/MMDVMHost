/*
 *   Copyright (C) 2015,2016,2020,2021,2022,2023,2025 by Jonathan Naylor G4KLX
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
#include "Defines.h"

#if defined(USE_DMR)

enum class CSBKO : unsigned char {
	NONE           = 0x00U,
	UUVREQ         = 0x04U,
	UUANSRSP       = 0x05U,
	CTCSBK         = 0x07U,
	CALL_ALERT     = 0x1FU,
	CALL_ALERT_ACK = 0x20U,
	RADIO_CHECK    = 0x24U,
	NACKRSP        = 0x26U,
	CALL_EMERGENCY = 0x27U,
	BSDWNACT       = 0x38U,
	PRECCSBK       = 0x3DU,
	MAINT          = 0x2AU,
	TV_GRANT       = 0x31U,
	PV_GRANT       = 0x30U,
	AHOY           = 0x1CU,
	ACKU           = 0x21U,
	P_CLEAR        = 0x2EU,
	C_BCAST        = 0x28U
};

class CDMRCSBK
{
public:
	CDMRCSBK();
	~CDMRCSBK();

	bool put(const unsigned char* bytes);

	void get(unsigned char* bytes) const;
	void setCSBKData(unsigned char* bytes);

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
	void          setDataType(unsigned char dataType);

	unsigned char getDataType() const;

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
	unsigned char  m_dataType;
};

#endif

#endif

