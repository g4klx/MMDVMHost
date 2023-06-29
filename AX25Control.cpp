/*
 *	Copyright (C) 2020,2023 Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "AX25Control.h"
#include "AX25Defines.h"
#include "Utils.h"
#include "Log.h"

#if defined(USE_AX25)

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

#include <nlohmann/json.hpp>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CAX25Control::CAX25Control(CAX25Network* network, bool trace) :
m_network(network),
m_trace(trace),
m_enabled(true),
m_fp(NULL)
{
}

CAX25Control::~CAX25Control()
{
}

bool CAX25Control::writeModem(unsigned char *data, unsigned int len)
{
	assert(data != NULL);

	if (!m_enabled)
		return false;

	if (m_trace)
		decode(data, len);

	decodeJSON("rf", data, len);

	CUtils::dump(1U, "AX.25 received packet", data, len);

	if (m_network == NULL)
		return true;

	return m_network->write(data, len);
}

unsigned int CAX25Control::readModem(unsigned char* data)
{
	assert(data != NULL);

	if (m_network == NULL)
		return 0U;

	if (!m_enabled)
		return 0U;

	unsigned int length = m_network->read(data, 500U);

	decodeJSON("network", data, length);

	if (length > 0U)
		CUtils::dump(1U, "AX.25 transmitted packet", data, length);

	return length;
}

void CAX25Control::enable(bool enabled)
{
	m_enabled = enabled;
}

void CAX25Control::decode(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length >= 15U);

	std::string text;

	bool more = decodeAddress(data + 7U, text);

	text += '>';

	decodeAddress(data + 0U, text);

	unsigned int n = 14U;
	while (more && n < length) {
		text += ',';
		more = decodeAddress(data + n, text, true);
		n += 7U;
	}

	text += ' ';

	if ((data[n] & 0x01U) == 0x00U) {
		// I frame
		char t[20U];
		::sprintf(t, "<I S%u R%u>", (data[n] >> 1) & 0x07U, (data[n] >> 5) & 0x07U);
		text += t;
	} else {
		if ((data[n] & 0x02U) == 0x00U) {
			// S frame
			char t[20U];
			switch (data[n] & 0x0FU) {
				case 0x01U:
					::sprintf(t, "<RR R%u>", (data[n] >> 5) & 0x07U);
					break;
				case 0x05U:
					::sprintf(t, "<RNR R%u>", (data[n] >> 5) & 0x07U);
					break;
				case 0x09U:
					::sprintf(t, "<REJ R%u>", (data[n] >> 5) & 0x07U);
					break;
				case 0x0DU:
					::sprintf(t, "<SREJ R%u>", (data[n] >> 5) & 0x07U);
					break;
				default:
					::sprintf(t, "<Unknown R%u>", (data[n] >> 5) & 0x07U);
					break;
			}

			text += t;
			LogMessage("AX.25, %s", text.c_str());
			return;
		} else {
			// U frame
			switch (data[n] & 0xEFU) {
				case 0x6FU:
					text += "<SABME>";
					break;
				case 0x2FU:
					text += "<SABM>";
					break;
				case 0x43U:
					text += "<DISC>";
					break;
				case 0x0FU:
					text += "<DM>";
					break;
				case 0x63U:
					text += "<UA>";
					break;
				case 0x87U:
					text += "<FRMR>";
					break;
				case 0x03U:
					text += "<UI>";
					break;
				case 0xAFU:
					text += "<XID>";
					break;
				case 0xE3U:
					text += "<TEST>";
					break;
				default:
					text += "<Unknown>";
					break;
			}

			if ((data[n] & 0xEFU) != 0x03U) {
				LogMessage("AX.25, %s", text.c_str());
				return;
			}
		}
	}

	n += 2U;

	LogMessage("AX.25, %s %.*s", text.c_str(), length - n, data + n);
}

void CAX25Control::decodeJSON(const char* source, const unsigned char* data, unsigned int length)
{
	assert(source != NULL);
	assert(data != NULL);
	assert(length >= 15U);

	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["source"]    = source;

	std::string text;

	bool isDigi;
	bool more = decodeAddressJSON(data + 7U, text, isDigi);
	json["source_cs"] = text;

	decodeAddressJSON(data + 0U, text, isDigi);
	json["destination_cs"] = text;

	unsigned int n = 14U;

	if (more) {
		while (more && n < length) {
			nlohmann::json digi;

			more = decodeAddressJSON(data + n, text, isDigi);
			n += 7U;

			digi["callsign"] = text;
			digi["repeated"] = isDigi;

			json["digipeaters"] = digi;
		}
	}

	if ((data[n] & 0x01U) == 0x00U) {
		// I frame
		char t[20U];
		::sprintf(t, "I S%u R%u", (data[n] >> 1) & 0x07U, (data[n] >> 5) & 0x07U);
		json["type"] = t;
	} else {
		if ((data[n] & 0x02U) == 0x00U) {
			// S frame
			char t[20U];
			switch (data[n] & 0x0FU) {
				case 0x01U:
					::sprintf(t, "RR R%u", (data[n] >> 5) & 0x07U);
					break;
				case 0x05U:
					::sprintf(t, "RNR R%u", (data[n] >> 5) & 0x07U);
					break;
				case 0x09U:
					::sprintf(t, "REJ R%u", (data[n] >> 5) & 0x07U);
					break;
				case 0x0DU:
					::sprintf(t, "SREJ R%u", (data[n] >> 5) & 0x07U);
					break;
				default:
					::sprintf(t, "Unknown R%u", (data[n] >> 5) & 0x07U);
					break;
			}

			json["type"] = t;
			WriteJSON("AX.25", json);
			return;
		} else {
			// U frame
			switch (data[n] & 0xEFU) {
				case 0x6FU:
					text = "SABME";
					break;
				case 0x2FU:
					text = "SABM";
					break;
				case 0x43U:
					text = "DISC";
					break;
				case 0x0FU:
					text = "DM";
					break;
				case 0x63U:
					text = "UA";
					break;
				case 0x87U:
					text = "FRMR";
					break;
				case 0x03U:
					text = "UI";
					break;
				case 0xAFU:
					text = "XID";
					break;
				case 0xE3U:
					text = "TEST";
					break;
				default:
					text = "Unknown>";
					break;
			}

			json["type"] = text;

			if ((data[n] & 0xEFU) != 0x03U) {
				WriteJSON("AX.25", json);
				return;
			}
		}
	}

	n++;

	char buffer[5U];
	::sprintf(buffer, "%02X", data[n]);
	json["pid"] = buffer;

	n++;

	text.clear();

	for (unsigned int i = 0U; i < (length - n); i++) {
		char buffer[5U];
		::sprintf(buffer, "%02X ", data[n + i]);
		text += buffer;
	}

	for (unsigned int i = 0U; i < (length - n); i++) {
		char c = data[n + i];

		if (::isprint(c) || c == ' ')
			text += c;
		else
			text += '.';
	}

	json["data"] = text;

	WriteJSON("AX.25", json);
}

bool CAX25Control::decodeAddress(const unsigned char* data, std::string& text, bool isDigi) const
{
	assert(data != NULL);

	for (unsigned int i = 0U; i < 6U; i++) {
		char c = data[i] >> 1;
		if (c != ' ')
			text += c;
	}

	unsigned char ssid = (data[6U] >> 1) & 0x0FU;
	if (ssid > 0U) {
		text += '-';
		if (ssid >= 10U) {
			text += '1';
			text += '0' + ssid - 10U;
		} else {
			text += '0' + ssid;
		}
	}

	if (isDigi) {
		if ((data[6U] & 0x80U) == 0x80U)
			text += '*';
	}

	return (data[6U] & 0x01U) == 0x00U;
}

bool CAX25Control::decodeAddressJSON(const unsigned char* data, std::string& text, bool& isDigi) const
{
	assert(data != NULL);

	text.clear();

	for (unsigned int i = 0U; i < 6U; i++) {
		char c = data[i] >> 1;
		if (c != ' ')
			text += c;
	}

	unsigned char ssid = (data[6U] >> 1) & 0x0FU;
	if (ssid > 0U) {
		text += '-';
		if (ssid >= 10U) {
			text += '1';
			text += '0' + ssid - 10U;
		} else {
			text += '0' + ssid;
		}
	}

	isDigi = (data[6U] & 0x80U) == 0x80U;

	return (data[6U] & 0x01U) == 0x00U;
}

#endif

