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
 *
 *   7/10/2017 changes by ON7LDS for Nextion ported to OLED by EA5SW
 */

#include "OLED.h"
#include "Log.h"
#include "NetworkInfo.h"
#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>
#include <clocale>

const unsigned char discon [] = {
0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x3C, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x04, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xC4, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0xE3, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xA0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x1B, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x80, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


const unsigned char parrot [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00,
0x46, 0x00, 0x02, 0x00, 0x1F, 0xE0, 0x00, 0x00, 0x42, 0x00, 0x02, 0x00, 0x3B, 0xF0, 0x00, 0x00,
0x4E, 0x00, 0x02, 0x00, 0x71, 0xE8, 0x00, 0x00, 0x70, 0x00, 0x03, 0x80, 0xF5, 0xC0, 0x00, 0x00,
0x47, 0x55, 0x32, 0x00, 0xF1, 0xBC, 0x00, 0x00, 0x41, 0x67, 0x4A, 0x00, 0xFB, 0x3C, 0x00, 0x00,
0x47, 0x44, 0x4A, 0x00, 0xFF, 0x7E, 0x00, 0x00, 0x45, 0x44, 0x4A, 0x00, 0xFF, 0x6E, 0x00, 0x00,
0x47, 0x44, 0x32, 0x00, 0xFF, 0xE6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x82, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xF0, 0x00, 0x00
};




const unsigned char XLX_B [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x07, 0xC1, 0xE7, 0x03, 0xE0, 0xF0, 0x00, 0x3F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x03, 0xC3, 0xC7, 0x01, 0xE1, 0xE0, 0x00, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x01, 0xE3, 0x87, 0x00, 0xF1, 0xC0, 0x01, 0xFF, 0xC0, 0x00, 0xE0, 0x00, 0x00, 0x08, 0x00, 0x01,
0x01, 0xE7, 0x87, 0x00, 0xF3, 0xC0, 0x03, 0xC1, 0xC0, 0x00, 0xE0, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0xE7, 0x07, 0x00, 0x73, 0x80, 0x07, 0x80, 0x01, 0xC3, 0xFC, 0x3C, 0x00, 0x08, 0x00, 0x01,
0x00, 0x7E, 0x07, 0x00, 0x3F, 0x00, 0x07, 0x80, 0x07, 0xF3, 0xFC, 0x7E, 0x00, 0x08, 0x00, 0x01,
0x00, 0x7E, 0x07, 0x00, 0x3F, 0x00, 0x07, 0x00, 0x06, 0x7B, 0xFC, 0xE7, 0x00, 0x08, 0x00, 0x01,
0x00, 0x3C, 0x07, 0x00, 0x1E, 0x00, 0x07, 0x1F, 0xC0, 0x38, 0xE1, 0xC3, 0x80, 0x08, 0x00, 0x01,
0x00, 0x7E, 0x07, 0x00, 0x3F, 0x07, 0xE7, 0x1F, 0xC0, 0xF8, 0xE1, 0xC3, 0x80, 0x08, 0x00, 0x01,
0x00, 0x7E, 0x07, 0x00, 0x3F, 0x07, 0xE7, 0x03, 0xC7, 0xF8, 0xE1, 0xFF, 0x80, 0x08, 0x00, 0x0F,
0x00, 0xE7, 0x07, 0x00, 0x73, 0x87, 0xE7, 0x83, 0xCE, 0x38, 0xE1, 0xFF, 0x80, 0x08, 0x00, 0x0F,
0x01, 0xE7, 0x87, 0x00, 0xF3, 0xC0, 0x03, 0xC3, 0xCE, 0x38, 0xE1, 0xC0, 0x00, 0x08, 0x00, 0x01,
0x01, 0xC3, 0x87, 0xF8, 0xE1, 0xC0, 0x03, 0xFF, 0xCE, 0x78, 0xF1, 0xE0, 0x00, 0x08, 0x00, 0x01,
0x03, 0xC3, 0xC7, 0xF9, 0xE1, 0xE0, 0x01, 0xFF, 0xCF, 0xF8, 0xFC, 0xFF, 0x80, 0x08, 0x00, 0x01,
0x07, 0x81, 0xE7, 0xFB, 0xC0, 0xF0, 0x00, 0x7F, 0x87, 0xB8, 0x7C, 0x7F, 0x80, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF
};




const unsigned char XLX [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41,
0x71, 0xDC, 0x1C, 0x70, 0x00, 0x00, 0x00, 0x41, 0x71, 0xDC, 0x1C, 0x70, 0x3C, 0x00, 0x00, 0x41,
0x3B, 0x9C, 0x0E, 0xE0, 0xFC, 0x02, 0x00, 0x41, 0x1B, 0x1C, 0x06, 0xC0, 0xC0, 0x07, 0x80, 0x41,
0x1E, 0x1C, 0x07, 0x81, 0x9C, 0xF2, 0x3C, 0x41, 0x0E, 0x1C, 0x03, 0x81, 0x9C, 0x32, 0x24, 0x41,
0x1F, 0x1C, 0x07, 0xC1, 0x84, 0xD2, 0x3C, 0x47, 0x3B, 0x1C, 0x0E, 0xC0, 0xEC, 0x92, 0x20, 0x41,
0x33, 0x9C, 0x0C, 0xE0, 0x7C, 0xFB, 0xBC, 0x41, 0x71, 0xDF, 0xDC, 0x70, 0x00, 0x00, 0x00, 0x41,
0x60, 0xDF, 0xD8, 0x70, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F
};

//Logo MMDVM for Idle Screen
static unsigned char logo_glcd_bmp[] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0xF8, 0x03, 0xFC, 0x7F, 0x80, 0x3F, 0xC7, 0xFF, 0xFC, 0xF8, 0x00, 0xF9, 0xFC, 0x01, 0xFE,
0x01, 0xFC, 0x07, 0xFC, 0x7F, 0xC0, 0x7F, 0xC4, 0x00, 0x02, 0x48, 0x00, 0x91, 0xFE, 0x03, 0xFE,
0x03, 0xFC, 0x07, 0xFC, 0x7F, 0xC0, 0x7F, 0xC5, 0xFF, 0xF1, 0x24, 0x01, 0x23, 0xFE, 0x03, 0xFE,
0x03, 0xFE, 0x0F, 0xBC, 0x7B, 0xE0, 0xFB, 0xC5, 0x00, 0x09, 0x24, 0x01, 0x23, 0xDF, 0x07, 0xDE,
0x07, 0xDE, 0x0F, 0x3C, 0x79, 0xE0, 0xF3, 0xC5, 0x00, 0x05, 0x12, 0x02, 0x47, 0xCF, 0x07, 0x9E,
0x07, 0x9F, 0x1F, 0x3C, 0x79, 0xF1, 0xF3, 0xC5, 0x00, 0x05, 0x12, 0x02, 0x47, 0x8F, 0x8F, 0x9E,
0x0F, 0x8F, 0x1E, 0x3C, 0x78, 0xF1, 0xE3, 0xC5, 0x00, 0x05, 0x09, 0x04, 0x8F, 0x87, 0x8F, 0x1E,
0x0F, 0x0F, 0xBE, 0x3C, 0x78, 0xFB, 0xE3, 0xC5, 0x00, 0x05, 0x09, 0x04, 0x8F, 0x07, 0xDF, 0x1E,
0x1F, 0x07, 0xFC, 0x3C, 0x78, 0x7F, 0xC3, 0xC5, 0x00, 0x05, 0x04, 0x89, 0x1F, 0x03, 0xFE, 0x1E,
0x1E, 0x03, 0xFC, 0x3C, 0x78, 0x7F, 0xC3, 0xC5, 0x00, 0x09, 0x04, 0x89, 0x1E, 0x01, 0xFE, 0x1E,
0x3E, 0x03, 0xF8, 0x3C, 0x78, 0x3F, 0x83, 0xC5, 0xFF, 0xF1, 0x02, 0x72, 0x3E, 0x01, 0xFC, 0x1E,
0x3C, 0x01, 0xF0, 0x3C, 0x78, 0x1F, 0x03, 0xC4, 0x00, 0x02, 0x02, 0x02, 0x3C, 0x00, 0xF8, 0x1E,
0x7C, 0x01, 0xF0, 0x3C, 0x78, 0x1F, 0x03, 0xC7, 0xFF, 0xFC, 0x01, 0xFC, 0x7C, 0x00, 0xF8, 0x1E,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//Logo D-Star 128x16 px
static unsigned char logo_dstar_bmp[] =
{
0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x60, 0x03, 0xFF, 0xC0, 0x00, 0x00, 0x1F, 0xF0, 0xFF, 0xFE, 0x07, 0x80, 0x3F, 0xF8,
0x00, 0x00, 0xC0, 0x07, 0xC1, 0xE0, 0x00, 0x00, 0x78, 0x7C, 0xFF, 0xFE, 0x0F, 0xC0, 0x3F, 0xFC,
0x00, 0x01, 0xC0, 0x07, 0x80, 0xF0, 0x00, 0x00, 0xE0, 0x3C, 0x07, 0x80, 0x0F, 0xC0, 0x78, 0x0E,
0x00, 0x03, 0xC0, 0x07, 0x80, 0x70, 0x00, 0x00, 0xE0, 0x38, 0x07, 0x00, 0x1B, 0xC0, 0x78, 0x0E,
0x00, 0x07, 0xC0, 0x07, 0x80, 0x70, 0x00, 0x01, 0xE0, 0x00, 0x07, 0x00, 0x33, 0xC0, 0x70, 0x1E,
0x07, 0xFF, 0xFE, 0x07, 0x00, 0x70, 0x00, 0x01, 0xF8, 0x00, 0x07, 0x00, 0x63, 0xC0, 0x70, 0x3C,
0x01, 0xFF, 0xF8, 0x0F, 0x00, 0x71, 0xFF, 0xE0, 0xFF, 0xF0, 0x0E, 0x00, 0xE1, 0xE0, 0xFF, 0xE0,
0x00, 0x7F, 0xE0, 0x0F, 0x00, 0x60, 0x00, 0x00, 0x03, 0xF8, 0x0E, 0x00, 0xC1, 0xE0, 0xFF, 0xE0,
0x00, 0x3F, 0x80, 0x0E, 0x00, 0xE0, 0x00, 0x00, 0x00, 0xF0, 0x0E, 0x01, 0xFF, 0xE0, 0xE0, 0x70,
0x00, 0x7F, 0x00, 0x1E, 0x00, 0xE0, 0x00, 0x03, 0x80, 0x70, 0x0C, 0x03, 0xFC, 0xE0, 0xE0, 0x30,
0x00, 0xFF, 0x00, 0x1E, 0x01, 0xC0, 0x00, 0x07, 0x80, 0xE0, 0x1C, 0x07, 0x00, 0xE1, 0xE0, 0x38,
0x01, 0xEF, 0x00, 0x1C, 0x07, 0x80, 0x00, 0x07, 0xC1, 0xE0, 0x1C, 0x06, 0x00, 0xF1, 0xC0, 0x38,
0x03, 0x87, 0x00, 0x3F, 0xFF, 0x00, 0x00, 0x03, 0xFF, 0x80, 0x1C, 0x0C, 0x00, 0xF3, 0xC0, 0x38,
0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


const unsigned char bm [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x17, 0x70, 0x18, 0x30, 0x00,
0x3B, 0x00, 0x00, 0x17, 0x70, 0x03, 0xB1, 0x80, 0x3B, 0x77, 0x3C, 0xF7, 0x73, 0x9F, 0xFB, 0xDC,
0x3E, 0x71, 0xBE, 0x97, 0x56, 0x5E, 0x36, 0x58, 0x3F, 0x63, 0xA6, 0x96, 0xD7, 0xDF, 0xB7, 0xD8,
0x3B, 0x6D, 0xA6, 0x96, 0xD6, 0x19, 0xB6, 0x18, 0x3F, 0x6F, 0xA6, 0xF6, 0xD7, 0x9F, 0xBB, 0xD8,
0x3E, 0x63, 0xA6, 0xF6, 0xD3, 0x9F, 0xBB, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x8F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x8F, 0xFF, 0xFC,
0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x8F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x8F, 0xFF, 0xFC,
0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x8F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x8F, 0xFF, 0xFC
};


//Logo Fusion 128x16
const unsigned char logo_fusion_bmp [] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0xFC, 0x00, 0x1F, 0xE1, 0xFE, 0x1F, 0xFF, 0xF8, 0x7F, 0xC3, 0xFF, 0xFF, 0x1F, 0xFF, 0xFE,
0x03, 0xFC, 0x00, 0x3F, 0xC3, 0xFC, 0x3F, 0x80, 0x00, 0x7F, 0x87, 0xF0, 0xFF, 0x0F, 0xF1, 0xFF,
0x07, 0xFF, 0xFC, 0x7F, 0x83, 0xF8, 0x7F, 0x80, 0x00, 0xFF, 0x0F, 0xF0, 0xFF, 0x1F, 0xE1, 0xFE,
0x0F, 0xFF, 0xF0, 0x7F, 0x07, 0xF0, 0xFF, 0xFF, 0xC1, 0xFF, 0x1F, 0xE1, 0xFE, 0x3F, 0xC3, 0xFC,
0x0F, 0xF0, 0x00, 0xFE, 0x0F, 0xE0, 0x7F, 0xFF, 0xE1, 0xFE, 0x3F, 0xC3, 0xFC, 0x3F, 0xC3, 0xFC,
0x1F, 0xE0, 0x01, 0xFC, 0x1F, 0xE0, 0x1F, 0xFF, 0xE3, 0xFC, 0x3F, 0xC3, 0xF8, 0x7F, 0x87, 0xF8,
0x3F, 0xC0, 0x03, 0xFC, 0x3F, 0xC0, 0x00, 0x3F, 0xC7, 0xF8, 0x7F, 0x87, 0xF8, 0xFF, 0x0F, 0xF0,
0x7F, 0xC0, 0x03, 0xFF, 0xFF, 0xE0, 0x00, 0x7F, 0x07, 0xF8, 0x7F, 0xCF, 0xE1, 0xFF, 0x1F, 0xF8,
0x7F, 0x80, 0x01, 0xFF, 0xFF, 0xC7, 0xFF, 0xFC, 0x0F, 0xF0, 0x3F, 0xFF, 0x81, 0xFE, 0x1F, 0xF0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


const unsigned char plus [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xE0, 0x00, 0x00,
0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF8, 0x00,
0x00, 0x01, 0xE6, 0x17, 0x80, 0x40, 0x3F, 0x00, 0x00, 0x01, 0xF7, 0x36, 0x40, 0x40, 0x07, 0xC0,
0x00, 0x71, 0xB7, 0x36, 0x4E, 0x55, 0xC0, 0xE0, 0x00, 0xE1, 0xB7, 0xF7, 0x89, 0x55, 0x00, 0xE0,
0x01, 0xC1, 0xB6, 0xD6, 0xCE, 0x54, 0xC0, 0x60, 0x03, 0x81, 0xE6, 0xD6, 0x68, 0x5D, 0xC0, 0xC0,
0x03, 0xC0, 0x00, 0x00, 0x08, 0x00, 0x01, 0x80, 0x01, 0xF0, 0x00, 0x00, 0x08, 0x00, 0x0E, 0x00,
0x00, 0xFE, 0x00, 0x00, 0x07, 0xFE, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
0x00, 0x03, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


const unsigned char dmrplus [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x3F, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x7F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xF0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xE0, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x07, 0xFC, 0x00, 0x00,
0x00, 0x00, 0x00, 0x0F, 0xF8, 0x3C, 0x07, 0x9F, 0xE0, 0x00, 0x38, 0x00, 0x00, 0xFF, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x0E, 0x1C, 0x07, 0x0C, 0x70, 0x00, 0x18, 0x00, 0x00, 0x1F, 0xC0, 0x00,
0x00, 0x00, 0x00, 0x07, 0x06, 0x1E, 0x0F, 0x0C, 0x30, 0x00, 0x18, 0x00, 0x00, 0x07, 0xF0, 0x00,
0x00, 0x00, 0x07, 0x07, 0x07, 0x1E, 0x0F, 0x0C, 0x30, 0x4C, 0x18, 0x00, 0x1C, 0x03, 0xF8, 0x00,
0x00, 0x00, 0x0E, 0x07, 0x03, 0x17, 0x1B, 0x0C, 0x70, 0xFE, 0x18, 0xC6, 0x36, 0x00, 0xFC, 0x00,
0x00, 0x00, 0x38, 0x07, 0x03, 0x13, 0x13, 0x0F, 0xE0, 0xE7, 0x18, 0xC6, 0x62, 0x00, 0x7C, 0x00,
0x00, 0x00, 0xF0, 0x07, 0x03, 0x1B, 0x33, 0x0F, 0xC0, 0xC3, 0x18, 0xC6, 0x70, 0x00, 0x7C, 0x00,
0x00, 0x01, 0xE0, 0x07, 0x03, 0x19, 0xB3, 0x0C, 0xC0, 0xC3, 0x18, 0xC6, 0x3C, 0x00, 0x7C, 0x00,
0x00, 0x03, 0xC0, 0x07, 0x07, 0x19, 0xE3, 0x0C, 0x60, 0xC3, 0x18, 0xC6, 0x0E, 0x00, 0x7C, 0x00,
0x00, 0x07, 0xC0, 0x07, 0x06, 0x18, 0xC3, 0x0C, 0x30, 0xC3, 0x18, 0xC6, 0x46, 0x00, 0x7C, 0x00,
0x00, 0x07, 0xC0, 0x07, 0x1C, 0x18, 0xC3, 0x0C, 0x38, 0xE6, 0x18, 0xDE, 0x62, 0x00, 0xF8, 0x00,
0x00, 0x07, 0xC0, 0x0F, 0xF8, 0x3C, 0xCF, 0x9F, 0x1E, 0xFC, 0x3C, 0x77, 0x7E, 0x00, 0xF0, 0x00,
0x00, 0x07, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00,
0x00, 0x07, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00,
0x00, 0x03, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00,
0x00, 0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
0x00, 0x00, 0x7F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x1F, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x03, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};



const unsigned char bm_B [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x07, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3E, 0x0F, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
0x07, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3F, 0x1F, 0x80, 0x07, 0x80, 0x07, 0x00, 0x00, 0x00,
0x07, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3F, 0x1F, 0x80, 0x03, 0x00, 0x07, 0x00, 0x00, 0x00,
0x07, 0x8F, 0x80, 0x00, 0x00, 0x00, 0x07, 0x3F, 0x1F, 0x83, 0x00, 0x07, 0x87, 0x03, 0x80, 0x00,
0x07, 0x87, 0xBF, 0x3F, 0x8F, 0xF0, 0xFF, 0x3F, 0x1F, 0x8F, 0xC7, 0x9F, 0xDF, 0xCF, 0xE3, 0xF0,
0x07, 0xFF, 0x3F, 0x7F, 0xCF, 0xF9, 0xFF, 0x3F, 0xBF, 0x9F, 0xE7, 0xBF, 0xDF, 0xCF, 0xF3, 0xF0,
0x07, 0xFE, 0x3F, 0x61, 0xCF, 0x79, 0xCF, 0x3B, 0xB7, 0x9C, 0x77, 0xBC, 0x07, 0x1C, 0x73, 0xC0,
0x07, 0xFF, 0x3C, 0x1F, 0xCF, 0x39, 0xCF, 0x3B, 0xF7, 0xBF, 0xF7, 0xBF, 0x87, 0x1F, 0xF3, 0x80,
0x07, 0x87, 0xBC, 0x3F, 0xCF, 0x39, 0xCF, 0x3B, 0xF7, 0xBF, 0xE7, 0x9F, 0xE7, 0x1F, 0xF3, 0x80,
0x07, 0x87, 0xBC, 0x71, 0xCF, 0x39, 0xCF, 0x39, 0xE7, 0xBC, 0x07, 0x87, 0xE7, 0x1C, 0x03, 0x80,
0x07, 0xFF, 0xBC, 0x71, 0xCF, 0x39, 0xCF, 0x39, 0xE7, 0x9E, 0x67, 0xB0, 0xE7, 0x1E, 0x73, 0x80,
0x07, 0xFF, 0x3C, 0x7F, 0xCF, 0x39, 0xFF, 0x39, 0xE7, 0x9F, 0xE7, 0xBF, 0xE7, 0xCF, 0xF3, 0x80,
0x07, 0xFE, 0x3C, 0x1F, 0xCF, 0x38, 0xFF, 0x39, 0xE7, 0x8F, 0xC7, 0x9F, 0xC7, 0xCF, 0xE3, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0
};


COLED::COLED(unsigned char displayType, unsigned char displayBrightness, bool displayInvert, bool displayScroll) :

m_ipaddress("(ip unknown)"),
m_displayType(displayType),
m_displayBrightness(displayBrightness),
m_displayInvert(displayInvert),
m_displayScroll(displayScroll)



{
}

COLED::~COLED()
{
}

bool COLED::open()
{

        unsigned char info[100U];
        CNetworkInfo* m_network;


        info[0]=0;
        m_network = new CNetworkInfo;
        m_network->getNetworkInterface(info);
        m_ipaddress = (char*)info;



/*
        setIdle();
*/

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

    display.invertDisplay(m_displayInvert ? 1 : 0);
    if (m_displayBrightness > 0U)
        display.setBrightness(m_displayBrightness);

    // init done
    display.clearDisplay();   // clears the screen  buffer
    display.display();        // display it (clear display)

    OLED_statusbar();
    display.setCursor(0,OLED_LINE1);
    display.print("Startup");
    display.setCursor(0,OLED_LINE5);
    display.printf("If:","%s", m_ipaddress.c_str());
    display.display();



    return true;
}

void COLED::setIdleInt()
{
    m_mode = MODE_IDLE; 

    display.clearDisplay();
    OLED_statusbar();

//    display.setCursor(0,30);
//    display.setTextSize(3);
//    display.print("Idle");

//    display.setTextSize(1);
//    display.startscrolldiagright(0x00,0x0f);  //the MMDVM logo scrolls the whole screen

    display.setCursor(0,OLED_LINE5);
      display.setTextSize(1);
//      display.printf("\n\n");
      display.printf("%s",m_ipaddress.c_str());

// Temperatura

FILE *temperatureFile;
double T;
temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
if (temperatureFile == NULL)
  ; //print some message
fscanf (temperatureFile, "%lf", &T);
T /= 1000;
//printf ("The temperature is %6.3f C.\n", T);

int temp = (int)T;


//printf ("Mitemp:%d\n",temp);


      display.setCursor(25,OLED_LINE3);
      display.setTextSize(2);
      display.printf("Temp:%d",temp);

fclose (temperatureFile);






    display.display();
}

void COLED::setErrorInt(const char* text)
{
    m_mode = MODE_ERROR;

    display.clearDisplay();
    OLED_statusbar();

    display.setCursor(0,OLED_LINE1);
    display.printf("%s\n",text);

    display.display();
}

void COLED::setLockoutInt()
{
    m_mode = MODE_LOCKOUT;

    display.clearDisplay();
    OLED_statusbar();

    display.setCursor(0,30);
    display.setTextSize(3);
    display.print("Lockout");

    display.setTextSize(1);
    display.display();
}

void COLED::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
    m_mode = MODE_DSTAR;

    display.clearDisplay();
    display.fillRect(0,OLED_LINE1,display.width(),display.height(),BLACK); //clear everything beneath logo

    display.setCursor(0,OLED_LINE2);
    display.printf("%s %.8s/%4.4s",type,my1,my2);

    display.setCursor(0,OLED_LINE3);
    display.printf("-> %.8s",your);

    display.setCursor(0,OLED_LINE5);
    display.printf("via %.8s",reflector);

    OLED_statusbar();
    display.display();
}

void COLED::clearDStarInt()
{
    display.fillRect(0,OLED_LINE1, display.width(),display.height(),BLACK); //clear everything beneath the logo

    display.setCursor(40,38);
    display.print("Listening");

    display.display();
}

void COLED::writeDMRInt(unsigned int slotNo,const std::string& src,bool group,const std::string& dst, const char* type)
{

    if (m_mode != MODE_DMR) {

    display.clearDisplay();

        m_mode = MODE_DMR;

OLED_statusbar();
display.display();

        if (slotNo == 1U)
          {
          display.fillRect(0,OLED_LINE2,display.width(),80,BLACK); //20=> clear 2 lines
          display.setCursor(30,OLED_LINE2);
          display.setTextSize(2);
          display.print("DMR RX");
  
          }
        else
          {
          

        }
    }

    if (slotNo == 1U)
      {
      
      }
    else
      {

//CALLSIGN Size 2 

      display.fillRect(0,OLED_LINE2,display.width(),60,BLACK);
      display.setCursor(5,OLED_LINE2);
      display.setTextSize(2);
      display.printf("%s %s",type,src.c_str());

// TG 

 if (strcmp ("6",dst.c_str()) ==0) {
    display.fillRect(0, 0, display.width(), 16, BLACK);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.drawBitmap(0, 0, XLX, 64, 16, WHITE);
      }

 if (strcmp ("4000",dst.c_str()) ==0) {
    display.fillRect(0, 0, display.width(), 16, BLACK);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.drawBitmap(0, 0, discon, 64, 16, WHITE);
      }


 if (strcmp ("9990",dst.c_str()) ==0) {
    display.fillRect(0, 0, display.width(), 16, BLACK);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.drawBitmap(0, 0, parrot, 64, 16, WHITE);
      }

 if (strcmp ("8",dst.c_str()) ==0) {
    display.fillRect(0, 0, display.width(), 16, BLACK);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.drawBitmap(0, 0, plus, 64, 16, WHITE);
      }


//      display.fillRect(0,OLED_LINE3,display.width(),55,BLACK);
      display.setCursor(70,OLED_LINE6);
      display.setTextSize(1);
      display.printf("%s%s", group ? "TG:" : "", dst.c_str());


// ShutDown & Reboot compares

if ((strcmp ("99999",dst.c_str()) ==0))
{
    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(20,OLED_LINE3);
    display.setTextSize(2);
    display.printf("Reboot");
    display.display();
    printf ("Reboot\n");
    system("sudo shutdown -r now");
    delay (1000);

}

else if ((strcmp ("99998",dst.c_str()) ==0))
{
    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(20,OLED_LINE3);
    display.setTextSize(2);
    display.printf("STOP");
    display.display();
    printf ("Shutdown\n");
    system("sudo shutdown -h now");
    delay (1000);

}


else if ((strcmp ("99997",dst.c_str()) ==0))
{
    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(20,OLED_LINE3);
    display.setTextSize(2);
    display.printf("Load +");
    display.display();
    printf ("DmrPlus\n");
    system("mm_plus");
    delay (100);

}

else if ((strcmp ("99996",dst.c_str()) ==0))
{
    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(20,OLED_LINE3);
    display.setTextSize(2);
    display.printf("Load Gate");
    display.display();
    printf ("DMRGateway\n");
    system("mm_gate");
    delay (100);

}

else if ((strcmp ("99995",dst.c_str()) ==0))
{
    display.clearDisplay();
    OLED_statusbar();
    display.setCursor(20,OLED_LINE3);
    display.setTextSize(2);
    display.printf("Load BM");
    display.display();
    printf ("BrandMeister\n");
    system("mm_BM");
    delay (100);

}


      }


//    OLED_statusbar();
    display.display();

}


void COLED::writeDMRTAInt(unsigned int slotNo,  unsigned char* talkerAlias, const char* type)
{

if (m_mode != MODE_DMR) {

    display.clearDisplay();


        m_mode = MODE_DMR;


 }


    char text[40U];

    if (type[0]==' ') {
      if (slotNo == 1U) {
//	    sendCommand("t0.pco=33808");
        } else {
//	    sendCommand("t2.pco=33808");
        }
        return;
    }

    if (slotNo == 1U) {

 } else {
	::sprintf(text, "%s",talkerAlias);
	if (strlen((char*)talkerAlias)>16-4)

//printf ("%s%s\n","TalkerAlias>16:",text);
;

//display.fillRect(0,OLED_LINE4,display.width(),10,BLACK);
display.setCursor(0,OLED_LINE4);
display.printf("%s%s","TA:",text);
display.display();

	if (strlen((char*)talkerAlias)>20-4)

//printf ("%s%s\n","TalkerAlias>20:",text);
;
//display.setCursor(0,OLED_LINE4);
//display.printf("%s",text);


if (strlen((char*)talkerAlias)>24-4)

//printf ("%s%s\n","TalkerAlias:>24",text);
;


//display.setCursor(0,OLED_LINE4);
//display.printf("%s%s","TA:",text);

    }

//  OLED_statusbar();
//  display.display();


}




void COLED::clearDMRInt(unsigned int slotNo)
{
    if (slotNo == 1U)
      {
      
      }
    else
      {
      display.fillRect(0, OLED_LINE1, display.width(), 100, BLACK);
      display.setCursor(30, OLED_LINE2);
      display.setTextSize(2);
      display.print("DMR RX");
      }

 //   OLED_statusbar();
    display.display();
}

void COLED::writeFusionInt(const char* source, const char* dest, const char* type, const char* origin)
{

    m_mode = MODE_YSF;

    display.clearDisplay();
    display.fillRect(0,OLED_LINE1,display.width(),display.height(),BLACK);

    display.setCursor(0,OLED_LINE2);
    display.printf("%s %.10s", type, source);

    display.setCursor(0,OLED_LINE3);
    display.printf("  %.10s", dest);

    OLED_statusbar();
    display.display();
}

void COLED::clearFusionInt()
{
    display.fillRect(0, OLED_LINE1, display.width(), display.height(), BLACK);

    display.setCursor(40,38);
    display.print("Listening");

    display.display();
}

void COLED::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
    m_mode = MODE_P25;

    display.clearDisplay();
    display.fillRect(0, OLED_LINE1, display.width(), display.height(), BLACK);

    display.setCursor(0,OLED_LINE2);
    display.printf("%s %.10s", type, source);

    display.setCursor(0,OLED_LINE3);
    display.printf("  %s%u", group ? "TG" : "", dest);

    OLED_statusbar();
    display.display();
}

void COLED::clearP25Int()
{
    display.fillRect(0, OLED_LINE1, display.width(), display.height(), BLACK);

    display.setCursor(40,38);
    display.print("Listening");

    display.display();
}

void COLED::writeCWInt()
{
    display.clearDisplay();

    display.setCursor(0,30);
    display.setTextSize(3);
    display.print("CW TX");

    display.setTextSize(1);
    display.display();
    display.startscrollright(0x02,0x0f);
}

void COLED::clearCWInt()
{
    display.clearDisplay();

    display.setCursor(0,30);
    display.setTextSize(3);
    display.print("Idle");

    display.setTextSize(1);
    display.display();
    display.startscrollleft(0x02,0x0f);
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
      display.drawBitmap(0, 0, bm, 64, 16, WHITE);
    else if (m_mode == MODE_DSTAR)
      display.drawBitmap(0, 0, logo_dstar_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_YSF)
      display.drawBitmap(0, 0, logo_fusion_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_P25)
      display.print("P25");
    else

// MMDVM LOGO
//      display.drawBitmap(0, 0, logo_glcd_bmp, 128, 16, WHITE);

// DmrPlus LOGO
//      display.drawBitmap(0, 0, dmrplus, 128, 32, WHITE);

// BrandMeister LOGO 
     display.drawBitmap(0, 0, bm_B, 128, 26, WHITE);

// XLX LOGO 
//   display.drawBitmap(0, 0, XLX_B, 128, 26, WHITE);

    if (m_displayScroll)
      //display.startscrollright(0x00,0x02);
      display.startscrollleft(0x00,0x0f); 
}
