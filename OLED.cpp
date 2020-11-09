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

#include "OLED.h"
#include "Log.h"

static bool networkInfoInitialized = false;
static unsigned char passCounter = 0;

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

//Logo DMR 128x16 px
static unsigned char logo_dmr_bmp[] =
{
0x00, 0x01, 0xFF, 0xFF, 0xF8, 0x01, 0xF8, 0x00, 0x00, 0x1F, 0x1F, 0xFF, 0xFF, 0xFC, 0x00, 0x00,
0x00, 0x01, 0xFF, 0xFF, 0xFF, 0x81, 0xFC, 0x00, 0x00, 0x3F, 0x1F, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xE1, 0xFE, 0x00, 0x00, 0xFF, 0x1F, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x0F, 0xF1, 0xFF, 0x80, 0x01, 0xFF, 0x1F, 0x80, 0x00, 0x1F, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x03, 0xF9, 0xFF, 0xC0, 0x03, 0xFF, 0x1F, 0x80, 0x00, 0x0F, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x01, 0xF9, 0xFF, 0xE0, 0x07, 0xFF, 0x1F, 0x80, 0x00, 0x0F, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x01, 0xFD, 0xF3, 0xF0, 0x1F, 0x9F, 0x1F, 0x80, 0x00, 0x1F, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x00, 0xFD, 0xF1, 0xFC, 0x3F, 0x1F, 0x1F, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x00, 0xFD, 0xF0, 0xFE, 0x7E, 0x1F, 0x1F, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x01, 0xFD, 0xF0, 0x7F, 0xFC, 0x1F, 0x1F, 0xFF, 0xFF, 0xFC, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x01, 0xF9, 0xF0, 0x1F, 0xF0, 0x1F, 0x1F, 0x81, 0xFC, 0x00, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x07, 0xF9, 0xF0, 0x0F, 0xE0, 0x1F, 0x1F, 0x80, 0x7F, 0x00, 0x00, 0x00,
0x00, 0x01, 0xF8, 0x00, 0x3F, 0xF1, 0xF0, 0x07, 0xC0, 0x1F, 0x1F, 0x80, 0x3F, 0xC0, 0x00, 0x00,
0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xC1, 0xF0, 0x03, 0x80, 0x1F, 0x1F, 0x80, 0x0F, 0xF0, 0x00, 0x00,
0x00, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xF0, 0x00, 0x00, 0x1F, 0x1F, 0x80, 0x03, 0xFC, 0x00, 0x00,
0x00, 0x01, 0xFF, 0xFF, 0xF0, 0x01, 0xF0, 0x00, 0x00, 0x1F, 0x1F, 0x80, 0x01, 0xFF, 0x00, 0x00
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

//Logo P25 128x16px
const unsigned char logo_P25_bmp [] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0xff, 0xf8, 0x00,
0x01, 0xff, 0xff, 0xff, 0xff, 0x80, 0x3f, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xf8, 0x00,
0x01, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xff, 0xf8, 0x3f, 0xff, 0x01, 0xff, 0xff, 0xff, 0xf8, 0x00,
0x01, 0xff, 0xc0, 0x00, 0x7f, 0xf1, 0xff, 0xc0, 0x07, 0xff, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00,
0x01, 0xff, 0xc0, 0x00, 0x3f, 0xf3, 0xff, 0x80, 0x03, 0xff, 0x81, 0xff, 0x00, 0x00, 0x00, 0x00,
0x01, 0xff, 0xc0, 0x00, 0x3f, 0xf1, 0xff, 0x80, 0x07, 0xff, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00,
0x01, 0xff, 0xc0, 0x07, 0xff, 0xe0, 0x00, 0x00, 0x1f, 0xfe, 0x01, 0xff, 0xff, 0xff, 0xfe, 0x00,
0x01, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x07, 0xff, 0x80,
0x01, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xc0,
0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xc0,
0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x07, 0xff, 0xe0, 0x00, 0x03, 0xf0, 0x00, 0x03, 0xff, 0xc0,
0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x00, 0x1f, 0xff, 0x00, 0x1f, 0xff, 0x00,
0x01, 0xff, 0xc0, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0x87, 0xff, 0xff, 0xff, 0xfc, 0x00,
0x01, 0xff, 0xc0, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0x80, 0x7f, 0xff, 0xff, 0xc0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Logo NXDN_sm, 128x16px
const unsigned char logo_NXDN_bmp [] =
{
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xf0, 0x1f, 0xf8, 0x0f, 0x00, 0xff, 0x80, 0x7c, 0x00, 0x0f, 0xff, 0x80, 0x7f, 0xe0, 0x7f,
0xff, 0xe0, 0x0f, 0xf0, 0x1f, 0x80, 0x7e, 0x01, 0xf8, 0x00, 0x00, 0x7f, 0x00, 0x3f, 0xc0, 0x7f,
0xff, 0xc0, 0x07, 0xe0, 0x3f, 0x80, 0x38, 0x07, 0xf0, 0x00, 0x00, 0x3e, 0x00, 0x3f, 0x80, 0xff,
0xff, 0x80, 0x03, 0xc0, 0x3f, 0xc0, 0x00, 0x3f, 0xe0, 0x1f, 0x80, 0x3e, 0x00, 0x1f, 0x01, 0xff,
0xff, 0x00, 0x03, 0x80, 0x7f, 0xe0, 0x00, 0xff, 0xc0, 0x3f, 0x80, 0x3c, 0x00, 0x0e, 0x03, 0xff,
0xfe, 0x00, 0x01, 0x00, 0xff, 0xe0, 0x03, 0xff, 0x80, 0x7f, 0x80, 0x78, 0x08, 0x04, 0x03, 0xff,
0xfc, 0x03, 0x00, 0x01, 0xff, 0x80, 0x01, 0xff, 0x00, 0xff, 0x00, 0xf0, 0x1c, 0x00, 0x07, 0xff,
0xfc, 0x07, 0x80, 0x03, 0xfc, 0x00, 0x01, 0xfe, 0x01, 0xfc, 0x01, 0xe0, 0x1e, 0x00, 0x0f, 0xff,
0xf8, 0x0f, 0xc0, 0x07, 0xf0, 0x0e, 0x00, 0xfc, 0x00, 0x00, 0x07, 0xc0, 0x3f, 0x00, 0x1f, 0xff,
0xf0, 0x1f, 0xe0, 0x0f, 0x80, 0x3f, 0x00, 0x7c, 0x00, 0x00, 0x3f, 0xc0, 0x7f, 0x80, 0x3f, 0xff,
0xe0, 0x3f, 0xf0, 0x0e, 0x01, 0xff, 0x80, 0x38, 0x00, 0x07, 0xff, 0x80, 0xff, 0x80, 0x7f, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Logo POCASG/DAPNET, 128x16px
const unsigned char logo_POCSAG_bmp [] =
{
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xf8, 0x7f, 0xfe, 0x03, 0xfe, 0xfe, 0x03, 0xdf, 0xf6, 0x00, 0x00, 0x1f, 0xff,
0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0xfc, 0xfc, 0xfe, 0xfc, 0xcf, 0xf6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0xfe, 0x7d, 0x7e, 0xfe, 0xc7, 0xf6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xfb, 0x7a, 0x7e, 0xff, 0x79, 0x7e, 0xfe, 0xd3, 0xf6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xf7, 0xbe, 0xff, 0x7b, 0xbe, 0xfe, 0xdb, 0xf6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xbe, 0xff, 0xbb, 0xbe, 0xfe, 0xdd, 0xf6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xf9, 0xff, 0xbe, 0xff, 0xb7, 0xde, 0xfe, 0xde, 0xf6, 0x01, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xee, 0x77, 0xbe, 0xff, 0xb7, 0xde, 0x81, 0xde, 0x76, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xdf, 0xb7, 0x7e, 0xff, 0xa0, 0x1e, 0xff, 0xdf, 0x36, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xdf, 0xbc, 0xfe, 0xff, 0x6f, 0xee, 0xff, 0xdf, 0xb6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xdf, 0xbf, 0xfe, 0xff, 0x6f, 0xee, 0xff, 0xdf, 0xd6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xdf, 0xbf, 0xfe, 0xfe, 0xdf, 0xf6, 0xff, 0xdf, 0xe6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xdf, 0x7f, 0xfe, 0xf9, 0xdf, 0xf6, 0xff, 0xdf, 0xe6, 0xff, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xe6, 0x7f, 0xfe, 0x07, 0xff, 0xf6, 0xff, 0xdf, 0xf6, 0x00, 0xfb, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

COLED::COLED(unsigned char displayType, unsigned char displayBrightness, bool displayInvert, bool displayScroll, bool displayRotate, bool displayLogoScreensaver, bool slot1Enabled, bool slot2Enabled) :
m_displayType(displayType),
m_displayBrightness(displayBrightness),
m_displayInvert(displayInvert),
m_displayScroll(displayScroll),
m_displayRotate(displayRotate),
m_displayLogoScreensaver(displayLogoScreensaver),
m_slot1Enabled(slot1Enabled),
m_slot2Enabled(slot2Enabled),
m_ipaddress(),
m_display()
{
}

COLED::~COLED()
{
}

bool COLED::open()
{

    // SPI
    if (m_display.oled_is_spi_proto(m_displayType))
    {
        // SPI change parameters to fit to your LCD
        if ( !m_display.init(OLED_SPI_DC,OLED_SPI_RESET,OLED_SPI_CS, m_displayType) )
            return false;
    }
    else
    {
        // I2C change parameters to fit to your LCD
        if ( !m_display.init(OLED_I2C_RESET, m_displayType) )
            return false;
    }


    m_display.begin();

    m_display.invertDisplay(m_displayInvert ? 1 : 0);
    if (m_displayBrightness > 0U)
        m_display.setBrightness(m_displayBrightness);

    if (m_displayRotate > 0U) {
      m_display.sendCommand(0xC0);
      m_display.sendCommand(0xA0);
    }

    // init done
    m_display.setTextWrap(false); // disable text wrap as default
    m_display.clearDisplay();   // clears the screen  buffer
    m_display.display();        // display it (clear display)

    OLED_statusbar();
    m_display.setCursor(0,OLED_LINE3);
    m_display.print("Startup");
    m_display.display();

    return true;
}

void COLED::setIdleInt()
{
    m_mode = MODE_IDLE;

    m_display.clearDisplay();
    OLED_statusbar();

//    m_display.setCursor(0,30);
//    m_display.setTextSize(3);
//    m_display.print("Idle");

//    m_display.setTextSize(1);
    if (m_displayScroll && m_displayLogoScreensaver)
        m_display.startscrolldiagleft(0x00,0x0f);  //the MMDVM logo scrolls the whole screen
    m_display.display();

    unsigned char info[100U];
    CNetworkInfo* m_network;

    passCounter ++;
    if (passCounter > 253U)
        networkInfoInitialized = false;

    if (! networkInfoInitialized) {
        //LogMessage("Initialize CNetworkInfo");
        info[0]=0;
        m_network = new CNetworkInfo;
        m_network->getNetworkInterface(info);
        m_ipaddress = (char*)info;
        delete m_network;

        networkInfoInitialized = true;
        passCounter = 0;
    }
}

void COLED::setErrorInt(const char* text)
{
    m_mode = MODE_ERROR;

    m_display.clearDisplay();
    OLED_statusbar();

    m_display.setTextWrap(true);    // text wrap temorally enable
    m_display.setCursor(0,OLED_LINE1);
    m_display.printf("%s\n",text);
    m_display.setTextWrap(false);

    m_display.display();
}

void COLED::setLockoutInt()
{
    m_mode = MODE_LOCKOUT;

    m_display.clearDisplay();
    OLED_statusbar();

    m_display.setCursor(0,30);
    m_display.setTextSize(3);
    m_display.print("Lockout");

    m_display.setTextSize(1);
    m_display.display();
}

void COLED::setQuitInt()
{
    m_mode = MODE_QUIT;

    m_display.clearDisplay();
    OLED_statusbar();

    m_display.setCursor(0,30);
    m_display.setTextSize(3);
    m_display.print("Stopped");

    m_display.setTextSize(1);
    m_display.display();
}

void COLED::setFMInt()
{
    m_mode = MODE_FM;

    m_display.clearDisplay();
    OLED_statusbar();

    m_display.setCursor(0,30);
    m_display.setTextSize(3);
    m_display.print("FM");

    m_display.setTextSize(1);
    m_display.display();
}

void COLED::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
    m_mode = MODE_DSTAR;

    m_display.clearDisplay();
    m_display.fillRect(0,OLED_LINE3,m_display.width(),m_display.height(),BLACK); //clear everything beneath logo

    m_display.setCursor(0,OLED_LINE3);
    m_display.printf("%s %.8s/%4.4s",type,my1,my2);

    m_display.setCursor(0,OLED_LINE4);
    m_display.printf("-> %.8s",your);

    m_display.setCursor(0,OLED_LINE5);
    m_display.printf("via %.8s",reflector);

    m_display.setCursor(0,OLED_LINE6);
    m_display.printf("%s",m_ipaddress.c_str());

    OLED_statusbar();
    m_display.display();

}

void COLED::clearDStarInt()
{
    m_display.fillRect(0,OLED_LINE3, m_display.width(),m_display.height(),BLACK); //clear everything beneath the logo

    m_display.setCursor(40,OLED_LINE3);
    m_display.print("Listening");

    m_display.setCursor(0,OLED_LINE5);
    m_display.printf("%s",m_ipaddress.c_str());

    m_display.display();
}

void COLED::writeDMRInt(unsigned int slotNo,const std::string& src,bool group,const std::string& dst,const char* type)
{
    CUserDBentry tmp;

    tmp.set(keyCALLSIGN, src);
    writeDMRIntEx(slotNo, tmp, group, dst, type);
}

#define CALLandNAME(u) ((u).get(keyCALLSIGN) + " " + (u).get(keyFIRST_NAME))

int COLED::writeDMRIntEx(unsigned int slotNo, const class CUserDBentry& src, bool group, const std::string& dst, const char* type)
{

    if (m_mode != MODE_DMR) {
        m_display.clearDisplay();
        m_mode = MODE_DMR;
        clearDMRInt(slotNo);
    }
    // if both slots, use lines 2-3 for slot 1, lines 4-5 for slot 2
    // if single slot, use lines 2-3
    if ( m_slot1Enabled && m_slot2Enabled ) {

        if (slotNo == 1U) {
            m_display.fillRect(0,OLED_LINE2,m_display.width(),40,BLACK);
            m_display.setCursor(0,OLED_LINE2);
            m_display.printf("%s",CALLandNAME(src).c_str());
            m_display.setCursor(0,OLED_LINE3);
            m_display.printf("Slot: %i %s %s%s",slotNo,type,group ? "TG: " : "",dst.c_str());
        }
        else
        {
            m_display.fillRect(0,OLED_LINE4,m_display.width(),40,BLACK);
            m_display.setCursor(0,OLED_LINE4);
            m_display.printf("%s",CALLandNAME(src).c_str());
            m_display.setCursor(0,OLED_LINE5);
            m_display.printf("Slot: %i %s %s%s",slotNo,type,group ? "TG: " : "",dst.c_str());
        }

        m_display.fillRect(0,OLED_LINE6,m_display.width(),20,BLACK);
        m_display.setCursor(0,OLED_LINE6);
        m_display.printf("%s",m_ipaddress.c_str());
    }
    else
    {
        m_display.fillRect(0,OLED_LINE2,m_display.width(),m_display.height(),BLACK);
        m_display.setCursor(0,OLED_LINE2);
        m_display.printf("%s",CALLandNAME(src).c_str());
        m_display.setCursor(0,OLED_LINE3);
        m_display.printf("Slot: %i %s %s%s",slotNo,type,group ? "TG: " : "",dst.c_str());
        m_display.setCursor(0,OLED_LINE4);
        m_display.printf("%s",src.get(keyCITY).c_str());
        m_display.setCursor(0,OLED_LINE5);
        m_display.printf("%s",src.get(keySTATE).c_str());
        m_display.setCursor(0,OLED_LINE6);
        m_display.printf("%s",src.get(keyCOUNTRY).c_str());
    }

    OLED_statusbar();
    m_display.display();

    // must be 0, to avoid calling writeDMRInt() from CDisplay::writeDMR()
    return 0;
}

void COLED::clearDMRInt(unsigned int slotNo)
{
    // if both slots, use lines 2-3 for slot 1, lines 4-5 for slot 2
    // if single slot, use lines 2-3
    if ( m_slot1Enabled && m_slot2Enabled ){
        if (slotNo == 1U) {
            m_display.fillRect(0, OLED_LINE3, m_display.width(), 40, BLACK);
            m_display.setCursor(0,OLED_LINE3);
            m_display.print("Slot: 1 Listening");
        }
        else {
            m_display.fillRect(0, OLED_LINE5, m_display.width(), 40, BLACK);
            m_display.setCursor(0, OLED_LINE5);
            m_display.print("Slot: 2 Listening");
        }
    }
    else {
        m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);
        m_display.setCursor(0,OLED_LINE3);
        m_display.printf("Slot: %i Listening",slotNo);
    }

    m_display.fillRect(0, OLED_LINE6, m_display.width(), 20, BLACK);
    m_display.setCursor(0,OLED_LINE6);
    m_display.printf("%s",m_ipaddress.c_str());
    m_display.display();
}

void COLED::writeFusionInt(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin)
{
    m_mode = MODE_YSF;

    m_display.clearDisplay();
    m_display.fillRect(0,OLED_LINE2,m_display.width(),m_display.height(),BLACK);

    m_display.setCursor(0,OLED_LINE4);
    m_display.printf("%s %.10s", type, source);

    m_display.setCursor(0,OLED_LINE5);
    m_display.printf("  DG-ID %u", dgid);

    OLED_statusbar();

    m_display.display();
}

void COLED::clearFusionInt()
{
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40,OLED_LINE4);
    m_display.print("Listening");

    m_display.setCursor(0,OLED_LINE6);
    m_display.printf("%s",m_ipaddress.c_str());

    m_display.display();
}

