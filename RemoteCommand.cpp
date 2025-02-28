/*
 *   Copyright (C) 2019,2020 by Jonathan Naylor G4KLX
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

#include "RemoteCommand.h"

#include "UDPSocket.h"
#include "Log.h"

#include <cstdio>
#include <chrono>
#include <thread>


const unsigned int BUFFER_LENGTH = 1024U;


int main(int argc, char *argv[]) {
	if (argc < 4) {
		::fprintf(stderr, "Usage: RemoteCommand <address> <port> <command> [argument]\n");
		return 1;
	}

	std::string host = std::string(argv[1]);
	unsigned int port = (unsigned int)::atoi(argv[2]);
	std::string cmd = std::string(argv[3]);

	for (int i = 4; i < argc; i++) {
		cmd += " ";
		cmd += std::string(argv[i]);
	}

	if (port == 0U) {
		::fprintf(stderr, "RemoteCommand: invalid port number - %s\n", argv[1]);
		return 1;
	}

	CRemoteCommand *command = new CRemoteCommand(host, port);

	return command->send(cmd);
}

CRemoteCommand::CRemoteCommand(unsigned int port): m_port(port) {
	CUDPSocket::startup();

	::LogInitialise(false, ".", "RemoteCommand", 2U, 2U, false);
}

CRemoteCommand::CRemoteCommand(const std::string &host, unsigned int port): m_host(host), m_port(port) {
	CUDPSocket::startup();

	::LogInitialise(false, ".", "RemoteCommand", 2U, 2U, false);
}

CRemoteCommand::~CRemoteCommand() {
	::LogFinalise();

	CUDPSocket::shutdown();
}

int CRemoteCommand::send(const std::string &command) {
	sockaddr_storage addr;
	unsigned int addrLen;
	char buffer[BUFFER_LENGTH];
	int retStatus = 0;

	if (CUDPSocket::lookup(m_host, m_port, addr, addrLen) != 0) {
		::fprintf(stderr, "Unable to resolve the address of the host\n");
		return 1;
	}

	CUDPSocket socket(0U);

	bool ret = socket.open(addr);
	if (!ret) {
		return 1;
	}

	ret = socket.write((unsigned char*)command.c_str(), command.length(), addr, addrLen);
	if (!ret) {
		socket.close();
		return 1;
	}

	::fprintf(stdout, "Command sent: \"%s\" to port: %u\n", command.c_str(), m_port);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	int len = socket.read((unsigned char*)buffer, BUFFER_LENGTH, addr, addrLen);
	if (len > 0) {
		buffer[len] = '\0';
		::fprintf(stdout, "%s\n", buffer);
	} else {
		retStatus = 1;
	}

	socket.close();

	return retStatus;
}
