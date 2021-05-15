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

#define RSSI_S1 141U
#define RSSI_S9 93U

void CSMeter::getSignal(unsigned int rssi, unsigned int & signal, unsigned int & plus)
{
    if (rssi > RSSI_S1) {
        signal = 0;
        plus = 0;
    }
    else if (rssi >= RSSI_S9 && rssi <= RSSI_S1) {
        signal = ((RSSI_S1 - rssi) / 6) + 1;
        plus =0;
    }
    else {
        signal = 9U;
        plus = RSSI_S9 - rssi;
    }
}