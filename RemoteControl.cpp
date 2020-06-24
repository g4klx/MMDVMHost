/*
 *   Copyright (C) 2019 by Jonathan Naylor G4KLX
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
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>

const unsigned int SET_MODE_ARGS = 2U;
const unsigned int ENABLE_ARGS = 2U;
const unsigned int DISABLE_ARGS = 2U;
const unsigned int PAGE_ARGS = 3U;
const unsigned int CW_ARGS = 2U;

const unsigned int BUFFER_LENGTH = 100U;

CRemoteControl::CRemoteControl(unsigned int port) :
m_socket(port),
m_command(RCD_NONE),
m_args()
{
	assert(port > 0U);
}

CRemoteControl::~CRemoteControl()
{
}

bool CRemoteControl::open()
{
	return m_socket.open();
}

REMOTE_COMMAND CRemoteControl::getCommand()
{
	m_command = RCD_NONE;
	m_args.clear();

	char command[BUFFER_LENGTH];
	char buffer[BUFFER_LENGTH];
	in_addr address;
	unsigned int port;
	int ret = m_socket.read((unsigned char*)buffer, BUFFER_LENGTH, address, port);
	if (ret > 0) {
		buffer[ret] = '\0';

		// Make a copy of the original command for logging.
		::strcpy(command, buffer);

		// Parse the original command into a vector of strings.
		char* b = buffer;
		char* p = NULL;
		while ((p = ::strtok(b, " ")) != NULL) {
			b = NULL;
			m_args.push_back(std::string(p));
		}

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
			else if (m_args.at(1U) == "fm")
				m_command = RCD_ENABLE_FM;
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
			else if (m_args.at(1U) == "fm")
				m_command = RCD_DISABLE_FM;
		} else if (m_args.at(0U) == "page" && m_args.size() >= PAGE_ARGS) {
			// Page command is in the form of "page <ric> <message>"
			m_command = RCD_PAGE;
		} else if (m_args.at(0U) == "cw" && m_args.size() >= CW_ARGS) {
                        // CW command is in the form of "cw <message>"
                        m_command = RCD_CW;
                }

		if (m_command == RCD_NONE) {
			m_args.clear();
			LogWarning("Invalid remote command of \"%s\" received", command);
		} else {
			LogMessage("Valid remote command of \"%s\" received", command);
		}
	}
	
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
			return m_args.size() - SET_MODE_ARGS;
		case RCD_PAGE:
			return m_args.size() - 1U;
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
			n += SET_MODE_ARGS;
			break;
		case RCD_PAGE:
			n += 1U;
			break;
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

void CRemoteControl::close()
{
	m_socket.close();
}
