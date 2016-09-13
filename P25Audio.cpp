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

#include "P25Audio.h"
#include "P25Utils.h"

#include <cstdio>
#include <cassert>

CP25Audio::CP25Audio() :
m_fec()
{
}

CP25Audio::~CP25Audio()
{
}

unsigned int CP25Audio::process(unsigned char* data)
{
	assert(data != NULL);

	unsigned int errs = 0U;

	unsigned char imbe[18U];

	CP25Utils::decode(data, imbe, 114U, 262U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 114U, 262U);

	CP25Utils::decode(data, imbe, 262U, 410U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 262U, 410U);

	CP25Utils::decode(data, imbe, 452U, 600U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 452U, 600U);

	CP25Utils::decode(data, imbe, 640U, 788U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 640U, 788U);

	CP25Utils::decode(data, imbe, 830U, 978U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 830U, 978U);

	CP25Utils::decode(data, imbe, 1020U, 1168U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1020U, 1168U);

	CP25Utils::decode(data, imbe, 1208U, 1356U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1208U, 1356U);

	CP25Utils::decode(data, imbe, 1398U, 1546U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1398U, 1546U);

	CP25Utils::decode(data, imbe, 1578U, 1726U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1578U, 1726U);

	return errs;
}
