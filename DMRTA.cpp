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

#include "DMRTA.h"
#include "Log.h"

#include <cstring>
#include <cassert>

CDMRTA::CDMRTA() :
m_TA(),
m_buf()
{
}

CDMRTA::~CDMRTA()
{
}

bool CDMRTA::add(unsigned int blockId, const unsigned char* data, unsigned int len)
{
    assert(data != NULL);
    if (blockId > 3) {
        // invalid block id
        reset();
        return false;
    }

    unsigned int offset = blockId * 7;

    if (offset + len >= sizeof(m_buf)) {
        // buffer overflow
        reset();
        return false;
    }

    ::memcpy(m_buf + offset, data, len);

    return decodeTA();
}

const unsigned char* CDMRTA::get()
{
    return (unsigned char*)m_TA;
}

void CDMRTA::reset()
{
    ::memset(m_TA, 0, sizeof(m_TA));
    ::memset(m_buf, 0, sizeof(m_buf));
}

bool CDMRTA::decodeTA()
{
    unsigned char *b;
    unsigned char c;
    int j;
    unsigned int i, t1, t2;

    unsigned char* talkerAlias = m_buf;

    unsigned int TAformat = (talkerAlias[0] >> 6U) & 0x03U;
    unsigned int TAsize   = (talkerAlias[0] >> 1U) & 0x1FU;
    ::strcpy(m_TA, "(could not decode)");

    switch (TAformat) {
	case 0U:		// 7 bit
		::memset(m_TA, 0, sizeof(m_TA));
		b = &talkerAlias[0];
		t1 = 0U; t2 = 0U; c = 0U;
		for (i = 0U; (i < 32U) && (t2 < TAsize); i++) {
		    for (j = 7; j >= 0; j--) {
			    c = (c << 1U) | (b[i] >> j);
			    if (++t1 == 7U) { 
                    if (i > 0U)
                        m_TA[t2++] = c & 0x7FU; 

					t1 = 0U;
                    c = 0U;
                }
		    }
		}
        m_TA[TAsize] = 0;
		break;

	case 1U:		// ISO 8 bit
	case 2U:		// UTF8
		::memcpy(m_TA, talkerAlias + 1U, sizeof(m_TA));
		break;

	case 3U:		// UTF16 poor man's conversion
		t2=0;
		::memset(&m_TA, 0, sizeof(m_TA));
		for (i = 0U; (i < 15U) && (t2 < TAsize); i++) {
		    if (talkerAlias[2U * i + 1U] == 0)
			    m_TA[t2++] = talkerAlias[2U * i + 2U];
            else
                m_TA[t2++] = '?';
		}
		m_TA[TAsize] = 0;
		break;
    }

    size_t TAlen = ::strlen(m_TA);
    LogMessage("DMR Talker Alias (Data Format %u, Received %u/%u char): '%s'", TAformat, TAlen, TAsize, m_TA);

	if (TAlen > TAsize) {
        if (TAlen < 29U)
            strcat(m_TA, " ?");
        else
            strcpy(m_TA + 28U, " ?");
    }

    return TAlen >= TAsize;
}
