/*
 *   Copyright (C) 2019,2020,2021,2023 by Jonathan Naylor G4KLX
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

#include "RemoteControl.h"
#include "MMDVMHost.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>

const unsigned int SET_MODE_ARGS = 2U;
const unsigned int ENABLE_ARGS = 2U;
const unsigned int DISABLE_ARGS = 2U;
const unsigned int PAGE_ARGS = 3U;
const unsigned int CW_ARGS = 2U;


CRemoteControl::CRemoteControl(CMMDVMHost *host, CMQTTConnection* mqtt) :
m_host(host),
m_mqtt(mqtt),
m_command(RCD_NONE),
m_args()
{
	assert(host != NULL);
	assert(mqtt != NULL);
}

CRemoteControl::~CRemoteControl()
{
}

REMOTE_COMMAND CRemoteControl::getCommand(const std::string& command)
{
	m_command = RCD_NONE;
	m_args.clear();

	std::string reply = "OK";

	std::stringstream tokeniser(command);

	// Parse the original command into a vector of strings.
	std::string token;
	while (std::getline(tokeniser, token, ' '))
		m_args.push_back(token);

	if (m_args.at(0U) == "mode" && m_args.size() >= SET_MODE_ARGS) {
		// Mode command is in the form of "mode <mode> [<timeout>|fixed]"
		if (m_args.at(1U) == "idle")
			m_command = RCD_MODE_IDLE;
		else if (m_args.at(1U) == "lockout")
			m_command = RCD_MODE_LOCKOUT;
		else if (m_args.at(1U) == "d-star")
			m_command = RCD_MODE_DSTAR;
		else if (m_args.at(1U) == "dmr")
			m_command = RCD_MODE_DMR;
		else if (m_args.at(1U) == "ysf")
			m_command = RCD_MODE_YSF;
		else if (m_args.at(1U) == "p25")
			m_command = RCD_MODE_P25;
		else if (m_args.at(1U) == "nxdn")
			m_command = RCD_MODE_NXDN;
		else if (m_args.at(1U) == "m17")
			m_command = RCD_MODE_M17;
		else
			reply = "KO";
	} else if (m_args.at(0U) == "enable" && m_args.size() >= ENABLE_ARGS) {
		if (m_args.at(1U) == "dstar")
			m_command = RCD_ENABLE_DSTAR;
		else if (m_args.at(1U) == "dmr")
			m_command = RCD_ENABLE_DMR;
		else if (m_args.at(1U) == "ysf")
			m_command = RCD_ENABLE_YSF;
		else if (m_args.at(1U) == "p25")
			m_command = RCD_ENABLE_P25;
		else if (m_args.at(1U) == "nxdn")
			m_command = RCD_ENABLE_NXDN;
		else if (m_args.at(1U) == "m17")
			m_command = RCD_ENABLE_M17;
#if defined(USE_FM)
		else if (m_args.at(1U) == "fm")
			m_command = RCD_ENABLE_FM;
#endif
#if defined(USE_AX25)
		else if (m_args.at(1U) == "ax25")
			m_command = RCD_ENABLE_AX25;
#endif
		else
			reply = "KO";
	} else if (m_args.at(0U) == "disable" && m_args.size() >= DISABLE_ARGS) {
		if (m_args.at(1U) == "dstar")
			m_command = RCD_DISABLE_DSTAR;
		else if (m_args.at(1U) == "dmr")
			m_command = RCD_DISABLE_DMR;
		else if (m_args.at(1U) == "ysf")
			m_command = RCD_DISABLE_YSF;
		else if (m_args.at(1U) == "p25")
			m_command = RCD_DISABLE_P25;
		else if (m_args.at(1U) == "nxdn")
			m_command = RCD_DISABLE_NXDN;
		else if (m_args.at(1U) == "m17")
			m_command = RCD_DISABLE_M17;
#if defined(USE_FM)
		else if (m_args.at(1U) == "fm")
			m_command = RCD_DISABLE_FM;
#endif
#if defined(USE_AX25)
		else if (m_args.at(1U) == "ax25")
			m_command = RCD_DISABLE_AX25;
#endif
		else
			reply = "KO";
#if defined(USE_POCSAG)
	} else if (m_args.at(0U) == "page" && m_args.size() >= PAGE_ARGS) {
		// Page command is in the form of "page <ric> <message>"
		m_command = RCD_PAGE;
	} else if (m_args.at(0U) == "page_bcd" && m_args.size() >= PAGE_ARGS) {
		// BCD page command is in the form of "page_bcd <ric> <bcd message>"
		m_command = RCD_PAGE_BCD;
	} else if (m_args.at(0U) == "page_a1" && m_args.size() == 2) {
		// Alert1 page command is in the form of "page_a1 <ric>"
		m_command = RCD_PAGE_A1;
	} else if (m_args.at(0U) == "page_a2" && m_args.size() >= PAGE_ARGS) {
		// Alert2 page command is in the form of "page_a2 <ric> <message>"
		m_command = RCD_PAGE_A2;
#endif
	} else if (m_args.at(0U) == "cw" && m_args.size() >= CW_ARGS) {
		// CW command is in the form of "cw <message>"
		m_command = RCD_CW;
	} else if (m_args.at(0U) == "reload") {
		// Reload command is in the form of "reload"
		m_command = RCD_RELOAD;
	} else if (m_args.at(0U) == "status") {
		if (m_host != NULL) {
			m_host->buildNetworkStatusString(reply);
		} else {
			reply = "KO";
		}

		m_command = RCD_CONNECTION_STATUS;
	} else if (m_args.at(0U) == "hosts") {
		if (m_host != NULL) {
			m_host->buildNetworkHostsString(reply);
		} else {
			reply = "KO";
		}

		m_command = RCD_CONFIG_HOSTS;
	} else {
		reply = "KO";
	}

	char buffer[200U];
	::snprintf(buffer, 200, "%s remote command of \"%s\" received", ((m_command == RCD_NONE) ? "Invalid" : "Valid"), command.c_str());

	if (m_command == RCD_NONE) {
		m_args.clear();
		LogWarning(buffer);
	} else {
		LogMessage(buffer);
	}
		
	m_mqtt->publish("command", reply.c_str());
	
	return m_command;
}

unsigned int CRemoteControl::getArgCount() const
{
	switch (m_command) {
		case RCD_MODE_IDLE:
		case RCD_MODE_LOCKOUT:
		case RCD_MODE_DSTAR:
		case RCD_MODE_DMR:
		case RCD_MODE_YSF:
		case RCD_MODE_P25:
		case RCD_MODE_NXDN:
		case RCD_MODE_M17:
			return m_args.size() - SET_MODE_ARGS;
#if defined(USE_POCSAG)
		case RCD_PAGE:
		case RCD_PAGE_BCD:
		case RCD_PAGE_A1:
		case RCD_PAGE_A2:
			return m_args.size() - 1U;
#endif
		case RCD_CW:
                        return m_args.size() - 1U;
		default:
			return 0U;
	}
}

std::string CRemoteControl::getArgString(unsigned int n) const
{
	switch (m_command) {
		case RCD_MODE_IDLE:
		case RCD_MODE_LOCKOUT:
		case RCD_MODE_DSTAR:
		case RCD_MODE_DMR:
		case RCD_MODE_YSF:
		case RCD_MODE_P25:
		case RCD_MODE_NXDN:
		case RCD_MODE_M17:
			n += SET_MODE_ARGS;
			break;
#if defined(USE_POCSAG)
		case RCD_PAGE:
		case RCD_PAGE_BCD:
		case RCD_PAGE_A1:
		case RCD_PAGE_A2:
			n += 1U;
			break;
#endif
		case RCD_CW:
			n += 1U;
			break;
		default:
			return "";
	}

	if (n >= m_args.size())
		return "";

	return m_args.at(n);
}

unsigned int CRemoteControl::getArgUInt(unsigned int n) const
{
	return (unsigned int)::atoi(getArgString(n).c_str());
}

int CRemoteControl::getArgInt(unsigned int n) const
{
	return ::atoi(getArgString(n).c_str());
}

