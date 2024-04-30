/*
 *   Copyright (C) 2024 by Jonathan Naylor G4KLX
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

#include "FIR.h"

#include <cstddef>
#include <cassert>

CFIR::CFIR(const float* taps, unsigned int count) :
m_line(NULL),
m_taps(NULL),
m_count(count),
m_index(0U)
{
    assert(taps != NULL);
    assert(count > 0U);

    m_line = new float[count];
    m_taps = new float[count];

    for (unsigned int i = 0U; i < count; i++) {
        m_line[i] = 0.0F;
        m_taps[i] = taps[i];
    }
}

CFIR::~CFIR()
{
    delete[] m_line;
    delete[] m_taps;
}

float CFIR::filter(float sample)
{
    m_line[m_index] = sample;

    m_index++;
    if (m_index >= m_count)
        m_index = 0U;

    float out = 0.0F;

    unsigned int index = m_index;

    for (unsigned int i = 0U; i < m_count; i++) {
        if (index > 0U)
            index--;
        else
            index = m_count - 1U;

        out += m_taps[i] * m_line[index];
    }

    return out;
}

void CFIR::reset()
{
    for (unsigned int i = 0U; i < m_count; i++)
        m_line[i] = 0.0F;

    m_index = 0U;
}
