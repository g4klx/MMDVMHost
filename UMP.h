/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#if !defined(UMP_H)
#define	UMP_H

#include "SerialController.h"
#include "SerialPort.h"

#include <string>

class CUMP : public ISerialPort
{
public:
	CUMP(const std::string& port);
	virtual ~CUMP();

	virtual bool open();

	bool setMode(unsigned char mode);

	bool setTX(bool on);

	bool setCD(bool on);

	bool getLockout() const;

	virtual int read(unsigned char* buffer, unsigned int length);

	virtual int write(const unsigned char* buffer, unsigned int length);

	void clock(unsigned int ms);

	virtual void close();

private:
	CSerialController m_serial;
	bool              m_open;
	unsigned char*    m_buffer;
	unsigned int      m_length;
	unsigned int      m_offset;
	bool              m_lockout;
	unsigned char     m_mode;
	bool              m_tx;
	bool              m_cd;
};

#endif
