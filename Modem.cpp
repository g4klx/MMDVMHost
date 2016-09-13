/*
 *   Copyright (C) 2011-2016 by Jonathan Naylor G4KLX
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

#include "DStarDefines.h"
#include "DMRDefines.h"
#include "YSFDefines.h"
#include "P25Defines.h"
#include "Thread.h"
#include "Modem.h"
#include "Utils.h"
#include "Log.h"

#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <unistd.h>
#endif

const unsigned char P25_DATA[6U][222U] = {
	{ 0xE0U, 0x67U, 0x30U, 0x01U, 0x55U, 0x75U, 0xF5U, 0xFFU, 0x77U, 0xFFU, 0x29U, 0x30U, 0x0EU, 0xCEU, 0xD7U, 0x7EU,
	0x00U, 0x70U, 0x80U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x0AU, 0x37U, 0x18U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x8EU,
	0xB5U, 0xE0U, 0x96U, 0x52U, 0x7CU, 0x2AU, 0x23U, 0x54U, 0x0DU, 0x9FU, 0x34U, 0x7EU, 0xF1U, 0xD4U, 0x1EU, 0x42U,
	0x1EU, 0xA2U, 0x35U, 0x8BU, 0xFEU, 0x31U, 0x2AU, 0x5EU, 0x37U, 0x18U, 0x4AU, 0x96U, 0x51U, 0xBAU, 0x30U, 0xA4U,
	0xF8U, 0x0CU, 0x75U, 0x4CU, 0x75U, 0x10U, 0x02U, 0x00U, 0x00U, 0x0FU, 0x3AU, 0x14U, 0xE9U, 0x4DU, 0x8CU, 0xCEU,
	0xE7U, 0x52U, 0x17U, 0x28U, 0xD6U, 0x45U, 0x77U, 0xF0U, 0xD2U, 0x23U, 0x25U, 0x6BU, 0x00U, 0x00U, 0x27U, 0x6CU,
	0xF2U, 0x86U, 0x46U, 0x45U, 0xE0U, 0x1AU, 0x90U, 0xD1U, 0x27U, 0x00U, 0x0AU, 0xD0U, 0x02U, 0x67U, 0x6AU, 0x13U,
	0x36U, 0xF9U, 0x4AU, 0xFEU, 0xFEU, 0x2AU, 0x8CU, 0x41U, 0xC3U, 0xCFU, 0x94U, 0x30U, 0x56U, 0xEBU, 0x10U, 0xC0U,
	0x8AU, 0xEFU, 0x5FU, 0x14U, 0xB3U, 0x82U, 0xE7U, 0xC4U, 0x4FU, 0x55U, 0x18U, 0xDEU, 0x33U, 0xDCU, 0x7EU, 0x69U,
	0x71U, 0xFFU, 0xB1U, 0xE9U, 0x10U, 0x27U, 0xD8U, 0x1AU, 0x28U, 0xF4U, 0xBFU, 0xB6U, 0x85U, 0x9AU, 0x12U, 0xB7U,
	0x92U, 0x42U, 0x33U, 0xB9U, 0x55U, 0xE0U, 0x8FU, 0x22U, 0x2FU, 0xD6U, 0x2FU, 0x71U, 0x07U, 0x61U, 0x38U, 0xFDU,
	0xF8U, 0x22U, 0xB2U, 0x11U, 0xCBU, 0x8FU, 0x3EU, 0x69U, 0x88U, 0x6FU, 0xDDU, 0x22U, 0x00U, 0x00U },

	{ 0xE0U, 0xDEU, 0x31U, 0x01U, 0x55U, 0x75U, 0xF5U, 0xFFU, 0x77U, 0xFFU, 0x29U, 0x35U, 0x56U, 0x7BU, 0xCBU, 0x19U,
	0x4DU, 0x0DU, 0xDBU, 0x10U, 0xBAU, 0x16U, 0xDEU, 0x2EU, 0x82U, 0x69U, 0x36U, 0x3DU, 0x98U, 0x1FU, 0x9AU, 0xF8U,
	0x8EU, 0xC6U, 0x2BU, 0x80U, 0x11U, 0xB1U, 0x0BU, 0xA2U, 0x5DU, 0xE2U, 0xE8U, 0x26U, 0x93U, 0x63U, 0xD9U, 0x81U,
	0xFAU, 0x6FU, 0x88U, 0xECU, 0x62U, 0xB8U, 0x01U, 0x80U, 0x00U, 0x02U, 0x00U, 0x00U, 0x06U, 0xC4U, 0x2EU, 0x85U,
	0xDEU, 0x2EU, 0x82U, 0x9AU, 0x4DU, 0x8FU, 0x66U, 0x07U, 0xE6U, 0xF8U, 0x8EU, 0xC6U, 0x8AU, 0xE0U, 0x04U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x1CU, 0x6EU, 0x10U, 0xBAU, 0x17U, 0x78U, 0xBAU, 0x09U, 0xA4U, 0xD8U, 0xF6U, 0x98U, 0x1FU,
	0x9BU, 0xE2U, 0x3BU, 0x18U, 0xAEU, 0x00U, 0x62U, 0x00U, 0x00U, 0x0FU, 0x3AU, 0x14U, 0xE9U, 0x4DU, 0x8CU, 0xCEU,
	0xE7U, 0x52U, 0x17U, 0x28U, 0xD6U, 0x45U, 0x77U, 0xF0U, 0xD2U, 0x23U, 0x25U, 0x6BU, 0x00U, 0x00U, 0x27U, 0x6CU,
	0xF2U, 0x86U, 0x46U, 0x45U, 0xE0U, 0x1AU, 0x90U, 0xD1U, 0x27U, 0x00U, 0x0AU, 0xD0U, 0x02U, 0x67U, 0x6AU, 0x13U,
	0x36U, 0xF9U, 0x4AU, 0xFEU, 0xFEU, 0x2AU, 0x8CU, 0x41U, 0xC3U, 0xCFU, 0x94U, 0x30U, 0x56U, 0xEBU, 0x10U, 0xC0U,
	0x8AU, 0xEFU, 0x5FU, 0x14U, 0xB3U, 0x82U, 0xE7U, 0xC4U, 0x4FU, 0x55U, 0x18U, 0xDEU, 0x33U, 0xDCU, 0x7EU, 0x69U,
	0x71U, 0xFFU, 0xB1U, 0xE9U, 0x10U, 0x27U, 0xD8U, 0x1AU, 0x28U, 0xF4U, 0xBFU, 0xB6U, 0x85U, 0x9AU, 0x12U, 0xB7U,
	0x92U, 0x42U, 0x33U, 0xB9U, 0x55U, 0xE0U, 0x8FU, 0x22U, 0x2FU, 0xD6U, 0x2FU, 0x71U, 0x07U, 0x61U, 0x38U, 0xFDU,
	0xF8U, 0x22U, 0xB2U, 0x11U, 0xCBU, 0x8FU, 0x3EU, 0x69U, 0x88U, 0x6FU, 0xDDU, 0x22U, 0x00U, 0x00U },

	{ 0xE0U, 0xDCU, 0x31U, 0x01U, 0x55U, 0x75U, 0xF5U, 0xFEU, 0x77U, 0xFFU, 0x29U, 0x3AU, 0xBAU, 0xA4U, 0xEFU, 0xB0U,
	0x9AU, 0x8AU, 0xCEU, 0x9DU, 0xADU, 0x1EU, 0xCDU, 0x06U, 0xFBU, 0xF6U, 0xA8U, 0x28U, 0xEDU, 0x7BU, 0x5AU, 0x8CU,
	0x15U, 0xC7U, 0x08U, 0xEEU, 0x38U, 0xD1U, 0x4BU, 0xEEU, 0xA5U, 0xE7U, 0xE0U, 0xD5U, 0x5DU, 0x49U, 0x05U, 0x0DU,
	0x72U, 0xFAU, 0xB0U, 0xD4U, 0xA6U, 0x08U, 0x64U, 0x40U, 0x00U, 0x02U, 0x00U, 0x00U, 0x07U, 0xA3U, 0x36U, 0x5CU,
	0x96U, 0x5AU, 0x26U, 0xFDU, 0xD6U, 0x35U, 0x64U, 0x11U, 0x82U, 0x0CU, 0x8DU, 0xFAU, 0x75U, 0xF6U, 0xD7U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x72U, 0x20U, 0x56U, 0x60U, 0xA5U, 0xEBU, 0x24U, 0x6BU, 0xF6U, 0x1AU, 0x83U, 0xECU,
	0xA9U, 0x39U, 0xACU, 0x7DU, 0x44U, 0xC4U, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x4AU, 0x00U, 0xDAU, 0x66U,
	0xF2U, 0x8FU, 0x3BU, 0xBAU, 0xA5U, 0x8EU, 0x9BU, 0xC4U, 0x0EU, 0x49U, 0x17U, 0xE9U, 0x5AU, 0x14U, 0xA0U, 0xE0U,
	0x00U, 0x02U, 0x00U, 0x03U, 0x75U, 0xD6U, 0xB9U, 0x6CU, 0x54U, 0x24U, 0x56U, 0x5CU, 0xACU, 0x5AU, 0x81U, 0xA5U,
	0x9BU, 0x06U, 0x47U, 0x96U, 0xE9U, 0x49U, 0xAEU, 0x8BU, 0x48U, 0xB6U, 0x49U, 0x9CU, 0x42U, 0xD8U, 0xA1U, 0x5DU,
	0x58U, 0xC0U, 0x70U, 0xC4U, 0xBAU, 0xEEU, 0x39U, 0x99U, 0xC8U, 0x2EU, 0xFFU, 0x31U, 0x37U, 0xA9U, 0xAAU, 0xF4U,
	0xF1U, 0xFDU, 0x63U, 0x0BU, 0xC2U, 0x71U, 0xD9U, 0x12U, 0xA7U, 0x35U, 0x13U, 0x15U, 0xBAU, 0x31U, 0x1EU, 0xAEU,
	0x6EU, 0xC7U, 0x3CU, 0x70U, 0x96U, 0x7CU, 0x41U, 0x1CU, 0x1CU, 0xE2U, 0xA2U, 0x44U, 0x59U, 0xE8U, 0x46U, 0x88U,
	0x3AU, 0x70U, 0xFEU, 0xD3U, 0xD3U, 0x13U, 0x44U, 0xA1U, 0x67U, 0xECU, 0xF8U, 0xAEU },

	{ 0xE0U, 0xDCU, 0x31U, 0x01U, 0x55U, 0x75U, 0xF5U, 0xFEU, 0x77U, 0xFFU, 0x29U, 0x35U, 0x56U, 0x7BU, 0xCBU, 0x19U,
	0x4DU, 0x0DU, 0xFDU, 0x2AU, 0x9AU, 0x12U, 0x2BU, 0x2AU, 0xE1U, 0xB0U, 0xA1U, 0xDBU, 0x88U, 0x26U, 0x82U, 0x5FU,
	0x40U, 0xFEU, 0x15U, 0x3DU, 0x20U, 0x92U, 0x1BU, 0x2EU, 0x49U, 0xF3U, 0xACU, 0x3FU, 0xB3U, 0x8DU, 0x39U, 0x2AU,
	0xF6U, 0x09U, 0x14U, 0x58U, 0xF1U, 0xE7U, 0x23U, 0x80U, 0x00U, 0x02U, 0x00U, 0x00U, 0x04U, 0x23U, 0x01U, 0xC3U,
	0x59U, 0x60U, 0xFAU, 0xD9U, 0xE1U, 0x12U, 0x6DU, 0x32U, 0x23U, 0x44U, 0x86U, 0xEEU, 0x11U, 0x87U, 0x68U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x1CU, 0x52U, 0xB4U, 0xD1U, 0x4DU, 0x55U, 0x33U, 0x6EU, 0x63U, 0x92U, 0x42U, 0x8CU, 0xB1U,
	0x9BU, 0x37U, 0xE9U, 0x42U, 0x9BU, 0x2DU, 0xA2U, 0x00U, 0x00U, 0x0FU, 0x3AU, 0x17U, 0x2EU, 0xDBU, 0x20U, 0x0EU,
	0x55U, 0xA3U, 0x6FU, 0x53U, 0xF1U, 0x25U, 0x31U, 0x3EU, 0xDEU, 0x12U, 0x42U, 0x69U, 0xA3U, 0x42U, 0x67U, 0x6CU,
	0xF2U, 0x86U, 0x46U, 0x4DU, 0x6AU, 0x91U, 0x60U, 0x8FU, 0x64U, 0xAAU, 0x1EU, 0xCAU, 0x52U, 0xA0U, 0x60U, 0x49U,
	0xA9U, 0x0CU, 0x70U, 0xCAU, 0x4EU, 0x3BU, 0x8CU, 0x41U, 0xC3U, 0xCFU, 0x90U, 0x80U, 0xBAU, 0x5EU, 0x59U, 0x39U,
	0x21U, 0x12U, 0x19U, 0x3FU, 0xEFU, 0x4AU, 0x2CU, 0xA6U, 0x36U, 0x1AU, 0x07U, 0xCBU, 0xFEU, 0x4CU, 0x7EU, 0x69U,
	0x71U, 0xFFU, 0xB1U, 0xF1U, 0xC7U, 0xE9U, 0x42U, 0x86U, 0xCFU, 0xE9U, 0x48U, 0xC0U, 0xAEU, 0xA4U, 0x58U, 0x97U,
	0xA6U, 0xE6U, 0x71U, 0x82U, 0x52U, 0xEAU, 0x90U, 0xAAU, 0x90U, 0x82U, 0x67U, 0x51U, 0xB5U, 0x5AU, 0x12U, 0xB8U,
	0x1BU, 0xFDU, 0x8AU, 0xBAU, 0xCBU, 0x93U, 0x21U, 0xEEU, 0xA3U, 0x60U, 0x46U, 0x66U },

	{ 0xE0U, 0xDCU, 0x31U, 0x01U, 0x55U, 0x75U, 0xF5U, 0xEFU, 0x77U, 0xFFU, 0x29U, 0x3AU, 0xBAU, 0xA4U, 0xEEU, 0xB0U,
	0x9AU, 0x8AU, 0xC5U, 0x11U, 0xD1U, 0xA2U, 0x0CU, 0x58U, 0xF4U, 0x8DU, 0xC8U, 0xF1U, 0xEDU, 0xA6U, 0x6EU, 0x2CU,
	0x67U, 0xA9U, 0x57U, 0xD1U, 0x38U, 0x02U, 0x91U, 0x2AU, 0xEBU, 0x63U, 0x28U, 0x49U, 0x1AU, 0xAFU, 0xA9U, 0xC6U,
	0xBAU, 0x3CU, 0xD3U, 0x61U, 0x2DU, 0x13U, 0x31U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x05U, 0x2BU, 0x48U, 0x4DU,
	0x8DU, 0x7EU, 0x82U, 0x7DU, 0xECU, 0x39U, 0x61U, 0x82U, 0x1DU, 0x3EU, 0xBCU, 0xF2U, 0x66U, 0x8BU, 0x02U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x06U, 0x31U, 0x30U, 0x79U, 0xF1U, 0xBBU, 0x5AU, 0x5AU, 0x51U, 0xB6U, 0x74U, 0x17U,
	0xEEU, 0x3EU, 0xA8U, 0x2BU, 0xA0U, 0xC3U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xCAU, 0x9CU, 0xA2U, 0x2AU,
	0x82U, 0xC7U, 0x03U, 0x4DU, 0xF1U, 0xE8U, 0xF1U, 0x20U, 0xAEU, 0xA9U, 0xC4U, 0xE9U, 0x1CU, 0xC0U, 0xA0U, 0xE0U,
	0x00U, 0x02U, 0x00U, 0x05U, 0x4AU, 0x05U, 0x38U, 0xE3U, 0x4EU, 0xEAU, 0x92U, 0x7AU, 0x51U, 0x23U, 0x32U, 0x12U,
	0xDEU, 0x69U, 0x0BU, 0x42U, 0x3AU, 0x85U, 0xAEU, 0x8BU, 0x48U, 0xB6U, 0x49U, 0x0AU, 0x36U, 0x16U, 0x9DU, 0xFDU,
	0x51U, 0x67U, 0xD0U, 0x76U, 0x44U, 0x6AU, 0x6DU, 0x4AU, 0x22U, 0x47U, 0xA1U, 0x91U, 0xEAU, 0xA9U, 0xAAU, 0xF4U,
	0xF1U, 0xFDU, 0x62U, 0x9FU, 0x1BU, 0x80U, 0xAFU, 0x4EU, 0xEEU, 0x4DU, 0x48U, 0x7CU, 0x99U, 0x41U, 0xA9U, 0xFAU,
	0x62U, 0xEFU, 0x9DU, 0x79U, 0x92U, 0x6AU, 0x90U, 0xA0U, 0x8FU, 0x2AU, 0xDDU, 0x20U, 0x70U, 0x62U, 0x1CU, 0xF1U,
	0x5FU, 0x71U, 0x4EU, 0xC0U, 0x54U, 0x80U, 0xE2U, 0x76U, 0x71U, 0xA9U, 0x30U, 0xCAU },

	{ 0xE0U, 0xDCU, 0x31U, 0x01U, 0x55U, 0x75U, 0xF5U, 0xFEU, 0x77U, 0xFFU, 0x29U, 0x33U, 0x3AU, 0x5DU, 0xDCU, 0xA3U,
	0x3BU, 0x5BU, 0x80U, 0x00U, 0x00U, 0x02U, 0x16U, 0x60U, 0x02U, 0x80U, 0x08U, 0x00U, 0x20U, 0x20U, 0x00U, 0x80U,
	0x00U, 0x00U, 0x00U, 0x80U, 0x00U, 0x00U, 0x25U, 0x88U, 0x00U, 0x28U, 0x22U, 0x20U, 0x01U, 0xB3U, 0x81U, 0x6AU,
	0x95U, 0x16U, 0x61U, 0x23U, 0xE7U, 0x20U, 0x00U, 0xD8U, 0xDFU, 0xE9U, 0xACU, 0xC9U, 0x07U, 0x69U, 0xB6U, 0x41U,
	0x92U, 0x04U, 0x63U, 0x4AU, 0xF4U, 0xA1U, 0x6FU, 0xF4U, 0xA7U, 0xEAU, 0xACU, 0xE6U, 0xD8U, 0x71U, 0xE3U, 0xE6U,
	0x95U, 0x2DU, 0x3CU, 0xBAU, 0x64U, 0x9EU, 0x97U, 0xF3U, 0x4AU, 0x66U, 0x85U, 0x40U, 0x73U, 0xDCU, 0x28U, 0x83U,
	0x7AU, 0x71U, 0xC7U, 0x01U, 0x5DU, 0xFCU, 0x19U, 0xBAU, 0x61U, 0xD1U, 0xCAU, 0xC7U, 0x82U, 0x96U, 0xF7U, 0x29U,
	0x7BU, 0xD2U, 0xB0U, 0xFBU, 0x21U, 0x13U, 0x31U, 0x54U, 0xFFU, 0x1BU, 0x50U, 0x0DU, 0xD7U, 0x68U, 0x76U, 0xCBU,
	0xA5U, 0xA7U, 0x86U, 0x5EU, 0xE0U, 0x21U, 0x45U, 0x7AU, 0x31U, 0x3EU, 0x50U, 0xAEU, 0x2AU, 0x30U, 0x80U, 0x25U,
	0xA9U, 0x9BU, 0x7DU, 0x4AU, 0x0EU, 0x17U, 0x92U, 0x2AU, 0xA6U, 0xD4U, 0xB2U, 0x60U, 0xCAU, 0xC7U, 0x9AU, 0x5EU,
	0x0AU, 0x14U, 0xD3U, 0xE2U, 0x35U, 0x04U, 0x48U, 0xBAU, 0xE6U, 0x44U, 0xE9U, 0x2EU, 0x73U, 0xB8U, 0xAEU, 0xD8U,
	0xA9U, 0x66U, 0x0DU, 0xA4U, 0x8AU, 0xB2U, 0x5FU, 0xE6U, 0x01U, 0x97U, 0xF4U, 0x1AU, 0x33U, 0x09U, 0x60U, 0x44U,
	0x8EU, 0xA6U, 0xB5U, 0xACU, 0x43U, 0x50U, 0xAFU, 0x5BU, 0xF6U, 0x42U, 0xA3U, 0x27U, 0x01U, 0x44U, 0xAFU, 0x7AU,
	0xB1U, 0x05U, 0x01U, 0x9FU, 0xA4U, 0xFDU, 0x2DU, 0x8FU, 0x48U, 0xA3U, 0xC1U, 0xA3U } };


const unsigned char MMDVM_FRAME_START = 0xE0U;

const unsigned char MMDVM_GET_VERSION = 0x00U;
const unsigned char MMDVM_GET_STATUS  = 0x01U;
const unsigned char MMDVM_SET_CONFIG  = 0x02U;
const unsigned char MMDVM_SET_MODE    = 0x03U;
const unsigned char MMDVM_SET_FREQ    = 0x04U;

const unsigned char MMDVM_SEND_CWID   = 0x0AU;

const unsigned char MMDVM_DSTAR_HEADER = 0x10U;
const unsigned char MMDVM_DSTAR_DATA   = 0x11U;
const unsigned char MMDVM_DSTAR_LOST   = 0x12U;
const unsigned char MMDVM_DSTAR_EOT    = 0x13U;

const unsigned char MMDVM_DMR_DATA1   = 0x18U;
const unsigned char MMDVM_DMR_LOST1   = 0x19U;
const unsigned char MMDVM_DMR_DATA2   = 0x1AU;
const unsigned char MMDVM_DMR_LOST2   = 0x1BU;
const unsigned char MMDVM_DMR_SHORTLC = 0x1CU;
const unsigned char MMDVM_DMR_START   = 0x1DU;
const unsigned char MMDVM_DMR_ABORT   = 0x1EU;

const unsigned char MMDVM_YSF_DATA    = 0x20U;
const unsigned char MMDVM_YSF_LOST    = 0x21U;

const unsigned char MMDVM_P25_HDR     = 0x30U;
const unsigned char MMDVM_P25_LDU     = 0x31U;
const unsigned char MMDVM_P25_LOST    = 0x32U;

const unsigned char MMDVM_ACK         = 0x70U;
const unsigned char MMDVM_NAK         = 0x7FU;

const unsigned char MMDVM_DEBUG1      = 0xF1U;
const unsigned char MMDVM_DEBUG2      = 0xF2U;
const unsigned char MMDVM_DEBUG3      = 0xF3U;
const unsigned char MMDVM_DEBUG4      = 0xF4U;
const unsigned char MMDVM_DEBUG5      = 0xF5U;

const unsigned int MAX_RESPONSES = 30U;

const unsigned int BUFFER_LENGTH = 500U;


CModem::CModem(const std::string& port, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, int oscOffset, bool debug) :
m_port(port),
m_colorCode(0U),
m_duplex(duplex),
m_rxInvert(rxInvert),
m_txInvert(txInvert),
m_pttInvert(pttInvert),
m_txDelay(txDelay),
m_dmrDelay(dmrDelay),
m_rxLevel(0U),
m_dstarTXLevel(0U),
m_dmrTXLevel(0U),
m_ysfTXLevel(0U),
m_p25TXLevel(0U),
m_oscOffset(oscOffset),
m_debug(debug),
m_rxFrequency(0U),
m_txFrequency(0U),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_ysfEnabled(false),
m_p25Enabled(false),
m_serial(port, SERIAL_115200, true),
m_buffer(NULL),
m_length(0U),
m_offset(0U),
m_rxDStarData(1000U, "Modem RX D-Star"),
m_txDStarData(1000U, "Modem TX D-Star"),
m_rxDMRData1(1000U, "Modem RX DMR1"),
m_rxDMRData2(1000U, "Modem RX DMR2"),
m_txDMRData1(1000U, "Modem TX DMR1"),
m_txDMRData2(1000U, "Modem TX DMR2"),
m_rxYSFData(1000U, "Modem RX YSF"),
m_txYSFData(1000U, "Modem TX YSF"),
m_rxP25Data(1000U, "Modem RX P25"),
m_txP25Data(1000U, "Modem TX P25"),
m_statusTimer(1000U, 0U, 250U),
m_inactivityTimer(1000U, 2U),
m_playoutTimer(1000U, 0U, 10U),
m_dstarSpace(0U),
m_dmrSpace1(0U),
m_dmrSpace2(0U),
m_ysfSpace(0U),
m_p25Space(0U),
m_tx(false),
m_lockout(false),
m_error(false),
m_hwType(HWT_UNKNOWN),
m_nn(0U)
{
	assert(!port.empty());

	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CModem::~CModem()
{
	delete[] m_buffer;
}

void CModem::setRFParams(unsigned int rxFrequency, unsigned int txFrequency)
{
	m_rxFrequency = rxFrequency;
	m_txFrequency = txFrequency;
}

void CModem::setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled)
{
	m_dstarEnabled = dstarEnabled;
	m_dmrEnabled   = dmrEnabled;
	m_ysfEnabled   = ysfEnabled;
	m_p25Enabled   = p25Enabled;
}

void CModem::setLevels(unsigned int rxLevel, unsigned int dstarTXLevel, unsigned int dmrTXLevel, unsigned int ysfTXLevel, unsigned int p25TXLevel)
{
	m_rxLevel      = rxLevel;
	m_dstarTXLevel = dstarTXLevel;
	m_dmrTXLevel   = dmrTXLevel;
	m_ysfTXLevel   = ysfTXLevel;
	m_p25TXLevel   = p25TXLevel;
}

void CModem::setDMRParams(unsigned int colorCode)
{
	assert(colorCode < 16U);

	m_colorCode = colorCode;
}

bool CModem::open()
{
	::LogMessage("Opening the MMDVM");

	bool ret = m_serial.open();
	if (!ret)
		return false;

	ret = readVersion();
	if (!ret) {
		m_serial.close();
		return false;
	} else {
		/* Stopping the inactivity timer here when a firmware version has been
		   successfuly read prevents the death spiral of "no reply from modem..." */
		m_inactivityTimer.stop();
	}

	ret = setFrequency();
	if (!ret) {
		m_serial.close();
		return false;
	}

	ret = setConfig();
	if (!ret) {
		m_serial.close();
		return false;
	}

	m_statusTimer.start();

	m_error  = false;
	m_offset = 0U;

	return true;
}

