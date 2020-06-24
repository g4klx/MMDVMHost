/*
 *   Copyright (C) 2016,2017,2019 by Jonathan Naylor G4KLX
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

#if !defined(YSFFICH_H)
#define  YSFFICH_H

class CYSFFICH {
public:
	CYSFFICH(const CYSFFICH& fich);
	CYSFFICH();
	~CYSFFICH();

	bool decode(const unsigned char* bytes);

	void encode(unsigned char* bytes);

	unsigned char getFI() const;
	unsigned char getCM() const;
	unsigned char getBN() const;
	unsigned char getBT() const;
	unsigned char getFN() const;
	unsigned char getFT() const;
	unsigned char getDT() const;
	unsigned char getMR() const;
	bool getDev() const;
	unsigned char getDGId() const;

	void setFI(unsigned char fi);
	void setFN(unsigned char fn);
	void setFT(unsigned char ft);
	void setMR(unsigned char mr);
	void setVoIP(bool set);
	void setDev(bool set);
	void setDGId(unsigned char id);

	CYSFFICH& operator=(const CYSFFICH& fich);

private:
	unsigned char* m_fich;
};

#endif
