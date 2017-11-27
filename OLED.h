/*
 *   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
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
#define OLED_LINE5 56


/* OLED for HotSpots
*
* 
*/
#define OLED_LINE6 6



#include "Display.h"
#include "Defines.h"

#include <string>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

class COLED : public CDisplay 
{
public:
  COLED(unsigned char displayType, unsigned char displayBrighness, bool displayInvert, bool displayScroll, bool duplex);
  virtual ~COLED();

  virtual bool open();

  virtual void setIdleInt();

  virtual void setErrorInt(const char* text);
  virtual void setLockoutInt();

  virtual void writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
  virtual void clearDStarInt();

  virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
  virtual void clearDMRInt(unsigned int slotNo);
 virtual void writeDMRTAInt(unsigned int slotNo, unsigned char* talkerAlias, const char* type);

  virtual void writeFusionInt(const char* source, const char* dest, const char* type, const char* origin);
  virtual void clearFusionInt();

  virtual void writeP25Int(const char* source, bool group, unsigned int dest, const char* type);
  virtual void clearP25Int();

  virtual void writeCWInt();
  virtual void clearCWInt();

  virtual void close();

private:
  std::string   m_ipaddress;    
  const char*   m_slot1_state;
  const char*   m_slot2_state;
  unsigned char m_mode;
  unsigned char m_displayType;
  unsigned char m_displayBrightness;
  bool          m_displayInvert;
  bool          m_displayScroll;
  bool          m_duplex;

  ArduiPi_OLED display;
  void OLED_statusbar();
};

#endif
