/*
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
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

#ifndef EmbeddedLC_H
#define EmbeddedLC_H

#include "LC.h"

enum LC_STATE {
	LCS_NONE,
	LCS_FIRST,
	LCS_SECOND,
	LCS_THIRD
};

class CEmbeddedLC
{
public:
	CEmbeddedLC();
	~CEmbeddedLC();

	CLC* addData(const unsigned char* data, unsigned char lcss);

	void setData(const CLC& lc);
	unsigned int getData(unsigned char* data, unsigned int n) const;

private:
	bool*    m_rawLC;
	LC_STATE m_state;

	CLC* processMultiBlockEmbeddedLC();
	void processSingleBlockEmbeddedLC(const bool* data);
};

#endif

