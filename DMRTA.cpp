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

CDMRTA::CDMRTA() :
m_TA(),
m_buf(),
m_bufOffset(0)
{
}

CDMRTA::~CDMRTA() {
}

bool CDMRTA::add(const unsigned char* data, unsigned int len) {
    assert(data);

    if (m_bufOffset + len >= sizeof(m_buf)) {
        // buffer overflow
        reset();
        return false;
    }

    ::memcpy(m_buf + m_bufOffset, data, len);
    m_bufOffset += len;

    decodeTA();
    return true;
}

const unsigned char* CDMRTA::get() {
    return (unsigned char*)m_TA;
}

void CDMRTA::reset() {
    ::memset(m_TA, 0, sizeof(m_TA));
    ::memset(m_buf, 0, sizeof(m_buf));
    m_bufOffset = 0;
}

void CDMRTA::decodeTA() {
    unsigned char *b;
    unsigned char c;
    int j;
    unsigned int i,t1,t2, TAsize, TAformat;

    unsigned char* talkerAlias = m_buf;

    TAformat=(talkerAlias[0]>>6U) & 0x03U;
    TAsize = (talkerAlias[0]>>1U) & 0x1FU;
    ::strncpy(m_TA, "(could not decode)", sizeof(m_TA));

    switch (TAformat) {
	case 0U:		// 7 bit
		::memset(m_TA, 0, sizeof(m_TA));
		b=&talkerAlias[0];
		t1=0; t2=0; c=0;
		for (i=0; (i<32U)&&(t2<TAsize); i++) {
		    for (j=7U;j>=0;j--) {
			    c = (c<<1U) | (b[i] >> j);
			    if (++t1==7U) { 
                    if (i>0) {
                        m_TA[t2++]=c & 0x7FU; 
                    }
                    t1=0;
                    c=0; 
                }
		    }
		}
        m_TA[TAsize]=0;
		break;
	case 1U:		// ISO 8 bit
	case 2U:		// UTF8
		::strncpy(m_TA,(char*)talkerAlias+1U, sizeof(m_TA));
		break;
	case 3U:		// UTF16 poor man's conversion
		t2=0;
		::memset (&m_TA,0,sizeof(m_TA));
		for(i=0; (i<15)&&(t2<TAsize); i++) {
		    if (talkerAlias[2U*i+1U]==0)
			    m_TA[t2++] = talkerAlias[2U*i+2U];
            else
                m_TA[t2++] = '?';
		}
		m_TA[TAsize]=0;
		break;
    }

    LogMessage("DMR Talker Alias (Data Format %u, Received %u/%u char): '%s'", TAformat, ::strlen(m_TA), TAsize, m_TA);
    if (::strlen(m_TA)>TAsize) { 
        if (strlen(m_TA)<29U)
            strcat(m_TA," ?");
        else
            strcpy(m_TA+28U," ?");
    }
}