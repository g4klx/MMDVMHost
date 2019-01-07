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

#ifndef	RemoteControl_H
#define	RemoteControl_H

#include "UDPSocket.h"

enum REMOTE_COMMAND {
	RC_NONE,
	RC_MODE_IDLE,
	RC_MODE_LOCKOUT,
	RC_MODE_DSTAR,
	RC_MODE_DMR,
	RC_MODE_YSF,
	RC_MODE_P25,
	RC_MODE_NXDN
};

class CRemoteControl {
public:
	CRemoteControl(unsigned int port);
	~CRemoteControl();

	bool open();

	REMOTE_COMMAND getCommand();

	void close();

private:
	CUDPSocket m_socket;
};

#endif
