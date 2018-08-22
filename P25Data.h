/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Bryan Biedenkapp <gatekeep@gmail.com>
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

#if !defined(P25Data_H)
#define  P25Data_H

#include "RS241213.h"
#include "P25Trellis.h"

class CP25Data {
public:
	CP25Data();
	~CP25Data();

	void encodeHeader(unsigned char* data);

	bool decodeLDU1(const unsigned char* data);
	void encodeLDU1(unsigned char* data);

	void encodeLDU2(unsigned char* data);

	bool decodeTSDU(const unsigned char* data);
	void encodeTSDU(unsigned char* data);

	void setMI(const unsigned char* mi);
	void getMI(unsigned char* mi) const;

	void setMFId(unsigned char id);
	unsigned char getMFId() const;

	void setAlgId(unsigned char id);
	unsigned char getAlgId() const;

	void setKId(unsigned int id);
	unsigned int getKId() const;

	void setEmergency(bool on);
	bool getEmergency() const;

	void setSrcId(unsigned int Id);
	unsigned int getSrcId() const;

	void setLCF(unsigned char lcf);
	unsigned char getLCF() const;

	void setDstId(unsigned int id);
	unsigned int getDstId() const;

	void setServiceType(unsigned char type);
	unsigned char getServiceType() const;

	void reset();

private:
	unsigned char* m_mi;
	unsigned char  m_mfId;
	unsigned char  m_algId;
	unsigned int   m_kId;
	unsigned char  m_lcf;
	bool           m_emergency;
	unsigned int   m_srcId;
	unsigned int   m_dstId;
	unsigned char  m_serviceType;
	CRS241213      m_rs241213;
	CP25Trellis    m_trellis;

	void decodeLDUHamming(const unsigned char* raw, unsigned char* data);
	void encodeLDUHamming(unsigned char* data, const unsigned char* raw);
};

#endif
