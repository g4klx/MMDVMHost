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

#include "I2CPi.h"

#include "bcm2835.h"

CI2CPi::CI2CPi()
{
}

CI2CPi::~CI2CPi()
{
}

bool CI2CPi::open(unsigned char displayType)
{
}

bool CI2CPi::write(const uint8_t* data, uint16_t length)
{
	bcm2835_i2c_write(data, length);

	return true;
}

void CI2CPi::close()
{
	bcm2835_i2c_end();

	// Release Raspberry I/O control
	bcm2835_close();
}
