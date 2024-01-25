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

#if !defined(IIRDIRECTFORM1FILTER_H)
#define	IIRDIRECTFORM1FILTER_H

class CIIRDirectForm1Filter
{
public:
  CIIRDirectForm1Filter(float b0, float b1, float b2, float, float a1, float a2, float additionalGaindB);
  float filter(float sample);
  void reset();

private:
// delay line
  float m_x2; // x[n-2]
  float m_y2; // y[n-2]
  float m_x1; // x[n-1]
  float m_y1; // y[n-1]
  
  // coefficients
  // FIR
  float m_b0;
  float m_b1;
  float m_b2;
  // IIR
  float m_a1;
  float m_a2;

  float m_additionalGainLin;
};


#endif