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

#ifndef	NXDNNetwork_H
#define	NXDNNetwork_H

#include "NXDNDefines.h"

#include <cstdint>

enum NXDN_NETWORK_MESSAGE_TYPE {
	NNMT_VOICE_HEADER,
	NNMT_VOICE_BODY,
	NNMT_VOICE_TRAILER,
	NNMT_DATA_HEADER,
	NNMT_DATA_BODY,
	NNMT_DATA_TRAILER
};

class INXDNNetwork {
public:
	virtual ~INXDNNetwork() = 0;

	virtual bool open() = 0;

	virtual void enable(bool enabled) = 0;

	virtual bool write(const unsigned char* data, NXDN_NETWORK_MESSAGE_TYPE type) = 0;

	virtual bool read(unsigned char* data) = 0;

	virtual void reset() = 0;

	virtual void close() = 0;

	virtual void clock(unsigned int ms) = 0;

private:
};

#endif
