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

#include "DMRSync.h"
#include "DMRDefines.h"

#include <cstdio>
#include <cassert>

const unsigned int BITS_LOOKUP[] = {0U, 1U, 1U, 2U, 1U, 2U, 2U, 3U, 1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U,
									1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U, 2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U,
									1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U, 2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U,
									2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
									1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U, 2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U,
									2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
									2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
									3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U, 4U, 5U, 5U, 6U, 5U, 6U, 6U, 7U,
									1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U, 2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U,
									2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
									2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
									3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U, 4U, 5U, 5U, 6U, 5U, 6U, 6U, 7U,
									2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
									3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U, 4U, 5U, 5U, 6U, 5U, 6U, 6U, 7U,
									3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U, 4U, 5U, 5U, 6U, 5U, 6U, 6U, 7U,
									4U, 5U, 5U, 6U, 5U, 6U, 6U, 7U, 5U, 6U, 6U, 7U, 6U, 7U, 7U, 8U};

const unsigned int THRESHOLD = 4U;

CDMRSync::CDMRSync()
{
}

CDMRSync::~CDMRSync()
{
}

DMR_SYNC_TYPE CDMRSync::matchDirectSync(const unsigned char* bytes) const
{
	assert(bytes != NULL);

	unsigned int diffs = compareBytes(bytes + 13U, DIRECT_SLOT1_AUDIO_SYNC);
	if (diffs < THRESHOLD)
		return DST_DIRECT_SLOT1_AUDIO;

	diffs = compareBytes(bytes + 13U, DIRECT_SLOT2_AUDIO_SYNC);
	if (diffs < THRESHOLD)
		return DST_DIRECT_SLOT2_AUDIO;

	diffs = compareBytes(bytes + 13U, DIRECT_SLOT1_DATA_SYNC);
	if (diffs < THRESHOLD)
		return DST_DIRECT_SLOT1_DATA;

	diffs = compareBytes(bytes + 13U, DIRECT_SLOT2_DATA_SYNC);
	if (diffs < THRESHOLD)
		return DST_DIRECT_SLOT2_DATA;

	return DST_NONE;
}

DMR_SYNC_TYPE CDMRSync::matchMSSync(const unsigned char* bytes) const
{
	assert(bytes != NULL);

	unsigned int diffs = compareBytes(bytes + 13U, MS_SOURCED_AUDIO_SYNC);
	if (diffs < THRESHOLD)
		return DST_MS_AUDIO;

	diffs = compareBytes(bytes + 13U, MS_SOURCED_DATA_SYNC);
	if (diffs < THRESHOLD)
		return DST_MS_DATA;

	return DST_NONE;
}

DMR_SYNC_TYPE CDMRSync::matchBSSync(const unsigned char* bytes) const
{
	assert(bytes != NULL);

	unsigned int diffs = compareBytes(bytes + 13U, BS_SOURCED_AUDIO_SYNC);
	if (diffs < THRESHOLD)
		return DST_BS_AUDIO;

	diffs = compareBytes(bytes + 13U, BS_SOURCED_DATA_SYNC);
	if (diffs < THRESHOLD)
		return DST_BS_DATA;

	return DST_NONE;
}

void CDMRSync::addSync(unsigned char* data, DMR_SYNC_TYPE type) const
{
	assert(data != NULL);

	const unsigned char* sync = NULL;
	switch (type) {
		case DST_BS_AUDIO:
			sync = BS_SOURCED_AUDIO_SYNC;
			break;
		case DST_BS_DATA:
			sync = BS_SOURCED_DATA_SYNC;
			break;
		case DST_MS_AUDIO:
			sync = MS_SOURCED_AUDIO_SYNC;
			break;
		case DST_MS_DATA:
			sync = MS_SOURCED_DATA_SYNC;
			break;
		case DST_DIRECT_SLOT1_AUDIO:
			sync = DIRECT_SLOT1_AUDIO_SYNC;
			break;
		case DST_DIRECT_SLOT1_DATA:
			sync = DIRECT_SLOT1_DATA_SYNC;
			break;
		case DST_DIRECT_SLOT2_AUDIO:
			sync = DIRECT_SLOT2_AUDIO_SYNC;
			break;
		case DST_DIRECT_SLOT2_DATA:
			sync = DIRECT_SLOT2_DATA_SYNC;
			break;
		default:
			return;
	}

	for (unsigned int i = 0U; i < 7U; i++)
		data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | sync[i];
}

unsigned int CDMRSync::compareBytes(const unsigned char *bytes1, const unsigned char* bytes2) const
{
	assert(bytes1 != NULL);
	assert(bytes2 != NULL);

	unsigned int diffs = 0U;
	for (unsigned int i = 0U; i < 7U; i++) {
		unsigned char b = SYNC_MASK[i] & (bytes1[i] ^ bytes2[i]);
		diffs += BITS_LOOKUP[b];
	}

	return diffs;
}
