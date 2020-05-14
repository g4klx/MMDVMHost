/*
 *   Copyright (C) 2015-2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2020 by Geoffrey Merck - F4FXL KC3FRA
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

#include "IIRDirectForm1Filter.h"
#include "math.h"

CIIRDirectForm1Filter::CIIRDirectForm1Filter(float b0, float b1, float b2, float , float a1, float a2, float addtionalGaindB) :
m_x2(0.0F),
m_y2(0.0F),
m_x1(0.0F),
m_y1(0.0F),
m_b0(b0),
m_b1(b1),
m_b2(b2),
m_a1(a1),
m_a2(a2),
m_additionalGainLin(0.0F)
{
  m_additionalGainLin = ::powf(10.0F, addtionalGaindB / 20.0F);
}

float CIIRDirectForm1Filter::filter(float sample)
{
  float output = m_b0 * sample
               + m_b1 * m_x1 
               + m_b2 * m_x2
               - m_a1 * m_y1 
               - m_a2 * m_y2;

  m_x2 = m_x1;
  m_y2 = m_y1;  
  m_x1 = sample;
  m_y1 = output;

  return output * m_additionalGainLin;
}

void CIIRDirectForm1Filter::reset()
{
  m_x1 = 0.0f;
  m_x2 = 0.0f;
  m_y1 = 0.0f;
  m_y2 = 0.0f;
}
