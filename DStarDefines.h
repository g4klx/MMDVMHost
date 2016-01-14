/*
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
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

#if !defined(DStarDefines_H)
#define	DStarDefines_H

const unsigned int DSTAR_HEADER_LENGTH_BYTES = 41U;
const unsigned int DSTAR_FRAME_LENGTH_BYTES  = 12U;

// Check this
const unsigned char DSTAR_SYNC_BYTES[] = {0x55U, 0x2DU, 0x16U};

#endif
