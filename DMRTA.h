/*
*	Copyright (C) 2015,2016,2017,2018 Jonathan Naylor, G4KLX
*	Copyright (C) 2018 by Shawn Chain, BG5HHP
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

#ifndef	DMRTA_H
#define	DMRTA_H

class CDMRTA {
public:
    CDMRTA();
    ~CDMRTA();

    bool add(unsigned int blockId, const unsigned char* data, unsigned int len);
    const unsigned char* get();
    void reset();

protected:
    bool decodeTA();

private:
    char            m_TA[32];
    unsigned char   m_buf[32];
};

#endif
