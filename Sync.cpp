/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "Sync.h"

#include "DStarDefines.h"
#include "DMRDefines.h"
#include "YSFDefines.h"
#include "P25Defines.h"

#include <cstdio>
#include <cassert>
#include <cstring>


void CSync::addDStarSync(unsigned char* data)
{
	assert(data != NULL);

	::memcpy(data + DSTAR_VOICE_FRAME_LENGTH_BYTES, DSTAR_SYNC_BYTES, DSTAR_DATA_FRAME_LENGTH_BYTES);
}

void CSync::addDMRDataSync(unsigned char* data, bool duplex)
{
	assert(data != NULL);

	if (duplex) {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | BS_SOURCED_DATA_SYNC[i];
	} else {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | MS_SOURCED_DATA_SYNC[i];
	}
}

void CSync::addDMRAudioSync(unsigned char* data, bool duplex)
{
	assert(data != NULL);

	if (duplex) {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | BS_SOURCED_AUDIO_SYNC[i];
	} else {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | MS_SOURCED_AUDIO_SYNC[i];
	}
}

void CSync::addYSFSync(unsigned char* data)
{
	assert(data != NULL);

	::memcpy(data, YSF_SYNC_BYTES, YSF_SYNC_LENGTH_BYTES);
}

void CSync::addP25Sync(unsigned char* data)
{
	assert(data != NULL);

	::memcpy(data, P25_SYNC_BYTES, P25_SYNC_LENGTH_BYTES);
}