void COLED::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
    m_mode = MODE_P25;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(0,OLED_LINE3);
    m_display.printf("%s %.10s", type, source);

    m_display.setCursor(0,OLED_LINE4);
    m_display.printf("  %s%u", group ? "TG" : "", dest);

    OLED_statusbar();
    m_display.display();

}

void COLED::clearP25Int()
{
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40,OLED_LINE4);
    m_display.print("Listening");

    m_display.setCursor(0,OLED_LINE6);
    m_display.printf("%s",m_ipaddress.c_str());

    m_display.display();
}

void COLED::writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type)
{
    CUserDBentry tmp;

    tmp.set(keyCALLSIGN, source);
    writeNXDNIntEx(tmp, group, dest, type);
}

int COLED::writeNXDNIntEx(const class CUserDBentry& source, bool group, unsigned int dest, const char* type)
{
    m_mode = MODE_NXDN;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(0,OLED_LINE2);
    m_display.printf("%s %s", type, CALLandNAME(source).c_str());

    m_display.setCursor(0,OLED_LINE3);
    m_display.printf("  %s%u", group ? "TG" : "", dest);

    m_display.setCursor(0,OLED_LINE4);
    m_display.printf("%s",source.get(keyCITY).c_str());

    m_display.setCursor(0,OLED_LINE5);
    m_display.printf("%s",source.get(keySTATE).c_str());

    m_display.setCursor(0,OLED_LINE6);
    m_display.printf("%s",source.get(keyCOUNTRY).c_str());

    OLED_statusbar();
    m_display.display();

    // must be 0, to avoid calling writeNXDNInt() from CDisplay::writeNXDN()
    return 0;
}

