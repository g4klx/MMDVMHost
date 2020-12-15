/*
 *   Copyright (C) 2019 by SASANO Takayoshi JG1UAA
 *   Copyright (C) 2015,2016,2018,2020 by Jonathan Naylor G4KLX
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

#if !defined(TFTSURENOO_H)
#define TFTSURENOO_H

#include "Display.h"
#include "Defines.h"
#include "SerialPort.h"
#include "UserDBentry.h"

#include <string>

class CTFTSurenoo : public CDisplay
{
public:
  CTFTSurenoo(const std::string& callsign, unsigned int dmrid, ISerialPort* serial, unsigned int brightness, bool duplex);
  virtual ~CTFTSurenoo();

  virtual bool open();

  virtual void close();

protected:
	virtual void setIdleInt();
	virtual void setErrorInt(const char* text);
	virtual void setLockoutInt();
	virtual void setQuitInt();
    virtual void setFMInt();

	virtual void writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
	virtual void clearDStarInt();

	virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
	virtual int writeDMRIntEx(unsigned int slotNo, const class CUserDBentry& src, bool group, const std::string& dst, const char* type);
	virtual void clearDMRInt(unsigned int slotNo);

	virtual void writeFusionInt(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin);
	virtual void clearFusionInt();

	virtual void writeP25Int(const char* source, bool group, unsigned int dest, const char* type);
	virtual void clearP25Int();

	virtual void writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type);
	virtual int writeNXDNIntEx(const class CUserDBentry& source, bool group, unsigned int dest, const char* type);
	virtual void clearNXDNInt();

	virtual void writePOCSAGInt(uint32_t ric, const std::string& message);
	virtual void clearPOCSAGInt();

	virtual void writeCWInt();
	virtual void clearCWInt();

	virtual void clockInt(unsigned int ms);

private:
   std::string   m_callsign;
   unsigned int  m_dmrid;
   ISerialPort*  m_serial;
   unsigned int  m_brightness;
   unsigned char m_mode;
   bool          m_duplex;
   bool          m_refresh;
   CTimer        m_refreshTimer;
   char*         m_lineBuf;
   char          m_temp[128];

  void setLineBuffer(char *buf, const char *text, int maxchar);
  void setModeLine(const char *text);
  void setStatusLine(unsigned int line, const char *text);
  void refreshDisplay(void);

  void lcdReset(void);
  void clearScreen(unsigned char colour);
  void setBackground(unsigned char colour);
  void setRotation(unsigned char rotation);
  void setBrightness(unsigned char brightness);
};

#endif
