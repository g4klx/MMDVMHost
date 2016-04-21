/*
 *   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#if !defined(HD44780_H)
#define	HD44780_H

#include "Display.h"

#include <string>
#include <vector>

#include <mcp23017.h>

// Defines for the Adafruit Pi LCD interface board
#ifdef ADAFRUIT_DISPLAY
#define AF_BASE         100
#define AF_RED          (AF_BASE + 6)
#define AF_RW           (AF_BASE + 14)
#define MCP23017        0x20
#endif

class CHD44780 : public IDisplay
{
public:
  CHD44780(unsigned int rows, unsigned int cols, const std::string& callsign, unsigned int dmrid, const std::vector<unsigned int>& pins);
  virtual ~CHD44780();

  virtual bool open();

  virtual void setIdle();

  virtual void setError(const char* text);
  virtual void setLockout();

  virtual void writeDStar(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
  virtual void clearDStar();

  virtual void writeDMR(unsigned int slotNo, const char* src, bool group, const char* dst, const char* type);
  virtual void clearDMR(unsigned int slotNo);

  virtual void writeFusion(const char* source, const char* dest);
  virtual void clearFusion();

  virtual void close();

private:
	unsigned int m_rows;
	unsigned int m_cols;
	std::string  m_callsign;
	unsigned int m_dmrid;
	unsigned int m_rb;
	unsigned int m_strb;
	unsigned int m_d0;
	unsigned int m_d1;
	unsigned int m_d2;
	unsigned int m_d3;
	int          m_fd;
	bool         m_dmr;

#ifdef ADAFRUIT_DISPLAY
    void adafruitLCDSetup();
#endif

};

#endif