void COLED::clearNXDNInt()
{
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40,OLED_LINE3);
    m_display.print("Listening");

    m_display.setCursor(0,OLED_LINE6);
    m_display.printf("%s",m_ipaddress.c_str());

    m_display.display();
}

void COLED::writePOCSAGInt(uint32_t ric, const std::string& message)
{
    m_mode = MODE_POCSAG;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE1, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(0,OLED_LINE3);
    m_display.printf("RIC: %u", ric);

    m_display.setTextWrap(true);    // text wrap temorally enable
    m_display.setCursor(0,OLED_LINE5);
    m_display.printf("MSG: %s", message.c_str());
    m_display.setTextWrap(false);

    OLED_statusbar();
    m_display.display();

}

void COLED::clearPOCSAGInt()
{
    m_display.fillRect(0, OLED_LINE1, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40,OLED_LINE4);
    m_display.print("Listening");

    m_display.setCursor(0,OLED_LINE6);
    m_display.printf("%s",m_ipaddress.c_str());

    m_display.display();
}

void COLED::writeCWInt()
{
    m_display.clearDisplay();

    m_display.setCursor(10,30);
    m_display.setTextSize(1);
    m_display.print("CW TX");

    m_display.setTextSize(1);
    m_display.display();
    if (m_displayScroll)
        m_display.startscrollleft(0x02,0x0f);
}

