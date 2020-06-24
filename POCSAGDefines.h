/*
*   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#if !defined(POCSAGDEFINES_H)
#define  POCSAGDEFINES_H

#include <cstdint>

const unsigned int POCSAG_RADIO_SYMBOL_LENGTH = 20U;      // At 24 kHz sample rate

const unsigned int POCSAG_FRAME_LENGTH_WORDS = 17U;
const unsigned int POCSAG_FRAME_LENGTH_BYTES = POCSAG_FRAME_LENGTH_WORDS * sizeof(uint32_t);

const unsigned int POCSAG_FRAME_ADDRESSES = 8U;

const uint32_t POCSAG_SYNC_WORD = 0x7CD215D8U;

const uint32_t POCSAG_IDLE_WORD = 0x7A89C197U;

#endif
