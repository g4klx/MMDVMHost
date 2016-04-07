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

#if !defined(NEXTION_H)
#define	NEXTION_H

#include "Display.h"
#include "SerialController.h"

#include <string>

class CNextion : public IDisplay
{
public:
  CNextion(const std::string& port, unsigned int brightness);
  virtual ~CNextion();

  virtual bool open();

  virtual void setIdle(const char* callsign);

  virtual void setError(const char* text);
  virtual void setLockout();

  virtual void setDStar();
  virtual void writeDStar(const char* my1, const char* my2, const char* your);
  virtual void clearDStar();

  virtual void setDMR();
  virtual void writeDMR(unsigned int slotNo, unsigned int srdId, bool group, unsigned int dstId, const char* type);
  virtual void clearDMR(unsigned int slotNo);

  virtual void setFusion();
  virtual void writeFusion(const char* source, const char* dest);
  virtual void clearFusion();

  virtual void close();

private:
  CSerialController m_serial;
  unsigned int      m_brightness;

  void sendCommand(const char* command);
};

#endif
