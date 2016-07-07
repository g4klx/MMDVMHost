/*
*	Copyright (C) 2016 by Jonathan Naylor, G4KLX
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

#ifndef	DMRTrellis_H
#define	DMRTrellis_H

class CDMRTrellis {
public:
	CDMRTrellis();
	~CDMRTrellis();

	void decode(const unsigned char* data, unsigned char* payload);
	void encode(const unsigned char* payload, unsigned char* data);

private:
	void deinterleave(const unsigned char* in, unsigned char* points) const;
	void interleave(const unsigned char* points, unsigned char* out) const;
	void totribits(const unsigned char* payload, unsigned char* tribits) const;
};

#endif
