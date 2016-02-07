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

#if !defined(DMRSYNC_H)
#define	DMRSYNC_H

enum DMR_SYNC_TYPE {
	DST_BS_AUDIO,
	DST_BS_DATA,

	DST_MS_AUDIO,
	DST_MS_DATA,

	DST_DIRECT_SLOT1_AUDIO,
	DST_DIRECT_SLOT1_DATA,

	DST_DIRECT_SLOT2_AUDIO,
	DST_DIRECT_SLOT2_DATA,

	DST_NONE_
};

class CDMRSync
{
public:
	CDMRSync();
	~CDMRSync();

	DMR_SYNC_TYPE matchDirectSync(const unsigned char* bytes) const;

	DMR_SYNC_TYPE matchMSSync(const unsigned char* bytes) const;

	DMR_SYNC_TYPE matchBSSync(const unsigned char* bytes) const;

	void addSync(unsigned char* data, DMR_SYNC_TYPE type) const;

private:
	unsigned int compareBytes(const unsigned char* bytes1, const unsigned char* bytes2) const;
};

#endif
