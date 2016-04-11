/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(TFTSERIAL_H)
#define	TFTSERIAL_H

#include "Display.h"
#include "Defines.h"
#include "SerialController.h"

#include <string>

class CTFTSerial : public IDisplay
{
public:
  CTFTSerial(const char* callsign, unsigned int dmrid, const std::string& port, unsigned int brightness);
  virtual ~CTFTSerial();

  virtual bool open();

  virtual void setIdle();

  virtual void setError(const char* text);
  virtual void setLockout();

  virtual void writeDStar(const char* my1, const char* my2, const char* your);
  virtual void clearDStar();

  virtual void writeDMR(unsigned int slotNo, unsigned int srdId, bool group, unsigned int dstId, const char* type);
  virtual void clearDMR(unsigned int slotNo);

  virtual void writeFusion(const char* source, const char* dest);
  virtual void clearFusion();

  virtual void close();

private:
   const char*       m_callsign;
   unsigned int      m_dmrid;
   CSerialController m_serial;
   unsigned int      m_brightness;
   unsigned char     m_mode;

  void clearScreen();
  void setBackground(unsigned char colour);
  void setForeground(unsigned char colour);
  void setRotation(unsigned char rotation);
  void setFontSize(unsigned char size);
  void gotoBegOfLine();
  void gotoPosText(unsigned char x, unsigned char y);
  void gotoPosPixel(unsigned char x, unsigned char y);
  void drawLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
  void drawBox(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, bool filled);
  void drawCircle(unsigned char x, unsigned char y, unsigned char radius, bool filled);
  void displayBitmap(unsigned char x, unsigned char y, const char* filename);
  void setBrightness(unsigned char brightness);
  void displayText(const char* text);
};

#endif
