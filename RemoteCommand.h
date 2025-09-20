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

#ifndef RemoteCommand_H_
#define RemoteCommand_H_

#include <string>


class CRemoteCommand {
public:
	CRemoteCommand(unsigned int port);
	CRemoteCommand(const std::string &host, unsigned int port);
	~CRemoteCommand();

	int send(const std::string &command);

private:
	std::string m_host;
	unsigned int m_port;
};


#endif	/* !RemoteCommand_H_ */
