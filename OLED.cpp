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
#include "NetworkInfo.h"

//Logo MMDVM for Idle Screen
static unsigned char logo_glcd_bmp[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x01U, 0xF8U, 0x03U, 0xFCU, 0x7FU, 0x80U, 0x3FU, 0xC7U, 0xFFU, 0xFCU, 0xF8U, 0x00U, 0xF9U, 0xFCU, 0x01U, 0xFEU,
0x01U, 0xFCU, 0x07U, 0xFCU, 0x7FU, 0xC0U, 0x7FU, 0xC4U, 0x00U, 0x02U, 0x48U, 0x00U, 0x91U, 0xFEU, 0x03U, 0xFEU,
0x03U, 0xFCU, 0x07U, 0xFCU, 0x7FU, 0xC0U, 0x7FU, 0xC5U, 0xFFU, 0xF1U, 0x24U, 0x01U, 0x23U, 0xFEU, 0x03U, 0xFEU,
0x03U, 0xFEU, 0x0FU, 0xBCU, 0x7BU, 0xD0U, 0xFBU, 0xC5U, 0x00U, 0x09U, 0x24U, 0x01U, 0x23U, 0xDFU, 0x07U, 0xDEU,
0x07U, 0xDEU, 0x0FU, 0x3CU, 0x79U, 0xD0U, 0xF3U, 0xC5U, 0x00U, 0x05U, 0x12U, 0x02U, 0x47U, 0xCFU, 0x07U, 0x9EU,
0x07U, 0x9FU, 0x1FU, 0x3CU, 0x79U, 0xF1U, 0xF3U, 0xC5U, 0x00U, 0x05U, 0x12U, 0x02U, 0x47U, 0x8FU, 0x8FU, 0x9EU,
0x0FU, 0x8FU, 0x1EU, 0x3CU, 0x78U, 0xF1U, 0xE3U, 0xC5U, 0x00U, 0x05U, 0x09U, 0x04U, 0x8FU, 0x87U, 0x8FU, 0x1EU,
0x0FU, 0x0FU, 0xBEU, 0x3CU, 0x78U, 0xFBU, 0xE3U, 0xC5U, 0x00U, 0x05U, 0x09U, 0x04U, 0x8FU, 0x07U, 0xDFU, 0x1EU,
0x1FU, 0x07U, 0xFCU, 0x3CU, 0x78U, 0x7FU, 0xC3U, 0xC5U, 0x00U, 0x05U, 0x04U, 0x89U, 0x1FU, 0x03U, 0xFEU, 0x1EU,
0x1EU, 0x03U, 0xFCU, 0x3CU, 0x78U, 0x7FU, 0xC3U, 0xC5U, 0x00U, 0x09U, 0x04U, 0x89U, 0x1EU, 0x01U, 0xFEU, 0x1EU,
0x3EU, 0x03U, 0xF8U, 0x3CU, 0x78U, 0x3FU, 0x83U, 0xC5U, 0xFFU, 0xF1U, 0x02U, 0x72U, 0x3EU, 0x01U, 0xFCU, 0x1EU,
0x3CU, 0x01U, 0xF0U, 0x3CU, 0x78U, 0x1FU, 0x03U, 0xC4U, 0x00U, 0x02U, 0x02U, 0x02U, 0x3CU, 0x00U, 0xF8U, 0x1EU,
0x7CU, 0x01U, 0xF0U, 0x3CU, 0x78U, 0x1FU, 0x03U, 0xC7U, 0xFFU, 0xFCU, 0x01U, 0xFCU, 0x7CU, 0x00U, 0xF8U, 0x1EU,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

//Logo D-Star 128x16 px
static unsigned char logo_dstar_bmp[] = {
0x00U, 0x00U, 0x20U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x60U, 0x03U, 0xFFU, 0xC0U, 0x00U, 0x00U, 0x1FU, 0xF0U, 0xFFU, 0xFEU, 0x07U, 0x80U, 0x3FU, 0xF8U,
0x00U, 0x00U, 0xC0U, 0x07U, 0xC1U, 0xD0U, 0x00U, 0x00U, 0x78U, 0x7CU, 0xFFU, 0xFEU, 0x0FU, 0xC0U, 0x3FU, 0xFCU,
0x00U, 0x01U, 0xC0U, 0x07U, 0x80U, 0xF0U, 0x00U, 0x00U, 0xD0U, 0x3CU, 0x07U, 0x80U, 0x0FU, 0xC0U, 0x78U, 0x0EU,
0x00U, 0x03U, 0xC0U, 0x07U, 0x80U, 0x70U, 0x00U, 0x00U, 0xD0U, 0x38U, 0x07U, 0x00U, 0x1BU, 0xC0U, 0x78U, 0x0EU,
0x00U, 0x07U, 0xC0U, 0x07U, 0x80U, 0x70U, 0x00U, 0x01U, 0xD0U, 0x00U, 0x07U, 0x00U, 0x33U, 0xC0U, 0x70U, 0x1EU,
0x07U, 0xFFU, 0xFEU, 0x07U, 0x00U, 0x70U, 0x00U, 0x01U, 0xF8U, 0x00U, 0x07U, 0x00U, 0x63U, 0xC0U, 0x70U, 0x3CU,
0x01U, 0xFFU, 0xF8U, 0x0FU, 0x00U, 0x71U, 0xFFU, 0xD0U, 0xFFU, 0xF0U, 0x0EU, 0x00U, 0xE1U, 0xD0U, 0xFFU, 0xD0U,
0x00U, 0x7FU, 0xD0U, 0x0FU, 0x00U, 0x60U, 0x00U, 0x00U, 0x03U, 0xF8U, 0x0EU, 0x00U, 0xC1U, 0xD0U, 0xFFU, 0xD0U,
0x00U, 0x3FU, 0x80U, 0x0EU, 0x00U, 0xD0U, 0x00U, 0x00U, 0x00U, 0xF0U, 0x0EU, 0x01U, 0xFFU, 0xD0U, 0xD0U, 0x70U,
0x00U, 0x7FU, 0x00U, 0x1EU, 0x00U, 0xD0U, 0x00U, 0x03U, 0x80U, 0x70U, 0x0CU, 0x03U, 0xFCU, 0xD0U, 0xD0U, 0x30U,
0x00U, 0xFFU, 0x00U, 0x1EU, 0x01U, 0xC0U, 0x00U, 0x07U, 0x80U, 0xD0U, 0x1CU, 0x07U, 0x00U, 0xE1U, 0xD0U, 0x38U,
0x01U, 0xEFU, 0x00U, 0x1CU, 0x07U, 0x80U, 0x00U, 0x07U, 0xC1U, 0xD0U, 0x1CU, 0x06U, 0x00U, 0xF1U, 0xC0U, 0x38U,
0x03U, 0x87U, 0x00U, 0x3FU, 0xFFU, 0x00U, 0x00U, 0x03U, 0xFFU, 0x80U, 0x1CU, 0x0CU, 0x00U, 0xF3U, 0xC0U, 0x38U,
0x06U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x3CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

//Logo DMR 128x16 px
static unsigned char logo_dmr_bmp[] = {
0x00U, 0x01U, 0xFFU, 0xFFU, 0xF8U, 0x01U, 0xF8U, 0x00U, 0x00U, 0x1FU, 0x1FU, 0xFFU, 0xFFU, 0xFCU, 0x00U, 0x00U,
0x00U, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0x81U, 0xFCU, 0x00U, 0x00U, 0x3FU, 0x1FU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U,
0x00U, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0xE1U, 0xFEU, 0x00U, 0x00U, 0xFFU, 0x1FU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x0FU, 0xF1U, 0xFFU, 0x80U, 0x01U, 0xFFU, 0x1FU, 0x80U, 0x00U, 0x1FU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x03U, 0xF9U, 0xFFU, 0xC0U, 0x03U, 0xFFU, 0x1FU, 0x80U, 0x00U, 0x0FU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x01U, 0xF9U, 0xFFU, 0xD0U, 0x07U, 0xFFU, 0x1FU, 0x80U, 0x00U, 0x0FU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x01U, 0xFDU, 0xF3U, 0xF0U, 0x1FU, 0x9FU, 0x1FU, 0x80U, 0x00U, 0x1FU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x00U, 0xFDU, 0xF1U, 0xFCU, 0x3FU, 0x1FU, 0x1FU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x00U, 0xFDU, 0xF0U, 0xFEU, 0x7EU, 0x1FU, 0x1FU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x01U, 0xFDU, 0xF0U, 0x7FU, 0xFCU, 0x1FU, 0x1FU, 0xFFU, 0xFFU, 0xFCU, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x01U, 0xF9U, 0xF0U, 0x1FU, 0xF0U, 0x1FU, 0x1FU, 0x81U, 0xFCU, 0x00U, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x07U, 0xF9U, 0xF0U, 0x0FU, 0xD0U, 0x1FU, 0x1FU, 0x80U, 0x7FU, 0x00U, 0x00U, 0x00U,
0x00U, 0x01U, 0xF8U, 0x00U, 0x3FU, 0xF1U, 0xF0U, 0x07U, 0xC0U, 0x1FU, 0x1FU, 0x80U, 0x3FU, 0xC0U, 0x00U, 0x00U,
0x00U, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0xC1U, 0xF0U, 0x03U, 0x80U, 0x1FU, 0x1FU, 0x80U, 0x0FU, 0xF0U, 0x00U, 0x00U,
0x00U, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0x01U, 0xF0U, 0x00U, 0x00U, 0x1FU, 0x1FU, 0x80U, 0x03U, 0xFCU, 0x00U, 0x00U,
0x00U, 0x01U, 0xFFU, 0xFFU, 0xF0U, 0x01U, 0xF0U, 0x00U, 0x00U, 0x1FU, 0x1FU, 0x80U, 0x01U, 0xFFU, 0x00U, 0x00U};

//Logo Fusion 128x16
const unsigned char logo_fusion_bmp [] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x03U, 0xF8U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0xFFU, 0xFFU, 0xD0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x07U, 0xF0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0xFFU, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x07U, 0xD0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x01U, 0xFEU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x03U, 0xFCU, 0x00U, 0x1FU, 0xE1U, 0xFEU, 0x1FU, 0xFFU, 0xF8U, 0x7FU, 0xC3U, 0xFFU, 0xFFU, 0x1FU, 0xFFU, 0xFEU,
0x03U, 0xFCU, 0x00U, 0x3FU, 0xC3U, 0xFCU, 0x3FU, 0x80U, 0x00U, 0x7FU, 0x87U, 0xF0U, 0xFFU, 0x0FU, 0xF1U, 0xFFU,
0x07U, 0xFFU, 0xFCU, 0x7FU, 0x83U, 0xF8U, 0x7FU, 0x80U, 0x00U, 0xFFU, 0x0FU, 0xF0U, 0xFFU, 0x1FU, 0xE1U, 0xFEU,
0x0FU, 0xFFU, 0xF0U, 0x7FU, 0x07U, 0xF0U, 0xFFU, 0xFFU, 0xC1U, 0xFFU, 0x1FU, 0xE1U, 0xFEU, 0x3FU, 0xC3U, 0xFCU,
0x0FU, 0xF0U, 0x00U, 0xFEU, 0x0FU, 0xD0U, 0x7FU, 0xFFU, 0xE1U, 0xFEU, 0x3FU, 0xC3U, 0xFCU, 0x3FU, 0xC3U, 0xFCU,
0x1FU, 0xD0U, 0x01U, 0xFCU, 0x1FU, 0xD0U, 0x1FU, 0xFFU, 0xE3U, 0xFCU, 0x3FU, 0xC3U, 0xF8U, 0x7FU, 0x87U, 0xF8U,
0x3FU, 0xC0U, 0x03U, 0xFCU, 0x3FU, 0xC0U, 0x00U, 0x3FU, 0xC7U, 0xF8U, 0x7FU, 0x87U, 0xF8U, 0xFFU, 0x0FU, 0xF0U,
0x7FU, 0xC0U, 0x03U, 0xFFU, 0xFFU, 0xD0U, 0x00U, 0x7FU, 0x07U, 0xF8U, 0x7FU, 0xCFU, 0xE1U, 0xFFU, 0x1FU, 0xF8U,
0x7FU, 0x80U, 0x01U, 0xFFU, 0xFFU, 0xC7U, 0xFFU, 0xFCU, 0x0FU, 0xF0U, 0x3FU, 0xFFU, 0x81U, 0xFEU, 0x1FU, 0xF0U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

//Logo P25 128x16px
const unsigned char logo_P25_bmp [] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x01U, 0xFFU, 0xFFU, 0xFFU, 0xF0U, 0x00U, 0x03U, 0xFFU, 0xFFU, 0xC0U, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0xF8U, 0x00U,
0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x80U, 0x3FU, 0xFFU, 0xFFU, 0xFCU, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0xF8U, 0x00U,
0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xD0U, 0xFFU, 0xF8U, 0x3FU, 0xFFU, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0xF8U, 0x00U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x7FU, 0xF1U, 0xFFU, 0xC0U, 0x07U, 0xFFU, 0x01U, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x3FU, 0xF3U, 0xFFU, 0x80U, 0x03U, 0xFFU, 0x81U, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x3FU, 0xF1U, 0xFFU, 0x80U, 0x07U, 0xFFU, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0xD0U, 0x00U,
0x01U, 0xFFU, 0xC0U, 0x07U, 0xFFU, 0xD0U, 0x00U, 0x00U, 0x1FU, 0xFEU, 0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFEU, 0x00U,
0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x80U, 0x00U, 0x00U, 0xFFU, 0xF8U, 0x00U, 0x00U, 0x00U, 0x07U, 0xFFU, 0x80U,
0x01U, 0xFFU, 0xFFU, 0xFFU, 0xF8U, 0x00U, 0x00U, 0x07U, 0xFFU, 0xC0U, 0x00U, 0x00U, 0x00U, 0x01U, 0xFFU, 0xC0U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x7FU, 0xFEU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0xFFU, 0xC0U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x00U, 0x00U, 0x07U, 0xFFU, 0xD0U, 0x00U, 0x03U, 0xF0U, 0x00U, 0x03U, 0xFFU, 0xC0U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0x80U, 0x00U, 0x1FU, 0xFFU, 0x00U, 0x1FU, 0xFFU, 0x00U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x00U, 0x07U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x87U, 0xFFU, 0xFFU, 0xFFU, 0xFCU, 0x00U,
0x01U, 0xFFU, 0xC0U, 0x00U, 0x00U, 0x07U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x80U, 0x7FU, 0xFFU, 0xFFU, 0xC0U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

// Logo NXDN_sm, 128x16px
const unsigned char logo_NXDN_bmp [] = {
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xF0U, 0x1FU, 0xF8U, 0x0FU, 0x00U, 0xFFU, 0x80U, 0x7CU, 0x00U, 0x0FU, 0xFFU, 0x80U, 0x7FU, 0xD0U, 0x7FU,
0xFFU, 0xD0U, 0x0FU, 0xF0U, 0x1FU, 0x80U, 0x7EU, 0x01U, 0xF8U, 0x00U, 0x00U, 0x7FU, 0x00U, 0x3FU, 0xC0U, 0x7FU,
0xFFU, 0xC0U, 0x07U, 0xD0U, 0x3FU, 0x80U, 0x38U, 0x07U, 0xF0U, 0x00U, 0x00U, 0x3EU, 0x00U, 0x3FU, 0x80U, 0xFFU,
0xFFU, 0x80U, 0x03U, 0xC0U, 0x3FU, 0xC0U, 0x00U, 0x3FU, 0xD0U, 0x1FU, 0x80U, 0x3EU, 0x00U, 0x1FU, 0x01U, 0xFFU,
0xFFU, 0x00U, 0x03U, 0x80U, 0x7FU, 0xD0U, 0x00U, 0xFFU, 0xC0U, 0x3FU, 0x80U, 0x3CU, 0x00U, 0x0EU, 0x03U, 0xFFU,
0xFEU, 0x00U, 0x01U, 0x00U, 0xFFU, 0xD0U, 0x03U, 0xFFU, 0x80U, 0x7FU, 0x80U, 0x78U, 0x08U, 0x04U, 0x03U, 0xFFU,
0xFCU, 0x03U, 0x00U, 0x01U, 0xFFU, 0x80U, 0x01U, 0xFFU, 0x00U, 0xFFU, 0x00U, 0xF0U, 0x1CU, 0x00U, 0x07U, 0xFFU,
0xFCU, 0x07U, 0x80U, 0x03U, 0xFCU, 0x00U, 0x01U, 0xFEU, 0x01U, 0xFCU, 0x01U, 0xD0U, 0x1EU, 0x00U, 0x0FU, 0xFFU,
0xF8U, 0x0FU, 0xC0U, 0x07U, 0xF0U, 0x0EU, 0x00U, 0xFCU, 0x00U, 0x00U, 0x07U, 0xC0U, 0x3FU, 0x00U, 0x1FU, 0xFFU,
0xF0U, 0x1FU, 0xD0U, 0x0FU, 0x80U, 0x3FU, 0x00U, 0x7CU, 0x00U, 0x00U, 0x3FU, 0xC0U, 0x7FU, 0x80U, 0x3FU, 0xFFU,
0xD0U, 0x3FU, 0xF0U, 0x0EU, 0x01U, 0xFFU, 0x80U, 0x38U, 0x00U, 0x07U, 0xFFU, 0x80U, 0xFFU, 0x80U, 0x7FU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

// Logo M17_sm, 128x16px
// XXX FIXME This is the NXDN logo, it needs replacing with the M17 logo
const unsigned char logo_M17_bmp [] = {
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xF0U, 0x1FU, 0xF8U, 0x0FU, 0x00U, 0xFFU, 0x80U, 0x7CU, 0x00U, 0x0FU, 0xFFU, 0x80U, 0x7FU, 0xD0U, 0x7FU,
0xFFU, 0xD0U, 0x0FU, 0xF0U, 0x1FU, 0x80U, 0x7EU, 0x01U, 0xF8U, 0x00U, 0x00U, 0x7FU, 0x00U, 0x3FU, 0xC0U, 0x7FU,
0xFFU, 0xC0U, 0x07U, 0xD0U, 0x3FU, 0x80U, 0x38U, 0x07U, 0xF0U, 0x00U, 0x00U, 0x3EU, 0x00U, 0x3FU, 0x80U, 0xFFU,
0xFFU, 0x80U, 0x03U, 0xC0U, 0x3FU, 0xC0U, 0x00U, 0x3FU, 0xD0U, 0x1FU, 0x80U, 0x3EU, 0x00U, 0x1FU, 0x01U, 0xFFU,
0xFFU, 0x00U, 0x03U, 0x80U, 0x7FU, 0xD0U, 0x00U, 0xFFU, 0xC0U, 0x3FU, 0x80U, 0x3CU, 0x00U, 0x0EU, 0x03U, 0xFFU,
0xFEU, 0x00U, 0x01U, 0x00U, 0xFFU, 0xD0U, 0x03U, 0xFFU, 0x80U, 0x7FU, 0x80U, 0x78U, 0x08U, 0x04U, 0x03U, 0xFFU,
0xFCU, 0x03U, 0x00U, 0x01U, 0xFFU, 0x80U, 0x01U, 0xFFU, 0x00U, 0xFFU, 0x00U, 0xF0U, 0x1CU, 0x00U, 0x07U, 0xFFU,
0xFCU, 0x07U, 0x80U, 0x03U, 0xFCU, 0x00U, 0x01U, 0xFEU, 0x01U, 0xFCU, 0x01U, 0xD0U, 0x1EU, 0x00U, 0x0FU, 0xFFU,
0xF8U, 0x0FU, 0xC0U, 0x07U, 0xF0U, 0x0EU, 0x00U, 0xFCU, 0x00U, 0x00U, 0x07U, 0xC0U, 0x3FU, 0x00U, 0x1FU, 0xFFU,
0xF0U, 0x1FU, 0xD0U, 0x0FU, 0x80U, 0x3FU, 0x00U, 0x7CU, 0x00U, 0x00U, 0x3FU, 0xC0U, 0x7FU, 0x80U, 0x3FU, 0xFFU,
0xD0U, 0x3FU, 0xF0U, 0x0EU, 0x01U, 0xFFU, 0x80U, 0x38U, 0x00U, 0x07U, 0xFFU, 0x80U, 0xFFU, 0x80U, 0x7FU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

// Logo POCASG/DAPNET, 128x16px
const unsigned char logo_POCSAG_bmp [] = {
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xF8U, 0x7FU, 0xFEU, 0x03U, 0xFEU, 0xFEU, 0x03U, 0xDFU, 0xF6U, 0x00U, 0x00U, 0x1FU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x7FU, 0xFEU, 0xFCU, 0xFCU, 0xFEU, 0xFCU, 0xCFU, 0xF6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x7FU, 0xFEU, 0xFEU, 0x7DU, 0x7EU, 0xFEU, 0xC7U, 0xF6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFBU, 0x7AU, 0x7EU, 0xFFU, 0x79U, 0x7EU, 0xFEU, 0xD3U, 0xF6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF7U, 0xBEU, 0xFFU, 0x7BU, 0xBEU, 0xFEU, 0xDBU, 0xF6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xBEU, 0xFFU, 0xBBU, 0xBEU, 0xFEU, 0xDDU, 0xF6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xF9U, 0xFFU, 0xBEU, 0xFFU, 0xB7U, 0xDEU, 0xFEU, 0xDEU, 0xF6U, 0x01U, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xEEU, 0x77U, 0xBEU, 0xFFU, 0xB7U, 0xDEU, 0x81U, 0xDEU, 0x76U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xDFU, 0xB7U, 0x7EU, 0xFFU, 0xA0U, 0x1EU, 0xFFU, 0xDFU, 0x36U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xDFU, 0xBCU, 0xFEU, 0xFFU, 0x6FU, 0xEEU, 0xFFU, 0xDFU, 0xB6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xDFU, 0xBFU, 0xFEU, 0xFFU, 0x6FU, 0xEEU, 0xFFU, 0xDFU, 0xD6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xDFU, 0xBFU, 0xFEU, 0xFEU, 0xDFU, 0xF6U, 0xFFU, 0xDFU, 0xE6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xDFU, 0x7FU, 0xFEU, 0xF9U, 0xDFU, 0xF6U, 0xFFU, 0xDFU, 0xE6U, 0xFFU, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xE6U, 0x7FU, 0xFEU, 0x07U, 0xFFU, 0xF6U, 0xFFU, 0xDFU, 0xF6U, 0x00U, 0xFBU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

COLED::COLED(unsigned char displayType, unsigned char displayBrightness, bool displayInvert, bool displayScroll, bool displayRotate, bool displayLogoScreensaver, bool slot1Enabled, bool slot2Enabled) :
m_displayType(displayType),
m_displayBrightness(displayBrightness),
m_displayInvert(displayInvert),
m_displayScroll(displayScroll),
m_displayRotate(displayRotate),
m_displayLogoScreensaver(displayLogoScreensaver),
m_slot1Enabled(slot1Enabled),
m_slot2Enabled(slot2Enabled),
m_ipAddress(),
m_passCounter(0U),
m_display()
{
}

COLED::~COLED()
{
}

bool COLED::open()
{
    // SPI
    if (m_display.oled_is_spi_proto(m_displayType)) {
        // SPI change parameters to fit to your LCD
        if (!m_display.init(OLED_SPI_DC, OLED_SPI_RESET, OLED_SPI_CS, m_displayType))
            return false;
    } else {
        // I2C change parameters to fit to your LCD
        if (!m_display.init(OLED_I2C_RESET, m_displayType))
            return false;
    }

    m_display.begin();

    m_display.invertDisplay(m_displayInvert ? 1 : 0);
    if (m_displayBrightness > 0U)
        m_display.setBrightness(m_displayBrightness);

    if (m_displayRotate > 0U) {
      m_display.sendCommand(0xC0U);
      m_display.sendCommand(0xA0U);
    }

    // init done
    m_display.setTextWrap(false); // disable text wrap as default
    m_display.clearDisplay();   // clears the screen  buffer
    m_display.display();        // display it (clear display)

    OLED_statusbar();

    m_display.setCursor(0, OLED_LINE3);
    m_display.print("Startup");

    m_display.display();

    return true;
}

void COLED::setIdleInt()
{
    m_mode = MODE_IDLE;

    m_display.clearDisplay();
    OLED_statusbar();

    if (m_displayScroll && m_displayLogoScreensaver)
        m_display.startscrolldiagleft(0x00U, 0x0FU);  //the MMDVM logo scrolls the whole screen
    m_display.display();

    m_passCounter++;
    if (m_passCounter > 253U)
        m_ipAddress.clear();

    if (m_ipAddress.empty()) {
        unsigned char info[100U] = { 0x00U };

        CNetworkInfo network;
        network.getNetworkInterface(info);
        m_ipAddress = (char*)info;

        m_passCounter = 0U;
    }
}

void COLED::setErrorInt(const char* text)
{
    m_mode = MODE_ERROR;

    m_display.clearDisplay();
    OLED_statusbar();

    m_display.setTextWrap(true);    // text wrap temorally enable

    m_display.setCursor(0, OLED_LINE1);
    m_display.printf("%s\n", text);

    m_display.setTextWrap(false);

    m_display.display();
}

void COLED::setLockoutInt()
{
    m_mode = MODE_LOCKOUT;

    m_display.clearDisplay();
    OLED_statusbar();

    m_display.setCursor(0, 30);
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

    m_display.setCursor(0, 30);
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

    m_display.setCursor(0, 30);
    m_display.setTextSize(3);
    m_display.print("FM");

    m_display.setTextSize(1);
    m_display.display();
}

void COLED::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
    m_mode = MODE_DSTAR;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE3, m_display.width(), m_display.height(), BLACK); //clear everything beneath logo

    m_display.setCursor(0, OLED_LINE3);
    m_display.printf("%s %.8s/%4.4s", type, my1, my2);

    m_display.setCursor(0, OLED_LINE4);
    m_display.printf("-> %.8s", your);

    m_display.setCursor(0, OLED_LINE5);
    m_display.printf("via %.8s", reflector);

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", m_ipAddress.c_str());

    OLED_statusbar();
    m_display.display();
}

void COLED::clearDStarInt()
{
    m_display.fillRect(0, OLED_LINE3, m_display.width(), m_display.height(), BLACK); //clear everything beneath the logo

    m_display.setCursor(40, OLED_LINE3);
    m_display.print("Listening");

    m_display.setCursor(0, OLED_LINE5);
    m_display.printf("%s", m_ipAddress.c_str());

    m_display.display();
}

void COLED::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
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

    // If both slots, use lines 2-3 for slot 1, lines 4-5 for slot 2
    // If single slot, use lines 2-3
    if (m_slot1Enabled && m_slot2Enabled) {
        if (slotNo == 1U) {
            m_display.fillRect(0, OLED_LINE2, m_display.width(), 40, BLACK);

            m_display.setCursor(0, OLED_LINE2);
            m_display.printf("%s", CALLandNAME(src).c_str());

            m_display.setCursor(0, OLED_LINE3);
            m_display.printf("Slot: %i %s %s%s", slotNo, type, group ? "TG: " : "", dst.c_str());
        } else {
            m_display.fillRect(0, OLED_LINE4, m_display.width(), 40, BLACK);

            m_display.setCursor(0, OLED_LINE4);
            m_display.printf("%s", CALLandNAME(src).c_str());

            m_display.setCursor(0, OLED_LINE5);
            m_display.printf("Slot: %i %s %s%s",slotNo,type,group ? "TG: " : "",dst.c_str());
        }

        m_display.fillRect(0, OLED_LINE6, m_display.width(), 20, BLACK);

        m_display.setCursor(0, OLED_LINE6);
        m_display.printf("%s",m_ipAddress.c_str());
    } else {
        m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

        m_display.setCursor(0, OLED_LINE2);
        m_display.printf("%s", CALLandNAME(src).c_str());

        m_display.setCursor(0, OLED_LINE3);
        m_display.printf("Slot: %i %s %s%s", slotNo, type, group ? "TG: " : "", dst.c_str());

        m_display.setCursor(0, OLED_LINE4);
        m_display.printf("%s", src.get(keyCITY).c_str());

        m_display.setCursor(0, OLED_LINE5);
        m_display.printf("%s", src.get(keySTATE).c_str());

        m_display.setCursor(0, OLED_LINE6);
        m_display.printf("%s", src.get(keyCOUNTRY).c_str());
    }

    OLED_statusbar();
    m_display.display();

    // must be 0, to avoid calling writeDMRInt() from CDisplay::writeDMR()
    return 0;
}

void COLED::clearDMRInt(unsigned int slotNo)
{
    // If both slots, use lines 2-3 for slot 1, lines 4-5 for slot 2
    // If single slot, use lines 2-3
    if (m_slot1Enabled && m_slot2Enabled) {
        if (slotNo == 1U) {
            m_display.fillRect(0, OLED_LINE3, m_display.width(), 40, BLACK);

            m_display.setCursor(0, OLED_LINE3);
            m_display.print("Slot: 1 Listening");
        } else {
            m_display.fillRect(0, OLED_LINE5, m_display.width(), 40, BLACK);

            m_display.setCursor(0, OLED_LINE5);
            m_display.print("Slot: 2 Listening");
        }
    } else {
        m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

        m_display.setCursor(0, OLED_LINE3);
        m_display.printf("Slot: %i Listening",slotNo);
    }

    m_display.fillRect(0, OLED_LINE6, m_display.width(), 20, BLACK);

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", m_ipAddress.c_str());

    m_display.display();
}

void COLED::writeFusionInt(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin)
{
    m_mode = MODE_YSF;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(0, OLED_LINE4);
    m_display.printf("%s %.10s", type, source);

    m_display.setCursor(0, OLED_LINE5);
    m_display.printf("  DG-ID %u", dgid);

    OLED_statusbar();
    m_display.display();
}

void COLED::clearFusionInt()
{
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40, OLED_LINE4);
    m_display.print("Listening");

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", m_ipAddress.c_str());

    m_display.display();
}

void COLED::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
    m_mode = MODE_P25;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(0, OLED_LINE3);
    m_display.printf("%s %.10s", type, source);

    m_display.setCursor(0, OLED_LINE4);
    m_display.printf("  %s%u", group ? "TG" : "", dest);

    OLED_statusbar();
    m_display.display();
}

