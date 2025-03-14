/*
 *   Copyright (C) 2019,2020,2021,2024,2025 by Jonathan Naylor G4KLX
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
#include <cstring>

const unsigned int SET_MODE_ARGS = 2U;
const unsigned int ENABLE_ARGS = 2U;
const unsigned int DISABLE_ARGS = 2U;
const unsigned int PAGE_ARGS = 3U;
const unsigned int CW_ARGS = 2U;

const unsigned int BUFFER_LENGTH = 100U;

CRemoteControl::CRemoteControl(CMMDVMHost *host, const std::string address, unsigned int port) :
m_host(host),
m_socket(address, port),
m_addr(),
m_addrLen(0U),
m_command(REMOTE_COMMAND::NONE),
m_args()
{
	assert(port > 0U);

	if (CUDPSocket::lookup(address, port, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;
}

CRemoteControl::~CRemoteControl()
{
}

bool CRemoteControl::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the remote control port");
		return false;
	}

	return m_socket.open(m_addr);
}

REMOTE_COMMAND CRemoteControl::getCommand()
{
	m_command = REMOTE_COMMAND::NONE;
	m_args.clear();

	char command[BUFFER_LENGTH];
	char buffer[BUFFER_LENGTH * 2];
	std::string replyStr = "OK";
	sockaddr_storage address;
	unsigned int addrlen;
	int ret = m_socket.read((unsigned char*)buffer, BUFFER_LENGTH, address, addrlen);
	if (ret > 0) {
		buffer[ret] = '\0';

		// Make a copy of the original command for logging.
		::strcpy(command, buffer);

		// Parse the original command into a vector of strings.
		char* b = buffer;
		char* p = nullptr;
		while ((p = ::strtok(b, " ")) != nullptr) {
			b = nullptr;
			m_args.push_back(std::string(p));
		}

		if (m_args.at(0U) == "mode" && m_args.size() >= SET_MODE_ARGS) {
			// Mode command is in the form of "mode <mode> [<timeout>|fixed]"
			if (m_args.at(1U) == "idle")
				m_command = REMOTE_COMMAND::MODE_IDLE;
			else if (m_args.at(1U) == "lockout")
				m_command = REMOTE_COMMAND::MODE_LOCKOUT;
			else if (m_args.at(1U) == "d-star")
				m_command = REMOTE_COMMAND::MODE_DSTAR;
			else if (m_args.at(1U) == "dmr")
				m_command = REMOTE_COMMAND::MODE_DMR;
			else if (m_args.at(1U) == "ysf")
				m_command = REMOTE_COMMAND::MODE_YSF;
			else if (m_args.at(1U) == "p25")
				m_command = REMOTE_COMMAND::MODE_P25;
			else if (m_args.at(1U) == "nxdn")
				m_command = REMOTE_COMMAND::MODE_NXDN;
			else if (m_args.at(1U) == "m17")
				m_command = REMOTE_COMMAND::MODE_M17;
			else
				replyStr = "KO";
		} else if (m_args.at(0U) == "enable" && m_args.size() >= ENABLE_ARGS) {
			if (m_args.at(1U) == "dstar")
				m_command = REMOTE_COMMAND::ENABLE_DSTAR;
			else if (m_args.at(1U) == "dmr")
				m_command = REMOTE_COMMAND::ENABLE_DMR;
			else if (m_args.at(1U) == "ysf")
				m_command = REMOTE_COMMAND::ENABLE_YSF;
			else if (m_args.at(1U) == "p25")
				m_command = REMOTE_COMMAND::ENABLE_P25;
			else if (m_args.at(1U) == "nxdn")
				m_command = REMOTE_COMMAND::ENABLE_NXDN;
			else if (m_args.at(1U) == "m17")
				m_command = REMOTE_COMMAND::ENABLE_M17;
			else if (m_args.at(1U) == "fm")
				m_command = REMOTE_COMMAND::ENABLE_FM;
			else if (m_args.at(1U) == "ax25")
				m_command = REMOTE_COMMAND::ENABLE_AX25;
			else
				replyStr = "KO";
		} else if (m_args.at(0U) == "disable" && m_args.size() >= DISABLE_ARGS) {
			if (m_args.at(1U) == "dstar")
				m_command = REMOTE_COMMAND::DISABLE_DSTAR;
			else if (m_args.at(1U) == "dmr")
				m_command = REMOTE_COMMAND::DISABLE_DMR;
			else if (m_args.at(1U) == "ysf")
				m_command = REMOTE_COMMAND::DISABLE_YSF;
			else if (m_args.at(1U) == "p25")
				m_command = REMOTE_COMMAND::DISABLE_P25;
			else if (m_args.at(1U) == "nxdn")
				m_command = REMOTE_COMMAND::DISABLE_NXDN;
			else if (m_args.at(1U) == "m17")
				m_command = REMOTE_COMMAND::DISABLE_M17;
			else if (m_args.at(1U) == "fm")
				m_command = REMOTE_COMMAND::DISABLE_FM;
			else if (m_args.at(1U) == "ax25")
				m_command = REMOTE_COMMAND::DISABLE_AX25;
			else
				replyStr = "KO";
		} else if (m_args.at(0U) == "page" && m_args.size() >= PAGE_ARGS) {
			// Page command is in the form of "page <ric> <message>"
			m_command = REMOTE_COMMAND::PAGE;
		} else if (m_args.at(0U) == "page_bcd" && m_args.size() >= PAGE_ARGS) {
			// BCD page command is in the form of "page_bcd <ric> <bcd message>"
			m_command = REMOTE_COMMAND::PAGE_BCD;
		} else if (m_args.at(0U) == "page_a1" && m_args.size() == 2) {
			// Alert1 page command is in the form of "page_a1 <ric>"
			m_command = REMOTE_COMMAND::PAGE_A1;
		} else if (m_args.at(0U) == "page_a2" && m_args.size() >= PAGE_ARGS) {
			// Alert2 page command is in the form of "page_a2 <ric> <message>"
			m_command = REMOTE_COMMAND::PAGE_A2;
		} else if (m_args.at(0U) == "cw" && m_args.size() >= CW_ARGS) {
                        // CW command is in the form of "cw <message>"
                        m_command = REMOTE_COMMAND::CW;
		} else if (m_args.at(0U) == "reload") {
                        // Reload command is in the form of "reload"
                        m_command = REMOTE_COMMAND::RELOAD;
		} else if (m_args.at(0U) == "status") {
			if (m_host != nullptr) {
				m_host->buildNetworkStatusString(replyStr);
			} else {
				replyStr = "KO";
			}

			m_command = REMOTE_COMMAND::CONNECTION_STATUS;
		} else if (m_args.at(0U) == "hosts") {
			if (m_host != nullptr) {
				m_host->buildNetworkHostsString(replyStr);
			} else {
				replyStr = "KO";
			}

			m_command = REMOTE_COMMAND::CONFIG_HOSTS;
		} else {
			replyStr = "KO";
		}

		::snprintf(buffer, BUFFER_LENGTH * 2, "%s remote command of \"%s\" received", ((m_command == REMOTE_COMMAND::NONE) ? "Invalid" : "Valid"), command);
		if (m_command == REMOTE_COMMAND::NONE) {
			m_args.clear();
			LogWarning(buffer);
		} else {
#if !defined(REMOTE_COMMAND_NO_LOG)
			LogMessage(buffer);
#endif
		}
		
		m_socket.write((unsigned char*)replyStr.c_str(), (unsigned int)replyStr.length(), address, addrlen);
	}
	
	return m_command;
}

unsigned int CRemoteControl::getArgCount() const
{
	switch (m_command) {
		case REMOTE_COMMAND::MODE_IDLE:
		case REMOTE_COMMAND::MODE_LOCKOUT:
		case REMOTE_COMMAND::MODE_DSTAR:
		case REMOTE_COMMAND::MODE_DMR:
		case REMOTE_COMMAND::MODE_YSF:
		case REMOTE_COMMAND::MODE_P25:
		case REMOTE_COMMAND::MODE_NXDN:
		case REMOTE_COMMAND::MODE_M17:
			return (unsigned int)m_args.size() - SET_MODE_ARGS;
		case REMOTE_COMMAND::PAGE:
		case REMOTE_COMMAND::PAGE_BCD:
		case REMOTE_COMMAND::PAGE_A1:
		case REMOTE_COMMAND::PAGE_A2:
			return (unsigned int)m_args.size() - 1U;
		case REMOTE_COMMAND::CW:
			return (unsigned int)m_args.size() - 1U;
		default:
			return 0U;
	}
}

std::string CRemoteControl::getArgString(unsigned int n) const
{
	switch (m_command) {
		case REMOTE_COMMAND::MODE_IDLE:
		case REMOTE_COMMAND::MODE_LOCKOUT:
		case REMOTE_COMMAND::MODE_DSTAR:
		case REMOTE_COMMAND::MODE_DMR:
		case REMOTE_COMMAND::MODE_YSF:
		case REMOTE_COMMAND::MODE_P25:
		case REMOTE_COMMAND::MODE_NXDN:
		case REMOTE_COMMAND::MODE_M17:
			n += SET_MODE_ARGS;
			break;
		case REMOTE_COMMAND::PAGE:
		case REMOTE_COMMAND::PAGE_BCD:
		case REMOTE_COMMAND::PAGE_A1:
		case REMOTE_COMMAND::PAGE_A2:
			n += 1U;
			break;
		case REMOTE_COMMAND::CW:
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
