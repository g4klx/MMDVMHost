/*
 *   Copyright (C) 2016,2017,2018 by Jonathan Naylor G4KLX
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
#include "Defines.h"
#include "SerialPort.h"
#include "Timer.h"

#include <string>

class CNextion : public CDisplay
{
public:
  CNextion(const std::string& callsign, unsigned int dmrid, ISerialPort* serial, unsigned int brightness, bool displayClock, bool utc, unsigned int idleBrightness, unsigned int screenLayout);
  virtual ~CNextion();

  virtual bool open();

  virtual void close();

protected:
  virtual void setIdleInt();
  virtual void setErrorInt(const char* text);
  virtual void setLockoutInt();

  virtual void writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
  virtual void writeDStarRSSIInt(unsigned char rssi);
  virtual void writeDStarBERInt(float ber);
  virtual void clearDStarInt();

  virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
  virtual void writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi);
  virtual void writeDMRTAInt(unsigned int slotNo, unsigned char* talkerAlias, const char* type);

  virtual void writeDMRBERInt(unsigned int slotNo, float ber);
  virtual void clearDMRInt(unsigned int slotNo);

  virtual void writeFusionInt(const char* source, const char* dest, const char* type, const char* origin);
  virtual void writeFusionRSSIInt(unsigned char rssi);
  virtual void writeFusionBERInt(float ber);
  virtual void clearFusionInt();

  virtual void writeP25Int(const char* source, bool group, unsigned int dest, const char* type);
  virtual void writeP25RSSIInt(unsigned char rssi);
  virtual void writeP25BERInt(float ber);
  virtual void clearP25Int();

  virtual void writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type);
  virtual void writeNXDNRSSIInt(unsigned char rssi);
  virtual void writeNXDNBERInt(float ber);
  virtual void clearNXDNInt();

  virtual void writePOCSAGInt(uint32_t ric, const std::string& message);
  virtual void clearPOCSAGInt();

  virtual void writeCWInt();
  virtual void clearCWInt();

  virtual void clockInt(unsigned int ms);

private:
  std::string   m_callsign;
  std::string   m_ipaddress;
  unsigned int  m_dmrid;
  ISerialPort*  m_serial;
  unsigned int  m_brightness;
  unsigned char m_mode;
  bool          m_displayClock;
  bool          m_utc;
  unsigned int  m_idleBrightness;
  unsigned int  m_screenLayout;
  CTimer        m_clockDisplayTimer;
  unsigned int  m_rssiAccum1;
  unsigned int  m_rssiAccum2;
  float         m_berAccum1;
  float         m_berAccum2;
  unsigned int  m_rssiCount1;
  unsigned int  m_rssiCount2;
  unsigned int  m_berCount1;
  unsigned int  m_berCount2;

  void sendCommand(const char* command);
  void sendCommandAction(unsigned int status);
};

#endif
