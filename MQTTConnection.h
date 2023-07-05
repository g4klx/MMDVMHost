/*
 *   Copyright (C) 2022,2023 by Jonathan Naylor G4KLX
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

#include <vector>
#include <string>

enum MQTT_QOS {
	MQTT_QOS_AT_MODE_ONCE  = 0U,
	MQTT_QOS_AT_LEAST_ONCE = 1U,
	MQTT_QOS_EXACTLY_ONCE  = 2U
};

class CMQTTConnection {
public:
	CMQTTConnection(const std::string& host, unsigned short port, const std::string& name, const std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>>& subs, unsigned int keepalive, MQTT_QOS qos = MQTT_QOS_EXACTLY_ONCE);
	~CMQTTConnection();

	bool open();

	bool publish(const char* topic, const char* text);
	bool publish(const char* topic, const std::string& text);
	bool publish(const char* topic, const unsigned char* data, unsigned int len);

	void close();

private:
	std::string    m_host;
	unsigned short m_port;
	std::string    m_name;
	std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>> m_subs;
	unsigned int   m_keepalive;
	MQTT_QOS       m_qos;
	mosquitto*     m_mosq;
	bool           m_connected;

	static void onConnect(mosquitto* mosq, void* obj, int rc);
	static void onSubscribe(mosquitto* mosq, void* obj, int mid, int qosCount, const int* grantedQOS);
	static void onMessage(mosquitto* mosq, void* obj, const mosquitto_message* message);
	static void onDisconnect(mosquitto* mosq, void* obj, int rc);
};

#endif

