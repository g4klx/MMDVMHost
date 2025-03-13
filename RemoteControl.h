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

#ifndef	RemoteControl_H
#define	RemoteControl_H

#include "UDPSocket.h"

#include <vector>
#include <string>

class CMMDVMHost;

enum class REMOTE_COMMAND {
	NONE,
	MODE_IDLE,
	MODE_LOCKOUT,
	MODE_DSTAR,
	MODE_DMR,
	MODE_YSF,
	MODE_P25,
	MODE_NXDN,
	MODE_M17,
	MODE_FM,
	ENABLE_DSTAR,
	ENABLE_DMR,
	ENABLE_YSF,
	ENABLE_P25,
	ENABLE_NXDN,
	ENABLE_M17,
	ENABLE_FM,
	ENABLE_AX25,
	DISABLE_DSTAR,
	DISABLE_DMR,
	DISABLE_YSF,
	DISABLE_P25,
	DISABLE_NXDN,
	DISABLE_M17,
	DISABLE_FM,
	DISABLE_AX25,
	PAGE,
	PAGE_BCD,
	PAGE_A1,
	PAGE_A2,
	CW,
	RELOAD,
	CONNECTION_STATUS,
	CONFIG_HOSTS
};

class CRemoteControl {
public:
	CRemoteControl(class CMMDVMHost *host, const std::string address, unsigned int port);
	~CRemoteControl();

	bool open();

	REMOTE_COMMAND getCommand();

	unsigned int getArgCount() const;

	std::string  getArgString(unsigned int n) const;
	unsigned int getArgUInt(unsigned int n) const;
	signed int   getArgInt(unsigned int n) const;

	void close();

private:
	CMMDVMHost*              m_host;
	CUDPSocket               m_socket;
	sockaddr_storage         m_addr;
	unsigned int             m_addrLen;
	REMOTE_COMMAND           m_command;
	std::vector<std::string> m_args;
};

#endif
