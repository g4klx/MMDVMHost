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

#if !defined(OLED_H)
#define	OLED_H

#define OLED_STATUSBAR 0
#define OLED_LINE1 8 //16
#define OLED_LINE2 18 //26 
#define OLED_LINE3 28 //36
#define OLED_LINE4 37 //46
#define OLED_LINE5 47 //56
#define OLED_LINE6 57

#include "Display.h"
#include "Defines.h"

#include <string>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
#include "NetworkInfo.h"

#pragma pack(1) 

struct BMP_FILEHEADER{
unsigned short  bfType;     //2Bytes, Should be "BM"，for Bitmap reserve.
unsigned int    bfSize;     //4Bytes, Whole file size.
unsigned short  bfReserved1;//2Bytes, Reserved should be 0.
unsigned short  bfReserved2;//2Bytes, Reserved should be 0.
unsigned int    bfOffBits;  //4Bytes, Pixel data  offset.
};

struct BMP_INFOHEADER{
unsigned int    biSize;         //4Bytes, Size of INFOHEADER struct.
unsigned int    biWidth;        //4Bytes, Width for image, in pixels.
int             biHeight;       //4Bytes, Height for image, in pixels.
unsigned short  biPlanes;       //2Bytes, Data planes, should be 1.
unsigned short  biBitCount;     //2Bytes, Data bit count in colors, should be 1 in monochrome.
unsigned int    biCompression;  //4Bytes, 0:No compress, 1:RLE8, 2:RLE4.
unsigned int    biSizeImage;    //4Bytes, Image size in Byte.
unsigned int    biXPelsPerMeter;//4Bytes, Horizontal resolution in pixels/meter.
unsigned int    biYPelsPerMeter;//4Bytes, Vertical resolution in pixels/meter.
unsigned int    biClrUsed;      //4Bytes, 
unsigned int    biClrImportant; //4Bytes, 
};

#pragma pack() 

class COLED : public CDisplay 
{
public:
  COLED(unsigned char displayType, unsigned char displayBrighness, bool displayInvert, bool displayScroll, bool displayRotate, bool slot1Enabled, bool slot2Enabled);
  virtual ~COLED();

  virtual bool open();

  virtual void setIdleInt();

  virtual void setErrorInt(const char* text);
  virtual void setLockoutInt();
  virtual void setQuitInt();
  
  virtual void writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
  virtual void clearDStarInt();

  virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
  virtual void clearDMRInt(unsigned int slotNo);

  virtual void writeFusionInt(const char* source, const char* dest, const char* type, const char* origin);
  virtual void clearFusionInt();

  virtual void writeP25Int(const char* source, bool group, unsigned int dest, const char* type);
  virtual void clearP25Int();

  virtual void writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type);
  virtual void clearNXDNInt();

  virtual void writePOCSAGInt(uint32_t ric, const std::string& message);
  virtual void clearPOCSAGInt();

  virtual void writeCWInt();
  virtual void clearCWInt();

  virtual void close();

  virtual bool readBMPLogo(const char *filename, unsigned char *logo_array);

private:
  const char*   m_slot1_state;
  const char*   m_slot2_state;
  unsigned char m_mode;
  unsigned char m_displayType;
  unsigned char m_displayBrightness;
  bool          m_displayInvert;
  bool          m_displayScroll;
  bool          m_displayRotate;
  bool          m_slot1Enabled;
  bool          m_slot2Enabled;
  std::string   m_ipaddress;
  ArduiPi_OLED  m_display;

  void OLED_statusbar();
};

#endif
