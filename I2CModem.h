/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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

#ifndef	I2CMODEM_H
#define	I2CMODEM_H

#include "I2CPort.h"
#include "Modem.h"

class CI2CModem : public II2CPort {
public:
	CI2CModem(IModem* modem);
	virtual ~CI2CModem();

	virtual bool open(unsigned char displayType);

	virtual void setDataMode();

	virtual void sendCommand(uint8_t c0, uint8_t c1, uint8_t c2);
	virtual void sendCommand(uint8_t c0, uint8_t c1);
	virtual void sendCommand(uint8_t c);

	virtual void writeData(const uint8_t* data, uint16_t length);
	virtual void writeData(uint8_t c);

	virtual void close();

private:
	IModem* m_modem;
};

#endif