void COLED::clearP25Int()
{
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40, OLED_LINE4);
    m_display.print("Listening");

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", m_ipAddress.c_str());

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

    m_display.setCursor(0, OLED_LINE2);
    m_display.printf("%s %s", type, CALLandNAME(source).c_str());

    m_display.setCursor(0, OLED_LINE3);
    m_display.printf("  %s%u", group ? "TG" : "", dest);

    m_display.setCursor(0, OLED_LINE4);
    m_display.printf("%s", source.get(keyCITY).c_str());

    m_display.setCursor(0, OLED_LINE5);
    m_display.printf("%s", source.get(keySTATE).c_str());

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", source.get(keyCOUNTRY).c_str());

    OLED_statusbar();
    m_display.display();

    // must be 0, to avoid calling writeNXDNInt() from CDisplay::writeNXDN()
    return 0;
}

void COLED::clearNXDNInt()
{
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40, OLED_LINE3);
    m_display.print("Listening");

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", m_ipAddress.c_str());

    m_display.display();
}

void COLED::writeM17Int(const char* source, const char* dest, const char* type)
{
    m_mode = MODE_M17;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(0, OLED_LINE3);
    m_display.printf("%s %s", type, source);

    m_display.setCursor(0, OLED_LINE4);
    m_display.printf("  %s", dest);

    OLED_statusbar();
    m_display.display();
}

