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

#include "OLED.h"

COLED::COLED(unsigned char displayType, unsigned char displayBrightness, unsigned char displayInvert) :
m_displayType(displayType),
m_displayBrightness(displayBrightness),
m_displayInvert(displayInvert)
{
}

COLED::~COLED()
{
}

bool COLED::open()
{
    // SPI
    if (display.oled_is_spi_proto(m_displayType))
    {
        // SPI change parameters to fit to your LCD
        if ( !display.init(OLED_SPI_DC,OLED_SPI_RESET,OLED_SPI_CS, m_displayType) )
            return false;
    }
    else
    {
        // I2C change parameters to fit to your LCD
        if ( !display.init(OLED_I2C_RESET, m_displayType) )
            return false; 
    }

    display.begin();

    display.invertDisplay(m_displayInvert);
    if (m_displayBrightness > 0)
        display.setBrightness(m_displayBrightness);

    // init done
    display.clearDisplay();   // clears the screen  buffer
    display.display();        // display it (clear display)

    OLED_statusbar();
    display.setCursor(0,OLED_LINE1);
    display.print("Startup");
    display.display();

	return true;
}

void COLED::setIdle()
{
    m_mode = MODE_IDLE; 

    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(0,display.height()/2);
    display.setTextSize(3);
    display.print("Idle");
    display.setTextSize(1);
    display.display();
    display.startscrollright(0x02,0x0f);
}

void COLED::setError(const char* text)
{
    m_mode = MODE_ERROR;

    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(0,OLED_LINE1);
    display.printf("%s\n", text);
    display.display();
}

void COLED::setLockout()
{
    m_mode = MODE_LOCKOUT;

    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(0,OLED_LINE1);
    display.print("Lockout");
    display.display();
}

void COLED::writeDStar(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
    m_mode = MODE_DSTAR;
}

void COLED::clearDStar()
{
    display.fillRect(0, OLED_LINE1, display.width(), 10, BLACK);
    display.setCursor(0,OLED_LINE1);
    display.print("Listening");
    OLED_statusbar();
    display.display();
}

void COLED::writeDMR(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{

    if (m_mode != MODE_DMR) {
        display.clearDisplay(); 
        m_mode = MODE_DMR;
        if (slotNo == 1U)
        {
          display.fillRect(0, OLED_LINE3, display.width(), 10, BLACK);
          display.setCursor(0,OLED_LINE3);
          display.print("2 Listening");
          display.fillRect(0, OLED_LINE4, display.width(), 10, BLACK);
        }
        else
        {
          display.fillRect(0, OLED_LINE1, display.width(), 10, BLACK);
          display.setCursor(0,OLED_LINE1);
          display.print("1 Listening");
          display.fillRect(0, OLED_LINE2, display.width(), 10, BLACK);
        }
    }

    if (slotNo == 1U) {
        display.fillRect(0, OLED_LINE1, display.width(), 10, BLACK);
        display.setCursor(0,OLED_LINE1);
        display.printf("%i %s %s", slotNo, type, src.c_str());
        display.fillRect(0, OLED_LINE2, display.width(), 10, BLACK);
        display.setCursor(0,OLED_LINE2);
        display.printf("%s%s", group ? "TG" : "", dst.c_str());
    } else {
        display.fillRect(0, OLED_LINE3, display.width(), 10, BLACK);
        display.setCursor(0,OLED_LINE3);
        display.printf("%i %s %s", slotNo, type, src.c_str());
        display.fillRect(0, OLED_LINE4, display.width(), 10, BLACK);
        display.setCursor(0,OLED_LINE4);
        display.printf("%s%s", group ? "TG" : "", dst.c_str());
    }
    OLED_statusbar();
    display.display();
}

void COLED::clearDMR(unsigned int slotNo)
{
    OLED_statusbar();
    if (slotNo == 1U)
    {
      display.fillRect(0, OLED_LINE1, display.width(), 10, BLACK);
      display.setCursor(0,OLED_LINE1);
      display.print("1 Listening");
      display.fillRect(0, OLED_LINE2, display.width(), 10, BLACK);
    }
    else
    {
      display.fillRect(0, OLED_LINE3, display.width(), 10, BLACK);
      display.setCursor(0,OLED_LINE3);
      display.print("2 Listening");
      display.fillRect(0, OLED_LINE4, display.width(), 10, BLACK);
    }
    display.display();
}

void COLED::writeFusion(const char* source, const char* dest)
{
    m_mode = MODE_YSF;
}

void COLED::clearFusion()
{
}

void COLED::close()
{
    display.close();
}

void COLED::OLED_statusbar()
{
    display.stopscroll();
    display.fillRect(0, 0, display.width(), 16, BLACK);
    display.setTextColor(WHITE);
	display.setCursor(0,0);
    if (m_mode == MODE_DMR)
      display.drawBitmap(0, 0, logo_dmr_bmp, 48, 16, WHITE);
    else
      display.drawBitmap(0, 0, logo_glcd_bmp, 16, 15, WHITE);
}
