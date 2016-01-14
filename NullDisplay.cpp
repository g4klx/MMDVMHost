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

CNullDisplay::CNullDisplay()
{
}

CNullDisplay::~CNullDisplay()
{
}

bool CNullDisplay::open()
{
	return true;
}

void CNullDisplay::setIdle()
{
}

void CNullDisplay::setDStar()
{
}

void CNullDisplay::writeDStar(const std::string& call1, const std::string& call2)
{
}

void CNullDisplay::clearDStar()
{
}

void CNullDisplay::setDMR()
{
}

void CNullDisplay::writeDMR(unsigned int slotNo, unsigned int srcId, bool group, unsigned int dstId)
{
}

void CNullDisplay::clearDMR(unsigned int slotNo)
{
}

void CNullDisplay::setFusion()
{
}

void CNullDisplay::writeFusion(const std::string& callsign)
{
}

void CNullDisplay::clearFusion()
{
}

void CNullDisplay::close()
{
}
