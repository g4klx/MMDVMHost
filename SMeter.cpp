/*
 *   Copyright (C) 2015-2021 by Jonathan Naylor G4KLX
 *   Copyright (C) 2021 by Geoffrey Merck F4FXL / KC3FRA
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

#include "SMeter.h"

void CSMeter::getSignal(unsigned int rssi, unsigned int & signal, unsigned int & plus)
{
    signal = 0U;
    plus = 0U;

    if (rssi >= 121) { signal = 1; plus = 0; }
    else if (rssi >= 115) { signal = 2; plus = 0; }
    else if (rssi >= 109) { signal = 3; plus = 0; }
    else if (rssi >= 103) { signal = 4; plus = 0; }
    else if (rssi >= 97)  { signal = 5; plus = 0; }
    else if (rssi >= 91)  { signal = 6; plus = 0; }
    else if (rssi >= 85)  { signal = 7; plus = 0; }
    else if (rssi >= 79)  { signal = 8; plus = 0; }
    else if (rssi >= 73)  { signal = 9; plus = 0; }
    else if (rssi >= 63)  { signal = 9; plus = 10; }
    else if (rssi >= 53)  { signal = 9; plus = 20; }
    else if (rssi >= 43)  { signal = 9; plus = 30; }
    else if (rssi >= 33)  { signal = 9; plus = 40; }
}