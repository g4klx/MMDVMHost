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

#include "NullDisplay.h"

CNullDisplay::CNullDisplay() :
CDisplay()
{
}

CNullDisplay::~CNullDisplay()
{
}

bool CNullDisplay::open()
{
	return true;
}

void CNullDisplay::setIdleInt()
{
}

void CNullDisplay::setErrorInt(const char* text)
{
}

void CNullDisplay::setLockoutInt()
{
}

void CNullDisplay::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
}

void CNullDisplay::clearDStarInt()
{
}

void CNullDisplay::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
}

void CNullDisplay::clearDMRInt(unsigned int slotNo)
{
}

void CNullDisplay::writeFusionInt(const char* source, const char* dest)
{
}

void CNullDisplay::clearFusionInt()
{
}

void CNullDisplay::close()
{
}
