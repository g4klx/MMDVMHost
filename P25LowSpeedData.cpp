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

#include "P25LowSpeedData.h"
#include "P25Utils.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

void CP25LowSpeedData::process(unsigned char* data)
{
	unsigned char lsd[4U];
	CP25Utils::decode(data, lsd, 1546U, 1578U);

	CUtils::dump(1U, "P25, Low Speed Data", lsd, 4U);
}
