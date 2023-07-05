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

#include "MQTTConnection.h"

#include <cassert>
#include <cstdio>
#include <cstring>


CMQTTConnection::CMQTTConnection(const std::string& host, unsigned short port, const std::string& name, const std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>>& subs, unsigned int keepalive, MQTT_QOS qos) :
m_host(host),
m_port(port),
m_name(name),
m_subs(subs),
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

CMQTTConnection::~CMQTTConnection()
{
	::mosquitto_lib_cleanup();
}

bool CMQTTConnection::open()
{
	m_mosq = ::mosquitto_new(m_name.c_str(), true, this);
	if (m_mosq == NULL){
		::fprintf(stderr, "MQTT Error newing: Out of memory.\n");
		return false;
	}

	::mosquitto_connect_callback_set(m_mosq, onConnect);
	::mosquitto_subscribe_callback_set(m_mosq, onSubscribe);
	::mosquitto_message_callback_set(m_mosq, onMessage);
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

bool CMQTTConnection::publish(const char* topic, const char* text)
{
	assert(topic != NULL);
	assert(text != NULL);

	return publish(topic, (unsigned char*)text, ::strlen(text));
}

bool CMQTTConnection::publish(const char* topic, const std::string& text)
{
	assert(topic != NULL);

	return publish(topic, (unsigned char*)text.c_str(), text.size());
}

bool CMQTTConnection::publish(const char* topic, const unsigned char* data, unsigned int len)
{
	assert(topic != NULL);
	assert(data != NULL);

	if (!m_connected)
		return false;

	if (::strchr(topic, '/') == NULL) {
		char topicEx[100U];
		::sprintf(topicEx, "%s/%s", m_name.c_str(), topic);

		int rc = ::mosquitto_publish(m_mosq, NULL, topicEx, len, data, static_cast<int>(m_qos), false);
		if (rc != MOSQ_ERR_SUCCESS) {
			::fprintf(stderr, "MQTT Error publishing: %s\n", ::mosquitto_strerror(rc));
			return false;
		}
	} else {
		int rc = ::mosquitto_publish(m_mosq, NULL, topic, len, data, static_cast<int>(m_qos), false);
		if (rc != MOSQ_ERR_SUCCESS) {
			::fprintf(stderr, "MQTT Error publishing: %s\n", ::mosquitto_strerror(rc));
			return false;
		}
	}

	return true;
}

void CMQTTConnection::close()
{
	if (m_mosq != NULL) {
		::mosquitto_disconnect(m_mosq);
		::mosquitto_destroy(m_mosq);
		m_mosq = NULL;
	}
}

void CMQTTConnection::onConnect(mosquitto* mosq, void* obj, int rc)
{
	assert(mosq != NULL);
	assert(obj != NULL);

	::fprintf(stdout, "MQTT: on_connect: %s\n", ::mosquitto_connack_string(rc));
	if (rc != 0) {
		::mosquitto_disconnect(mosq);
		return;
	}

	CMQTTConnection* p = static_cast<CMQTTConnection*>(obj);
	p->m_connected = true;

	for (std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>>::const_iterator it = p->m_subs.cbegin(); it != p->m_subs.cend(); ++it) {
		std::string topic = (*it).first;

		if (topic.find_first_of('/') == std::string::npos) {
			char topicEx[100U];
			::sprintf(topicEx, "%s/%s", p->m_name.c_str(), topic.c_str());

			rc = ::mosquitto_subscribe(mosq, NULL, topicEx, static_cast<int>(p->m_qos));
			if (rc != MOSQ_ERR_SUCCESS) {
				::fprintf(stderr, "MQTT: error subscribing to %s - %s\n", topicEx, ::mosquitto_strerror(rc));
				::mosquitto_disconnect(mosq);
			}
		} else {
			rc = ::mosquitto_subscribe(mosq, NULL, topic.c_str(), static_cast<int>(p->m_qos));
			if (rc != MOSQ_ERR_SUCCESS) {
				::fprintf(stderr, "MQTT: error subscribing to %s - %s\n", topic.c_str(), ::mosquitto_strerror(rc));
				::mosquitto_disconnect(mosq);
			}
		}
	}
}

void CMQTTConnection::onSubscribe(mosquitto* mosq, void* obj, int mid, int qosCount, const int* grantedQOS)
{
	assert(mosq != NULL);
	assert(obj != NULL);
	assert(grantedQOS != NULL);

	for (int i = 0; i < qosCount; i++)
		::fprintf(stdout, "MQTT: on_subscribe: %d:%d\n", i, grantedQOS[i]);
}

void CMQTTConnection::onMessage(mosquitto* mosq, void* obj, const mosquitto_message* message)
{
	assert(mosq != NULL);
	assert(obj != NULL);
	assert(message != NULL);

	CMQTTConnection* p = static_cast<CMQTTConnection*>(obj);

	for (std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>>::const_iterator it = p->m_subs.cbegin(); it != p->m_subs.cend(); ++it) {
		std::string topic = (*it).first;

		char topicEx[100U];
		::sprintf(topicEx, "%s/%s", p->m_name.c_str(), topic.c_str());

		if (::strcmp(topicEx, message->topic) == 0) {
			(*it).second((unsigned char*)message->payload, message->payloadlen);
			break;
		}
	}
}

void CMQTTConnection::onDisconnect(mosquitto* mosq, void* obj, int rc)
{
	assert(mosq != NULL);
	assert(obj != NULL);

	::fprintf(stdout, "MQTT: on_disconnect: %s\n", ::mosquitto_reason_string(rc));

	CMQTTConnection* p = static_cast<CMQTTConnection*>(obj);
	p->m_connected = false;
}

