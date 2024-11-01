/*
 *   Copyright (C) 2020,2021,2023,2024 by Jonathan Naylor G4KLX
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

#if !defined(FMNetwork_H)
#define	FMNetwork_H

#include <cstdint>
#include <string>

class IFMNetwork {
public:
	virtual ~IFMNetwork() = 0;

	virtual bool open() = 0;

	virtual void enable(bool enabled) = 0;

	virtual bool writeStart() = 0;

	virtual bool writeData(const float* data, unsigned int nSamples) = 0;

	virtual bool writeEnd() = 0;

	virtual unsigned int readData(float* out, unsigned int nOut) = 0;

	virtual void reset() = 0;

	virtual void close() = 0;

	virtual void clock(unsigned int ms) = 0;

	virtual std::string getAddress() = 0;

private:
};

#endif
