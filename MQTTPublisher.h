/*
 *   Copyright (C) 2022 by Jonathan Naylor G4KLX
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

#if !defined(MQTTPUBLISHER_H)
#define	MQTTPUBLISHER_H

#include <mosquitto.h>

#include <string>


class CMQTTPublisher {
public:
	CMQTTPublisher(const std::string& host, unsigned short port, unsigned int keepalive, unsigned int qos);
	~CMQTTPublisher();

	bool open();

	bool publish(const char* topic, const char* text);

	void close();

private:
	std::string    m_host;
	unsigned short m_port;
	unsigned int   m_keepalive;
	unsigned int   m_qos;
	mosquitto*     m_mosq;
	bool           m_connected;

	static void onConnect(mosquitto* mosq, void* obj, int rc);
	static void onDisconnect(mosquitto* mosq, void* obj, int rc);
};

#endif

