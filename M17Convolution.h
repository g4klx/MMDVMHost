/*
 *   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
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

#if !defined(M17Convolution_H)
#define  M17Convolution_H

#include <cstdint>

class CM17Convolution {
public:
	CM17Convolution();
	~CM17Convolution();

	unsigned int decodeLinkSetup(const unsigned char* in, unsigned char* out);
	unsigned int decodeData(const unsigned char* in, unsigned char* out);

	void encodeLinkSetup(const unsigned char* in, unsigned char* out) const;
	void encodeData(const unsigned char* in, unsigned char* out) const;

private:
	uint16_t* m_metrics1;
	uint16_t* m_metrics2;
	uint16_t* m_oldMetrics;
	uint16_t* m_newMetrics;
	uint64_t* m_decisions;
	uint64_t* m_dp;

	void start();
	void decode(uint8_t s0, uint8_t s1);

	unsigned int chainback(unsigned char* out, unsigned int nBits);

	void encode(const unsigned char* in, unsigned char* out, unsigned int nBits) const;
};

#endif