void CModem::clock(unsigned int ms)
{
	// Poll the modem status every 250ms
	m_statusTimer.clock(ms);
	if (m_statusTimer.hasExpired()) {
		readStatus();
		m_statusTimer.start();

#ifdef notdef
		const unsigned char* dat = P25_DATA[m_nn];
		if (m_nn == 0U) {
			unsigned char data = 101U;
			m_rxP25Data.addData(&data, 1U);

			data = TAG_HEADER;
			m_rxP25Data.addData(&data, 1U);

			m_rxP25Data.addData(dat + 3U, 100U);
		} else if (m_nn > 0U && m_nn < 6U) {
			unsigned char data = 220U;
			m_rxP25Data.addData(&data, 1U);

			data = TAG_DATA;
			m_rxP25Data.addData(&data, 1U);

			m_rxP25Data.addData(dat + 3U, 219U);
		}

		m_nn++;
#endif
	}

	m_inactivityTimer.clock(ms);
	if (m_inactivityTimer.hasExpired()) {
		LogError("No reply from the modem for some time, resetting it");
		m_error = true;
		close();

		CThread::sleep(2000U);		// 2s
		while (!open())
			CThread::sleep(5000U);	// 5s
	}

	RESP_TYPE_MMDVM type = getResponse();

	if (type == RTM_TIMEOUT) {
		// Nothing to do
	} else if (type == RTM_ERROR) {
		// Nothing to do
	} else {
		// type == RTM_OK
		switch (m_buffer[2U]) {
			case MMDVM_DSTAR_HEADER: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star Header", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_HEADER;
					m_rxDStarData.addData(&data, 1U);

					m_rxDStarData.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DSTAR_DATA: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star Data", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_DATA;
					m_rxDStarData.addData(&data, 1U);

					m_rxDStarData.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DSTAR_LOST: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star Lost", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDStarData.addData(&data, 1U);
				}
				break;

			case MMDVM_DSTAR_EOT: {
					if (m_debug)
						CUtils::dump(1U, "RX D-Star EOT", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDStarData.addData(&data, 1U);

					data = TAG_EOT;
					m_rxDStarData.addData(&data, 1U);
				}
				break;

			case MMDVM_DMR_DATA1: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Data 1", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDMRData1.addData(&data, 1U);

					if (m_buffer[3U] == (DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC))
						data = TAG_EOT;
					else
						data = TAG_DATA;
					m_rxDMRData1.addData(&data, 1U);

					m_rxDMRData1.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DMR_DATA2: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Data 2", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxDMRData2.addData(&data, 1U);

					if (m_buffer[3U] == (DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC))
						data = TAG_EOT;
					else
						data = TAG_DATA;
					m_rxDMRData2.addData(&data, 1U);

					m_rxDMRData2.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_DMR_LOST1: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Lost 1", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDMRData1.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDMRData1.addData(&data, 1U);
				}
				break;

			case MMDVM_DMR_LOST2: {
					if (m_debug)
						CUtils::dump(1U, "RX DMR Lost 2", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxDMRData2.addData(&data, 1U);

					data = TAG_LOST;
					m_rxDMRData2.addData(&data, 1U);
				}
				break;

			case MMDVM_YSF_DATA: {
					if (m_debug)
						CUtils::dump(1U, "RX YSF Data", m_buffer, m_length);

					unsigned char data = m_length - 2U;
					m_rxYSFData.addData(&data, 1U);

					data = TAG_DATA;
					m_rxYSFData.addData(&data, 1U);

					m_rxYSFData.addData(m_buffer + 3U, m_length - 3U);
				}
				break;

			case MMDVM_YSF_LOST: {
					if (m_debug)
						CUtils::dump(1U, "RX YSF Lost", m_buffer, m_length);

					unsigned char data = 1U;
					m_rxYSFData.addData(&data, 1U);

					data = TAG_LOST;
					m_rxYSFData.addData(&data, 1U);
				}
				break;

			case MMDVM_P25_HDR: {
				if (m_debug)
					CUtils::dump(1U, "RX P25 Header", m_buffer, m_length);

				unsigned char data = m_length - 2U;
				m_rxP25Data.addData(&data, 1U);

				data = TAG_HEADER;
				m_rxP25Data.addData(&data, 1U);

				m_rxP25Data.addData(m_buffer + 3U, m_length - 3U);
			}
			break;

			case MMDVM_P25_LDU: {
				if (m_debug)
					CUtils::dump(1U, "RX P25 LDU", m_buffer, m_length);

				unsigned char data = m_length - 2U;
				m_rxP25Data.addData(&data, 1U);

				data = TAG_DATA;
				m_rxP25Data.addData(&data, 1U);

				m_rxP25Data.addData(m_buffer + 3U, m_length - 3U);
			}
			break;

			case MMDVM_P25_LOST: {
				if (m_debug)
					CUtils::dump(1U, "RX P25 Lost", m_buffer, m_length);

				unsigned char data = 1U;
				m_rxP25Data.addData(&data, 1U);

				data = TAG_LOST;
				m_rxP25Data.addData(&data, 1U);
			}
			break;

			case MMDVM_GET_STATUS: {
					// if (m_debug)
					//	CUtils::dump(1U, "GET_STATUS", m_buffer, m_length);

					m_tx = (m_buffer[5U] & 0x01U) == 0x01U;

					bool adcOverflow = (m_buffer[5U] & 0x02U) == 0x02U;
					if (adcOverflow)
						LogError("MMDVM ADC levels have overflowed");

					bool rxOverflow = (m_buffer[5U] & 0x04U) == 0x04U;
					if (rxOverflow)
						LogError("MMDVM RX buffer has overflowed");

					bool txOverflow = (m_buffer[5U] & 0x08U) == 0x08U;
					if (txOverflow)
						LogError("MMDVM TX buffer has overflowed");

					m_lockout = (m_buffer[5U] & 0x10U) == 0x10U;

					bool dacOverflow = (m_buffer[5U] & 0x20U) == 0x20U;
					if (dacOverflow)
						LogError("MMDVM DAC levels have overflowed");

					m_dstarSpace = m_buffer[6U];
					m_dmrSpace1  = m_buffer[7U];
					m_dmrSpace2  = m_buffer[8U];
					m_ysfSpace   = m_buffer[9U];
					m_p25Space   = m_buffer[10U];

					m_inactivityTimer.start();
					// LogMessage("status=%02X, tx=%d, space=%u,%u,%u,%u,%u lockout=%d", m_buffer[5U], int(m_tx), m_dstarSpace, m_dmrSpace1, m_dmrSpace2, m_ysfSpace, m_p25Space, int(m_lockout));
				}
				break;

			// These should not be received, but don't complain if we do
			case MMDVM_GET_VERSION:
			case MMDVM_ACK:
				break;

			case MMDVM_NAK:
				LogWarning("Received a NAK from the MMDVM, command = 0x%02X, reason = %u", m_buffer[3U], m_buffer[4U]);
				break;

			default:
				LogMessage("Unknown message, type: %02X", m_buffer[2U]);
				CUtils::dump("Buffer dump", m_buffer, m_length);
				break;
		}
	}

	// Only feed data to the modem if the playout timer has expired
	m_playoutTimer.clock(ms);
	if (!m_playoutTimer.hasExpired())
		return;

	if (m_dstarSpace > 1U && !m_txDStarData.isEmpty()) {
		unsigned char buffer[4U];
		m_txDStarData.peek(buffer, 4U);

		if ((buffer[3U] == MMDVM_DSTAR_HEADER && m_dstarSpace > 4U) ||
			(buffer[3U] == MMDVM_DSTAR_DATA   && m_dstarSpace > 1U) ||
			(buffer[3U] == MMDVM_DSTAR_EOT    && m_dstarSpace > 1U)) {
			unsigned char len = 0U;
			m_txDStarData.getData(&len, 1U);
			m_txDStarData.getData(m_buffer, len);

			switch (buffer[3U]) {
			case MMDVM_DSTAR_HEADER:
				if (m_debug)
					CUtils::dump(1U, "TX D-Star Header", m_buffer, len);
				m_dstarSpace -= 4U;
				break;
			case MMDVM_DSTAR_DATA:
				if (m_debug)
					CUtils::dump(1U, "TX D-Star Data", m_buffer, len);
				m_dstarSpace -= 1U;
				break;
			default:
				if (m_debug)
					CUtils::dump(1U, "TX D-Star EOT", m_buffer, len);
				m_dstarSpace -= 1U;
				break;
			}

			int ret = m_serial.write(m_buffer, len);
			if (ret != int(len))
				LogWarning("Error when writing D-Star data to the MMDVM");

			m_playoutTimer.start();
		}
	}

	if (m_dmrSpace1 > 1U && !m_txDMRData1.isEmpty()) {
		unsigned char len = 0U;
		m_txDMRData1.getData(&len, 1U);
		m_txDMRData1.getData(m_buffer, len);

		if (m_debug)
			CUtils::dump(1U, "TX DMR Data 1", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing DMR data to the MMDVM");

		m_playoutTimer.start();

		m_dmrSpace1--;
	}

	if (m_dmrSpace2 > 1U && !m_txDMRData2.isEmpty()) {
		unsigned char len = 0U;
		m_txDMRData2.getData(&len, 1U);
		m_txDMRData2.getData(m_buffer, len);

		if (m_debug)
			CUtils::dump(1U, "TX DMR Data 2", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing DMR data to the MMDVM");

		m_playoutTimer.start();

		m_dmrSpace2--;
	}

	if (m_ysfSpace > 1U && !m_txYSFData.isEmpty()) {
		unsigned char len = 0U;
		m_txYSFData.getData(&len, 1U);
		m_txYSFData.getData(m_buffer, len);

		if (m_debug)
			CUtils::dump(1U, "TX YSF Data", m_buffer, len);

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing YSF data to the MMDVM");

		m_playoutTimer.start();

		m_ysfSpace--;
	}

	if (m_p25Space > 1U && !m_txP25Data.isEmpty()) {
		unsigned char len = 0U;
		m_txP25Data.getData(&len, 1U);
		m_txP25Data.getData(m_buffer, len);

		if (m_debug) {
			if (m_buffer[3U] == MMDVM_P25_HDR)
				CUtils::dump(1U, "TX P25 HDR", m_buffer, len);
			else
				CUtils::dump(1U, "TX P25 LDU", m_buffer, len);
		}

		int ret = m_serial.write(m_buffer, len);
		if (ret != int(len))
			LogWarning("Error when writing P25 data to the MMDVM");

		m_playoutTimer.start();

		m_p25Space--;
	}
}

void CModem::close()
{
	::LogMessage("Closing the MMDVM");

	m_serial.close();
}

unsigned int CModem::readDStarData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDStarData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDStarData.getData(&len, 1U);
	m_rxDStarData.getData(data, len);

	return len;
}

unsigned int CModem::readDMRData1(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDMRData1.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDMRData1.getData(&len, 1U);
	m_rxDMRData1.getData(data, len);

	return len;
}

unsigned int CModem::readDMRData2(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxDMRData2.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxDMRData2.getData(&len, 1U);
	m_rxDMRData2.getData(data, len);

	return len;
}

unsigned int CModem::readYSFData(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxYSFData.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxYSFData.getData(&len, 1U);
	m_rxYSFData.getData(data, len);

	return len;
}

unsigned int CModem::readP25Data(unsigned char* data)
{
	assert(data != NULL);

	if (m_rxP25Data.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_rxP25Data.getData(&len, 1U);
	m_rxP25Data.getData(data, len);

	return len;
}

bool CModem::hasDStarSpace() const
{
	unsigned int space = m_txDStarData.freeSpace() / (DSTAR_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeDStarData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;

	switch (data[0U]) {
		case TAG_HEADER:
			buffer[2U] = MMDVM_DSTAR_HEADER;
			break;
		case TAG_DATA:
			buffer[2U] = MMDVM_DSTAR_DATA;
			break;
		case TAG_EOT:
			buffer[2U] = MMDVM_DSTAR_EOT;
			break;
		default:
			CUtils::dump(2U, "Unknown D-Star packet type", data, length);
			return false;
	}

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txDStarData.addData(&len, 1U);
	m_txDStarData.addData(buffer, len);

	return true;
}

bool CModem::hasDMRSpace1() const
{
	unsigned int space = m_txDMRData1.freeSpace() / (DMR_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::hasDMRSpace2() const
{
	unsigned int space = m_txDMRData2.freeSpace() / (DMR_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeDMRData1(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[40U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = MMDVM_DMR_DATA1;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txDMRData1.addData(&len, 1U);
	m_txDMRData1.addData(buffer, len);

	return true;
}

bool CModem::writeDMRData2(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[40U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = MMDVM_DMR_DATA2;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txDMRData2.addData(&len, 1U);
	m_txDMRData2.addData(buffer, len);

	return true;
}

bool CModem::hasYSFSpace() const
{
	unsigned int space = m_txYSFData.freeSpace() / (YSF_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeYSFData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[130U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = MMDVM_YSF_DATA;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txYSFData.addData(&len, 1U);
	m_txYSFData.addData(buffer, len);

	return true;
}

bool CModem::hasP25Space() const
{
	unsigned int space = m_txP25Data.freeSpace() / (P25_LDU_FRAME_LENGTH_BYTES + 4U);

	return space > 1U;
}

bool CModem::writeP25Data(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (data[0U] != TAG_HEADER && data[0U] != TAG_DATA && data[0U] != TAG_EOT)
		return false;

	unsigned char buffer[250U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 2U;
	buffer[2U] = (data[0U] == TAG_HEADER) ? MMDVM_P25_HDR : MMDVM_P25_LDU;

	::memcpy(buffer + 3U, data + 1U, length - 1U);

	unsigned char len = length + 2U;
	m_txP25Data.addData(&len, 1U);
	m_txP25Data.addData(buffer, len);

	return true;
}

bool CModem::hasTX() const
{
	return m_tx;
}

bool CModem::hasLockout() const
{
	return m_lockout;
}

bool CModem::hasError() const
{
	return m_error;
}

bool CModem::readVersion()
{
	CThread::sleep(2000U);	// 2s

	for (unsigned int i = 0U; i < 6U; i++) {
		unsigned char buffer[3U];

		buffer[0U] = MMDVM_FRAME_START;
		buffer[1U] = 3U;
		buffer[2U] = MMDVM_GET_VERSION;

		// CUtils::dump(1U, "Written", buffer, 3U);

		int ret = m_serial.write(buffer, 3U);
		if (ret != 3)
			return false;

		for (unsigned int count = 0U; count < MAX_RESPONSES; count++) {
			CThread::sleep(10U);
			RESP_TYPE_MMDVM resp = getResponse();
			if (resp == RTM_OK && m_buffer[2U] == MMDVM_GET_VERSION) {
				if (::memcmp(m_buffer + 4U, "MMDVM", 5U) == 0)
					m_hwType = HWT_MMDVM;
				else if (::memcmp(m_buffer + 4U, "DVMEGA", 6U) == 0)
					m_hwType = HWT_DVMEGA;

				LogInfo("MMDVM protocol version: %u, description: %.*s", m_buffer[3U], m_length - 4U, m_buffer + 4U);
				return true;
			}
		}

		CThread::sleep(1000U);
	}

	LogError("Unable to read the firmware version after six attempts");

	return false;
}

bool CModem::readStatus()
{
	unsigned char buffer[3U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 3U;
	buffer[2U] = MMDVM_GET_STATUS;

	// CUtils::dump(1U, "Written", buffer, 3U);

	return m_serial.write(buffer, 3U) == 3;
}

bool CModem::setConfig()
{
	unsigned char buffer[20U];

	buffer[0U] = MMDVM_FRAME_START;

	buffer[1U] = 16U;

	buffer[2U] = MMDVM_SET_CONFIG;

	buffer[3U] = 0x00U;
	if (m_rxInvert)
		buffer[3U] |= 0x01U;
	if (m_txInvert)
		buffer[3U] |= 0x02U;
	if (m_pttInvert)
		buffer[3U] |= 0x04U;
	if (!m_duplex)
		buffer[3U] |= 0x80U;

	buffer[4U] = 0x00U;
	if (m_dstarEnabled)
		buffer[4U] |= 0x01U;
	if (m_dmrEnabled)
		buffer[4U] |= 0x02U;
	if (m_ysfEnabled)
		buffer[4U] |= 0x04U;
	if (m_p25Enabled)
		buffer[4U] |= 0x08U;

	buffer[5U] = m_txDelay / 10U;		// In 10ms units

	buffer[6U] = MODE_IDLE;

	buffer[7U] = (m_rxLevel * 255U) / 100U;
	buffer[8U] = (m_dstarTXLevel * 255U) / 100U;		// For backwards compatibility

	buffer[9U] = m_colorCode;

	buffer[10U] = m_dmrDelay;

	buffer[11U] = (unsigned char)(m_oscOffset + 128);

	buffer[12U] = (m_dstarTXLevel * 255U) / 100U;
	buffer[13U] = (m_dmrTXLevel * 255U) / 100U;
	buffer[14U] = (m_ysfTXLevel * 255U) / 100U;
	buffer[15U] = (m_p25TXLevel * 255U) / 100U;

	// CUtils::dump(1U, "Written", buffer, 16U);

	int ret = m_serial.write(buffer, 16U);
	if (ret != 16)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		CThread::sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				LogError("The MMDVM is not responding to the SET_CONFIG command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump(1U, "Response", m_buffer, m_length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_CONFIG command from the modem");
		return false;
	}

	m_playoutTimer.start();

	return true;
}

bool CModem::setFrequency()
{
	unsigned char buffer[15U];

	buffer[0U]  = MMDVM_FRAME_START;

	buffer[1U]  = 12U;

	buffer[2U]  = MMDVM_SET_FREQ;

	buffer[3U]  = 0x00U;

	buffer[4U]  = (m_rxFrequency >> 0) & 0xFFU;
	buffer[5U]  = (m_rxFrequency >> 8) & 0xFFU;
	buffer[6U]  = (m_rxFrequency >> 16) & 0xFFU;
	buffer[7U]  = (m_rxFrequency >> 24) & 0xFFU;

	buffer[8U]  = (m_txFrequency >> 0) & 0xFFU;
	buffer[9U]  = (m_txFrequency >> 8) & 0xFFU;
	buffer[10U] = (m_txFrequency >> 16) & 0xFFU;
	buffer[11U] = (m_txFrequency >> 24) & 0xFFU;

	// CUtils::dump(1U, "Written", buffer, 12U);

	int ret = m_serial.write(buffer, 12U);
	if (ret != 12)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		CThread::sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				LogError("The MMDVM is not responding to the SET_FREQ command");
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	// CUtils::dump(1U, "Response", m_buffer, m_length);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		LogError("Received a NAK to the SET_FREQ command from the modem");
		return false;
	}

	return true;
}

RESP_TYPE_MMDVM CModem::getResponse()
{
	if (m_offset == 0U) {
		// Get the start of the frame or nothing at all
		int ret = m_serial.read(m_buffer + 0U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the modem");
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		if (m_buffer[0U] != MMDVM_FRAME_START)
			return RTM_TIMEOUT;

		m_offset = 1U;
	}

	if (m_offset == 1U) {
		// Get the length of the frame
		int ret = m_serial.read(m_buffer + 1U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the modem");
			m_offset = 0U;
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		if (m_buffer[1U] >= 250U) {
			LogError("Invalid length received from the modem - %u", m_buffer[1U]);
			m_offset = 0U;
			return RTM_ERROR;
		}

		m_length = m_buffer[1U];
		m_offset = 2U;
	}

	if (m_offset == 2U) {
		// Get the frame type
		int ret = m_serial.read(m_buffer + 2U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the modem");
			m_offset = 0U;
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		switch (m_buffer[2U]) {
		case MMDVM_DSTAR_HEADER:
		case MMDVM_DSTAR_DATA:
		case MMDVM_DSTAR_LOST:
		case MMDVM_DSTAR_EOT:
		case MMDVM_DMR_DATA1:
		case MMDVM_DMR_DATA2:
		case MMDVM_DMR_LOST1:
		case MMDVM_DMR_LOST2:
		case MMDVM_YSF_DATA:
		case MMDVM_YSF_LOST:
		case MMDVM_P25_HDR:
		case MMDVM_P25_LDU:
		case MMDVM_P25_LOST:
		case MMDVM_GET_STATUS:
		case MMDVM_GET_VERSION:
		case MMDVM_ACK:
		case MMDVM_NAK:
		case MMDVM_DEBUG1:
		case MMDVM_DEBUG2:
		case MMDVM_DEBUG3:
		case MMDVM_DEBUG4:
		case MMDVM_DEBUG5:
			break;

		default:
			LogError("Unknown message, type: %02X", m_buffer[2U]);
			m_offset = 0U;
			return RTM_ERROR;
		}

		m_offset = 3U;
	}

	if (m_offset >= 3U) {
		while (m_offset < m_length) {
			int ret = m_serial.read(m_buffer + m_offset, m_length - m_offset);
			if (ret < 0) {
				LogError("Error when reading from the modem");
				m_offset = 0U;
				return RTM_ERROR;
			}

			if (ret == 0)
				return RTM_TIMEOUT;

			if (ret > 0)
				m_offset += ret;
		}
	}

	m_offset = 0U;

	switch (m_buffer[2U]) {
	case MMDVM_DEBUG1:
	case MMDVM_DEBUG2:
	case MMDVM_DEBUG3:
	case MMDVM_DEBUG4:
	case MMDVM_DEBUG5:
		printDebug();
		return RTM_TIMEOUT;

	default:
		// CUtils::dump(1U, "Received", m_buffer, m_length);
		return RTM_OK;
	}
}

HW_TYPE CModem::getHWType() const
{
	return m_hwType;
}

bool CModem::setMode(unsigned char mode)
{
	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_SET_MODE;
	buffer[3U] = mode;

	// CUtils::dump(1U, "Written", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CModem::sendCWId(const std::string& callsign)
{
	unsigned int length = callsign.length();
	if (length > 200U)
		length = 200U;

	unsigned char buffer[205U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = length + 3U;
	buffer[2U] = MMDVM_SEND_CWID;

	for (unsigned int i = 0U; i < length; i++)
		buffer[i + 3U] = callsign.at(i);

	// CUtils::dump(1U, "Written", buffer, length + 3U);

	return m_serial.write(buffer, length + 3U) == int(length + 3U);
}

bool CModem::writeDMRStart(bool tx)
{
	if (tx && m_tx)
		return true;
	if (!tx && !m_tx)
		return true;

	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_DMR_START;
	buffer[3U] = tx ? 0x01U : 0x00U;

	// CUtils::dump(1U, "Written", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CModem::writeDMRAbort(unsigned int slotNo)
{
	if (slotNo == 1U)
		m_txDMRData1.clear();
	else
		m_txDMRData2.clear();

	unsigned char buffer[4U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_DMR_ABORT;
	buffer[3U] = slotNo;

	// CUtils::dump(1U, "Written", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CModem::writeDMRShortLC(const unsigned char* lc)
{
	assert(lc != NULL);

	unsigned char buffer[12U];

	buffer[0U]  = MMDVM_FRAME_START;
	buffer[1U]  = 12U;
	buffer[2U]  = MMDVM_DMR_SHORTLC;
	buffer[3U]  = lc[0U];
	buffer[4U]  = lc[1U];
	buffer[5U]  = lc[2U];
	buffer[6U]  = lc[3U];
	buffer[7U]  = lc[4U];
	buffer[8U]  = lc[5U];
	buffer[9U]  = lc[6U];
	buffer[10U] = lc[7U];
	buffer[11U] = lc[8U];

	// CUtils::dump(1U, "Written", buffer, 12U);

	return m_serial.write(buffer, 12U) == 12;
}

void CModem::printDebug()
{
	if (m_buffer[2U] == 0xF1U) {
		LogMessage("Debug: %.*s", m_length - 3U, m_buffer + 3U);
	} else if (m_buffer[2U] == 0xF2U) {
		short val1 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d", m_length - 5U, m_buffer + 3U, val1);
	} else if (m_buffer[2U] == 0xF3U) {
		short val1 = (m_buffer[m_length - 4U] << 8) | m_buffer[m_length - 3U];
		short val2 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d %d", m_length - 7U, m_buffer + 3U, val1, val2);
	} else if (m_buffer[2U] == 0xF4U) {
		short val1 = (m_buffer[m_length - 6U] << 8) | m_buffer[m_length - 5U];
		short val2 = (m_buffer[m_length - 4U] << 8) | m_buffer[m_length - 3U];
		short val3 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d %d %d", m_length - 9U, m_buffer + 3U, val1, val2, val3);
	} else if (m_buffer[2U] == 0xF5U) {
		short val1 = (m_buffer[m_length - 8U] << 8) | m_buffer[m_length - 7U];
		short val2 = (m_buffer[m_length - 6U] << 8) | m_buffer[m_length - 5U];
		short val3 = (m_buffer[m_length - 4U] << 8) | m_buffer[m_length - 3U];
		short val4 = (m_buffer[m_length - 2U] << 8) | m_buffer[m_length - 1U];
		LogMessage("Debug: %.*s %d %d %d %d", m_length - 11U, m_buffer + 3U, val1, val2, val3, val4);
	}
}
