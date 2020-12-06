/*
 *   Copyright (C) 2005, 2006, 2008, 2009 Free Software Foundation, Inc.
 *   Copyright (C) 2011,2015,2016 by Jonathan Naylor G4KLX
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

#ifndef SHA256_H
#define SHA256_H

#include <cstdint>

enum {
	SHA256_DIGEST_SIZE = 256 / 8
};

class CSHA256 {
public:
	CSHA256();
	~CSHA256();

	/* Starting with the result of former calls of this function (or the
	   initialization function update the context for the next LEN bytes
	   starting at BUFFER.
	   It is necessary that LEN is a multiple of 64!!! */
	void processBlock(const unsigned char* buffer, unsigned int len);

	/* Starting with the result of former calls of this function (or the
	   initialization function update the context for the next LEN bytes
	   starting at BUFFER.
	   It is NOT required that LEN is a multiple of 64.  */
	void processBytes(const unsigned char* buffer, unsigned int len);

	/* Process the remaining bytes in the buffer and put result from CTX
	   in first 32 bytes following RESBUF.  The result is always in little
	   endian byte order, so that a byte-wise output yields to the wanted
	   ASCII representation of the message digest.  */
	unsigned char* finish(unsigned char* resbuf);

	/* Put result from CTX in first 32 bytes following RESBUF.  The result is
	   always in little endian byte order, so that a byte-wise output yields
	   to the wanted ASCII representation of the message digest.  */
	unsigned char* read(unsigned char* resbuf);

	/* Compute SHA256 message digest for LEN bytes beginning at BUFFER.  The
	   result is always in little endian byte order, so that a byte-wise
	   output yields to the wanted ASCII representation of the message
	   digest.  */
	unsigned char* buffer(const unsigned char* buffer, unsigned int len, unsigned char* resblock);

private:
	uint32_t*    m_state;
	uint32_t*    m_total;
	unsigned int m_buflen;
	uint32_t*    m_buffer;

	void init();
	void conclude();
};

#endif
