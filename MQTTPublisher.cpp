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

#include "MQTTPublisher.h"

#include <cassert>
#include <cstdio>
#include <cstring>


CMQTTPublisher::CMQTTPublisher(const std::string& host, unsigned short port, const std::string& name, unsigned int keepalive, MQTT_QOS qos) :
m_host(host),
m_port(port),
m_name(name),
m_keepalive(keepalive),
m_qos(qos),
m_mosq(NULL),
m_connected(false)
{
	assert(!host.empty());
	assert(port > 0U);
	assert(!name.empty());
	assert(keepalive >= 5U);

	::mosquitto_lib_init();
}

CMQTTPublisher::~CMQTTPublisher()
{
	::mosquitto_lib_cleanup();
}

bool CMQTTPublisher::open()
{
	m_mosq = ::mosquitto_new(m_name.c_str(), true, this);
	if (m_mosq == NULL){
		::fprintf(stderr, "MQTT Error newing: Out of memory.\n");
		return false;
	}

	::mosquitto_connect_callback_set(m_mosq, onConnect);
	::mosquitto_disconnect_callback_set(m_mosq, onDisconnect);

	int rc = ::mosquitto_connect(m_mosq, m_host.c_str(), m_port, m_keepalive);
	if (rc != MOSQ_ERR_SUCCESS) {
		::mosquitto_destroy(m_mosq);
		m_mosq = NULL;
		::fprintf(stderr, "MQTT Error connecting: %s\n", ::mosquitto_strerror(rc));
		return false;
	}

	rc = ::mosquitto_loop_start(m_mosq);
	if (rc != MOSQ_ERR_SUCCESS) {
		::mosquitto_disconnect(m_mosq);
		::mosquitto_destroy(m_mosq);
		m_mosq = NULL;
		::fprintf(stderr, "MQTT Error loop starting: %s\n", ::mosquitto_strerror(rc));
		return false;
	}

	return true;
}

bool CMQTTPublisher::publish(const char* topic, const char* text)
{
	assert(topic != NULL);
	assert(text != NULL);

	if (!m_connected)
		return false;

	char topicEx[100U];
	::sprintf(topicEx, "%s/%s", m_name.c_str(), topic);

	int rc = ::mosquitto_publish(m_mosq, NULL, topicEx, ::strlen(text), text, static_cast<int>(m_qos), false);
	if (rc != MOSQ_ERR_SUCCESS) {
		::fprintf(stderr, "MQTT Error publishing: %s\n", ::mosquitto_strerror(rc));
		return false;
	}

	return true;
}

void CMQTTPublisher::close()
{
	if (m_mosq != NULL) {
		::mosquitto_disconnect(m_mosq);
		::mosquitto_destroy(m_mosq);
		m_mosq = NULL;
	}
}

void CMQTTPublisher::onConnect(mosquitto* mosq, void* obj, int rc)
{
	assert(mosq != NULL);
	assert(obj != NULL);

	::fprintf(stdout, "MQTT: on_connect: %s\n", ::mosquitto_connack_string(rc));

	CMQTTPublisher* p = static_cast<CMQTTPublisher*>(obj);
	p->m_connected = true;
}

void CMQTTPublisher::onDisconnect(mosquitto* mosq, void* obj, int rc)
{
	assert(mosq != NULL);
	assert(obj != NULL);

	::fprintf(stdout, "MQTT: on_disconnect: %s\n", ::mosquitto_reason_string(rc));

	CMQTTPublisher* p = static_cast<CMQTTPublisher*>(obj);
	p->m_connected = false;
}

