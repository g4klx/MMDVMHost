/*
*	Copyright (C) 2016,2018 by Jonathan Naylor, G4KLX
*
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation; version 2 of the License.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*/

#ifndef	P25Trellis_H
#define	P25Trellis_H

class CP25Trellis {
public:
	CP25Trellis();
	~CP25Trellis();

	bool decode34(const unsigned char* data, unsigned char* payload);
	void encode34(const unsigned char* payload, unsigned char* data);

	bool decode12(const unsigned char* data, unsigned char* payload);
	void encode12(const unsigned char* payload, unsigned char* data);

private:
	void deinterleave(const unsigned char* in, signed char* dibits) const;
	void interleave(const signed char* dibits, unsigned char* out) const;
	void dibitsToPoints(const signed char* dibits, unsigned char* points) const;
	void pointsToDibits(const unsigned char* points, signed char* dibits) const;
	void bitsToTribits(const unsigned char* payload, unsigned char* tribits) const;
	void bitsToDibits(const unsigned char* payload, unsigned char* dibits) const;
	void tribitsToBits(const unsigned char* tribits, unsigned char* payload) const;
	void dibitsToBits(const unsigned char* dibits, unsigned char* payload) const;
	bool fixCode34(unsigned char* points, unsigned int failPos, unsigned char* payload) const;
	unsigned int checkCode34(const unsigned char* points, unsigned char* tribits) const;
	bool fixCode12(unsigned char* points, unsigned int failPos, unsigned char* payload) const;
	unsigned int checkCode12(const unsigned char* points, unsigned char* dibits) const;
};

#endif
