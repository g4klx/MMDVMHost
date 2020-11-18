/*
 *   Copyright (C) 2016,2017,2018,2020 by Jonathan Naylor G4KLX
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

#include "Display.h"
#include "Defines.h"
#include "UserDBentry.h"
#include "I2CPort.h"

#include <string>

class COLED : public CDisplay 
{
public:
	COLED(II2CPort* port, unsigned char displayType, unsigned char displayBrighness, bool displayInvert, bool displayScroll, bool displayRotate, bool displayLogoScreensaver, bool slot1Enabled, bool slot2Enabled);
	virtual ~COLED();

	virtual bool open();

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

	virtual void writeM17Int(const char* source, const char* dest, const char* type);
	virtual void clearM17Int();

	virtual void writePOCSAGInt(uint32_t ric, const std::string& message);
	virtual void clearPOCSAGInt();

	virtual void writeCWInt();
	virtual void clearCWInt();

	virtual void close();

private:
	II2CPort*      m_port;
	unsigned char  m_mode;
	unsigned char  m_displayType;
	unsigned char  m_displayBrightness;
	bool           m_displayInvert;
	bool           m_displayScroll;
	bool           m_displayRotate;
	bool           m_displayLogoScreensaver;
	bool           m_slot1Enabled;
	bool           m_slot2Enabled;
	std::string    m_ipAddress;
	unsigned int   m_passCounter;

	unsigned char* m_oledBuffer;
	uint16_t       m_oledBufferSize;
	uint8_t        m_width;
	uint8_t        m_height;
	bool           m_textWrap;
	uint8_t        m_cursorX;
	uint8_t        m_cursorY;
	uint8_t        m_textSize;
	uint16_t       m_textColor;
	uint16_t       m_textBGColor;
	uint8_t        m_grayH;
	uint8_t        m_grayL;

	void statusbar();
	void clearDisplay();
	void startscrollleft(uint8_t start, uint8_t stop);
	void startscrolldiagleft(uint8_t start, uint8_t stop);
	void stopscroll();
	void setBrightness(uint8_t brightness);
	void invertDisplay(bool invert);
	void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
	void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
	void drawBitmap(uint16_t x, uint16_t y, const uint8_t* bitmap, uint16_t w, uint16_t h, uint16_t color);
	void drawPixel(uint16_t x, uint16_t y, uint16_t color);
	void print(const char* text);
	void display();
	size_t write(uint8_t c);
	void drawChar(uint16_t x, uint16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
	void begin();
};

#endif
