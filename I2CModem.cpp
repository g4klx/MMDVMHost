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

#include "I2CModem.h"

#include <cassert>

CI2CModem::CI2CModem(IModem* modem) :
m_modem(modem)
{
	assert(modem != NULL);
}

CI2CModem::~CI2CModem()
{
}

bool CI2CModem::open()
{
	return true;
}

bool CI2CModem::write(const uint8_t* data, uint16_t length)
{
	return m_modem->writeI2C(data, length);
}

void CI2CModem::close()
{
}
