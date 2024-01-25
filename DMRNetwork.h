/*
 *   Copyright (C) 2015,2016,2017,2018,2020,2021 by Jonathan Naylor G4KLX
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

#if !defined(DMRNetwork_H)
#define	DMRNetwork_H

#include "DMRData.h"

#include <string>

class IDMRNetwork
{
public:
	virtual ~IDMRNetwork() = 0;

	virtual void setOptions(const std::string& options) = 0;

	virtual void setConfig(const std::string& callsign, unsigned int rxFrequency, unsigned int txFrequency, unsigned int power, unsigned int colorCode, float latitude, float longitude, int height, const std::string& location, const std::string& description, const std::string& url) = 0;

	virtual bool open() = 0;

	virtual void enable(bool enabled) = 0;

	virtual bool read(CDMRData& data) = 0;

	virtual bool write(const CDMRData& data) = 0;

	virtual bool writeRadioPosition(unsigned int id, const unsigned char* data) = 0;

	virtual bool writeTalkerAlias(unsigned int id, unsigned char type, const unsigned char* data) = 0;

	virtual bool wantsBeacon() = 0;

	virtual void clock(unsigned int ms) = 0;

	virtual bool isConnected() const = 0;

	virtual void close(bool sayGoodbye) = 0;

private: 
};

#endif