void COLED::clearCWInt()
{
    m_display.clearDisplay();

    //m_display.setCursor(0,30);
    //m_display.setTextSize(3);
    //m_display.print("Idle");

    //m_display.setTextSize(1);
    //m_display.display();
    //m_display.startscrollleft(0x02,0x0f);

    OLED_statusbar();
    m_display.display();
    if (m_displayScroll)
        m_display.startscrollleft(0x02,0x0f);
}

void COLED::close()
{
    m_display.clearDisplay();
    m_display.fillRect(0, 0, m_display.width(), 16, BLACK);
    if (m_displayScroll)
        m_display.startscrollleft(0x00,0x01);
    m_display.setCursor(0,00);
    m_display.setTextSize(2);
    m_display.print("-CLOSE-");
    m_display.display();

    m_display.close();
}

void COLED::OLED_statusbar()
{
    m_display.stopscroll();
    m_display.fillRect(0, 0, m_display.width(), 16, BLACK);
    m_display.setTextColor(WHITE);

    m_display.setCursor(0,0);
    if (m_mode == MODE_DMR)
        m_display.drawBitmap(0, 0, logo_dmr_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_DSTAR)
        m_display.drawBitmap(0, 0, logo_dstar_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_YSF)
        m_display.drawBitmap(0, 0, logo_fusion_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_P25)
        m_display.drawBitmap(0, 0, logo_P25_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_NXDN)
        m_display.drawBitmap(0, 0, logo_NXDN_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_POCSAG)
        m_display.drawBitmap(0, 0, logo_POCSAG_bmp, 128, 16, WHITE);
    else if (m_displayLogoScreensaver)
        m_display.drawBitmap(0, 0, logo_glcd_bmp, 128, 16, WHITE);

    if (m_displayScroll)
        m_display.startscrollleft(0x00,0x01);
}
