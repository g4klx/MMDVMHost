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

#ifndef	RemoteControl_H
#define	RemoteControl_H

#include "Defines.h"

#include <vector>
#include <string>

#include "MQTTConnection.h"

class CMMDVMHost;

enum REMOTE_COMMAND {
	RCD_NONE,
	RCD_MODE_IDLE,
	RCD_MODE_LOCKOUT,
#if defined(USE_DSTAR)
	RCD_MODE_DSTAR,
#endif
	RCD_MODE_DMR,
	RCD_MODE_YSF,
	RCD_MODE_P25,
	RCD_MODE_NXDN,
#if defined(USE_M17)
	RCD_MODE_M17,
#endif
#if defined(USE_FM)
	RCD_MODE_FM,
#endif
#if defined(USE_DSTAR)
	RCD_ENABLE_DSTAR,
#endif
	RCD_ENABLE_DMR,
	RCD_ENABLE_YSF,
	RCD_ENABLE_P25,
	RCD_ENABLE_NXDN,
#if defined(USE_M17)
	RCD_ENABLE_M17,
#endif
#if defined(USE_FM)
	RCD_ENABLE_FM,
#endif
#if defined(USE_AX25)
	RCD_ENABLE_AX25,
#endif
#if defined(USE_DSTAR)
	RCD_DISABLE_DSTAR,
#endif
	RCD_DISABLE_DMR,
	RCD_DISABLE_YSF,
	RCD_DISABLE_P25,
	RCD_DISABLE_NXDN,
#if defined(USE_M17)
	RCD_DISABLE_M17,
#endif
#if defined(USE_FM)
	RCD_DISABLE_FM,
#endif
#if defined(USE_AX25)
	RCD_DISABLE_AX25,
#endif
#if defined(USE_POCSAG)
	RCD_PAGE,
	RCD_PAGE_BCD,
	RCD_PAGE_A1,
	RCD_PAGE_A2,
#endif
	RCD_CW,
	RCD_RELOAD,
	RCD_CONNECTION_STATUS,
	RCD_CONFIG_HOSTS
};

class CRemoteControl {
public:
	CRemoteControl(class CMMDVMHost *host, CMQTTConnection* mqtt);
	~CRemoteControl();

	REMOTE_COMMAND getCommand(const std::string& command);

	unsigned int getArgCount() const;

	std::string  getArgString(unsigned int n) const;
	unsigned int getArgUInt(unsigned int n) const;
	signed int   getArgInt(unsigned int n) const;

private:
	CMMDVMHost*              m_host;
	CMQTTConnection*         m_mqtt;
	REMOTE_COMMAND           m_command;
	std::vector<std::string> m_args;
};

#endif