void COLED::clearM17Int()
{
    m_display.fillRect(0, OLED_LINE2, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40, OLED_LINE4);
    m_display.print("Listening");

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", m_ipAddress.c_str());

    m_display.display();
}

void COLED::writePOCSAGInt(uint32_t ric, const std::string& message)
{
    m_mode = MODE_POCSAG;

    m_display.clearDisplay();
    m_display.fillRect(0, OLED_LINE1, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(0, OLED_LINE3);
    m_display.printf("RIC: %u", ric);

    m_display.setTextWrap(true);    // text wrap temorally enable

    m_display.setCursor(0, OLED_LINE5);
    m_display.printf("MSG: %s", message.c_str());

    m_display.setTextWrap(false);

    OLED_statusbar();
    m_display.display();
}

void COLED::clearPOCSAGInt()
{
    m_display.fillRect(0, OLED_LINE1, m_display.width(), m_display.height(), BLACK);

    m_display.setCursor(40, OLED_LINE4);
    m_display.print("Listening");

    m_display.setCursor(0, OLED_LINE6);
    m_display.printf("%s", m_ipAddress.c_str());

    m_display.display();
}

void COLED::writeCWInt()
{
    m_display.clearDisplay();

    m_display.setCursor(0, 30);
    m_display.setTextSize(3);

    m_display.print("CW TX");

    m_display.setTextSize(1);
    m_display.display();

    if (m_displayScroll)
        m_display.startscrollleft(0x02U, 0x0FU);
}

void COLED::clearCWInt()
{
    m_display.clearDisplay();

    m_display.setCursor(0, 30);
    m_display.setTextSize(3);

    m_display.print("Idle");

    m_display.setTextSize(1);
    m_display.display();

    if (m_displayScroll)
        m_display.startscrollleft(0x02U, 0x0FU);
}

void COLED::close()
{
    m_display.clearDisplay();
    m_display.fillRect(0, 0, m_display.width(), 16, BLACK);

    if (m_displayScroll)
        m_display.startscrollleft(0x00U, 0x01U);

    m_display.setCursor(0, 0);
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

    m_display.setCursor(0, 0);
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
    else if (m_mode == MODE_M17)
        m_display.drawBitmap(0, 0, logo_M17_bmp, 128, 16, WHITE);
    else if (m_mode == MODE_POCSAG)
        m_display.drawBitmap(0, 0, logo_POCSAG_bmp, 128, 16, WHITE);
    else if (m_displayLogoScreensaver)
        m_display.drawBitmap(0, 0, logo_glcd_bmp, 128, 16, WHITE);

    if (m_displayScroll)
        m_display.startscrollleft(0x00U, 0x01U);
}
