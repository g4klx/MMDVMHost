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

#if !defined(OLED_H)
#define	OLED_H

#define OLED_STATUSBAR 0
#define OLED_LINE1 16
#define OLED_LINE2 26 
#define OLED_LINE3 36
#define OLED_LINE4 46

#include "Display.h"
#include "Defines.h"

#include <string>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

static unsigned char logo_glcd_bmp[] =
  { 0b00101011, 0b11010100,
    0b01010111, 0b11101010,
    0b01010111, 0b11101010,
    0b00101011, 0b11010100,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000000, 0b00000000 };

//DMR 48x16 px
static unsigned char logo_dmr_bmp[] =
  { 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111,
    0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000001,
    0b10111111, 0b11111000, 0b01111000, 0b00011110, 0b01111111, 0b11100001,
    0b10111111, 0b11111110, 0b01111100, 0b00111110, 0b01100000, 0b00011001,
    0b10110000, 0b00001110, 0b01100110, 0b01100110, 0b01100000, 0b00011001,
    0b10110000, 0b00000110, 0b01100011, 0b11000110, 0b01100000, 0b00011001,
    0b10110000, 0b00000110, 0b01100001, 0b10000110, 0b01100000, 0b00011001,
    0b10110000, 0b00000110, 0b01100000, 0b00000110, 0b01111111, 0b11111001,
    0b10110000, 0b00000110, 0b01100000, 0b00000110, 0b01111000, 0b00000001,
    0b10110000, 0b00000110, 0b01100000, 0b00000110, 0b01101100, 0b00000001,
    0b10110000, 0b00000110, 0b01100000, 0b00000110, 0b01100110, 0b00000001,
    0b10110000, 0b00001110, 0b01100000, 0b00000110, 0b01100011, 0b00000001,
    0b10111111, 0b11111110, 0b01100000, 0b00000110, 0b01100001, 0b10000001,
    0b10011111, 0b11111000, 0b01100000, 0b00000110, 0b01100000, 0b11000001,
    0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000001,
    0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111
  };

class COLED : public CDisplay 
{
public:
  COLED(unsigned char displayType, unsigned char displayBrighness, unsigned char displayInvert);
  virtual ~COLED();

  virtual bool open();

  virtual void setIdleInt();

  virtual void setErrorInt(const char* text);
  virtual void setLockoutInt();

  virtual void writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
  virtual void clearDStarInt();

  virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
  virtual void clearDMRInt(unsigned int slotNo);

  virtual void writeFusionInt(const char* source, const char* dest, const char* type, const char* origin);
  virtual void clearFusionInt();

	virtual void writeP25Int(const char* source, bool group, unsigned int dest, const char* type);
	virtual void clearP25Int();

  virtual void close();

private:
  const char* m_slot1_state;
  const char* m_slot2_state;
  unsigned char     m_mode;
  unsigned char     m_displayType;
  unsigned char     m_displayBrightness;
  unsigned char     m_displayInvert;


  ArduiPi_OLED display;
  void OLED_statusbar();
};

#endif
