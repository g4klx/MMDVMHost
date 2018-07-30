/*
 *   Copyright (C) 2016,2017,2018 by Tony Corbett G0WFV
 *   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

/*
* Some LCD displays include additional LEDs for status.
* If they exist, the LDCproc server will use the output command.
* If the LEDs do not exist, the command is ignored.
* to control these LEDs Below are the values for the Crystalfontz CFA-635.
* N4IRS

*    LED 1 (DMR)
*    Green 1		0000 0001
*    Red 16		0001 0000
*    Yellow 17		0001 0001

*    LED 2 (P25)
*    Green 2		0000 0010
*    Red 32		0010 0000
*    Yellow 34		0010 0010

*    LED 3 (Fusion)
*    Green 4		0000 0100
*    Red 64		0100 0000
*    Yellow 68		1000 0100

*    LED 4 (D-Star)
*    Green 8		0000 1000
*    Red 128		1000 0000
*    Yellow 136		1000 1000

*    LED 5 (NXDN)
*    Green 16		0001 0000
*    Red 255		1111 1111
*    Yellow 255		1111 1111

*/

#include "LCDproc.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <ctime>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#else
#include <winsock.h>
#endif

#define BUFFER_MAX_LEN 128

int            m_socketfd;
char           m_buffer[BUFFER_MAX_LEN];
fd_set         m_readfds, m_writefds;
struct timeval m_timeout;
int            m_recvsize;
unsigned int   m_rows(0);
unsigned int   m_cols(0);
bool           m_screensDefined(false);
bool           m_connected(false);

char           m_displayBuffer1[BUFFER_MAX_LEN];
char           m_displayBuffer2[BUFFER_MAX_LEN];

const unsigned int DSTAR_RSSI_COUNT = 3U;		// 3 * 420ms = 1260ms
const unsigned int DMR_RSSI_COUNT   = 4U;		// 4 * 360ms = 1440ms
const unsigned int YSF_RSSI_COUNT   = 13U;		// 13 * 100ms = 1300ms
const unsigned int P25_RSSI_COUNT   = 7U;		// 7 * 180ms = 1260ms
const unsigned int NXDN_RSSI_COUNT  = 28U;		// 28 * 40ms = 1120ms

CLCDproc::CLCDproc(std::string address, unsigned int port, unsigned int localPort, const std::string& callsign, unsigned int dmrid, bool displayClock, bool utc, bool duplex, bool dimOnIdle) :
CDisplay(),
m_address(address),
m_port(port),
m_localPort(localPort),
m_callsign(callsign),
m_dmrid(dmrid),
m_displayClock(displayClock),
m_utc(utc),
m_duplex(duplex),
//m_duplex(true),                      // uncomment to force duplex display for testing!
m_dimOnIdle(dimOnIdle),
m_dmr(false),
m_clockDisplayTimer(1000U, 0U, 250U),   // Update the clock display every 250ms
m_rssiCount1(0U),
m_rssiCount2(0U)
{
}

CLCDproc::~CLCDproc()
{
}

bool CLCDproc::open()
{
	const char *server;
	unsigned int port, localPort;
	struct sockaddr_in serverAddress, clientAddress;
	struct hostent *h;

	server    = m_address.c_str();
	port      = m_port;
	localPort = m_localPort;


	/* Create TCP socket */
	m_socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socketfd == -1) {
		LogError("LCDproc, failed to create socket");
		return false;
	}

	/* Sets client address (random port - need to specify manual port from ini file?) */
	clientAddress.sin_family      = AF_INET;
	clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	//clientAddress.sin_port        = htons(0);
	clientAddress.sin_port        = htons(localPort);

	/* Bind the address to the socket */
	if (bind(m_socketfd, (struct sockaddr *)&clientAddress, sizeof(clientAddress)) == -1) {
		LogError("LCDproc, error whilst binding address");
		return false;
	}

	/* Lookup the hostname address */
	h = gethostbyname(server);

	/* Sets server address */
	serverAddress.sin_family = h->h_addrtype;
	memcpy((char*)&serverAddress.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
	serverAddress.sin_port = htons(port);

	if (connect(m_socketfd, (struct sockaddr * )&serverAddress, sizeof(serverAddress))==-1) {
		LogError("LCDproc, cannot connect to server");
		return false;
	}

	socketPrintf(m_socketfd, "hello");   // Login to the LCD server
	socketPrintf(m_socketfd, "output 0");   // Clear all LEDs

	return true;
}

void CLCDproc::setIdleInt()
{
	m_clockDisplayTimer.start();          // Start the clock display in IDLE only

	if (m_screensDefined) {
		socketPrintf(m_socketfd, "screen_set DStar -priority hidden");
		socketPrintf(m_socketfd, "screen_set DMR -priority hidden");
		socketPrintf(m_socketfd, "screen_set YSF -priority hidden");
		socketPrintf(m_socketfd, "screen_set P25 -priority hidden");
		socketPrintf(m_socketfd, "screen_set NXDN -priority hidden");
		socketPrintf(m_socketfd, "widget_set Status Status %u %u Idle", m_cols - 3, m_rows);
		socketPrintf(m_socketfd, "output 0");   // Clear all LEDs
	}

	m_dmr = false;
}

void CLCDproc::setErrorInt(const char* text)
{
	assert(text != NULL);

	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_screensDefined) {
		socketPrintf(m_socketfd, "screen_set DStar -priority hidden");
		socketPrintf(m_socketfd, "screen_set DMR -priority hidden");
		socketPrintf(m_socketfd, "screen_set YSF -priority hidden");
		socketPrintf(m_socketfd, "screen_set P25 -priority hidden");
		socketPrintf(m_socketfd, "screen_set NXDN -priority hidden");
		socketPrintf(m_socketfd, "widget_set Status Status %u %u Error", m_cols - 4, m_rows);
		socketPrintf(m_socketfd, "output 0");   // Clear all LEDs
	}

	m_dmr = false;
}

void CLCDproc::setLockoutInt()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_screensDefined) {
		socketPrintf(m_socketfd, "screen_set DStar -priority hidden");
		socketPrintf(m_socketfd, "screen_set DMR -priority hidden");
		socketPrintf(m_socketfd, "screen_set YSF -priority hidden");
		socketPrintf(m_socketfd, "screen_set P25 -priority hidden");
		socketPrintf(m_socketfd, "screen_set NXDN -priority hidden");
		socketPrintf(m_socketfd, "widget_set Status Status %u %u Lockout", m_cols - 6, m_rows);
		socketPrintf(m_socketfd, "output 0");   // Clear all LEDs
	}

	m_dmr = false;
}

// LED 4 Green 8 Red 128 Yellow 136

void CLCDproc::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "screen_set DStar -priority foreground");
	socketPrintf(m_socketfd, "widget_set DStar Mode 1 1 \"D-Star\"");

	::sprintf(m_displayBuffer1, "%.8s", your);

	char *p = m_displayBuffer1;
	for (; *p; ++p) {
		if (*p == ' ')
			*p = '_';
	}

	if (strcmp(reflector, "        ") != 0)
		sprintf(m_displayBuffer2, " via %.8s", reflector);
	else
		memset(m_displayBuffer2, 0, BUFFER_MAX_LEN);

	if (m_rows == 2U) {
		socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 %u 2 h 3 \"%.8s/%.4s to %s%s\"", m_cols - 1, my1, my2, m_displayBuffer1, m_displayBuffer2);
	} else {
		socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 %u 2 h 3 \"%.8s/%.4s\"", m_cols - 1, my1, my2);
		socketPrintf(m_socketfd, "widget_set DStar Line3 1 3 %u 3 h 3 \"%s%s\"", m_cols - 1, m_displayBuffer1, m_displayBuffer2);
		socketPrintf(m_socketfd, "output 128"); // Set LED4 color red
	}

	m_dmr = false;
	m_rssiCount1 = 0U;
}

void CLCDproc::writeDStarRSSIInt(unsigned char rssi)
{
	if (m_rssiCount1 == 0U) {
		socketPrintf(m_socketfd, "widget_set DStar Line4 1 4 %u 4 h 3 \"-%3udBm\"", m_cols - 1, rssi);
	}
 
	m_rssiCount1++;
 	if (m_rssiCount1 >= DSTAR_RSSI_COUNT)
 		m_rssiCount1 = 0U;
}

void CLCDproc::clearDStarInt()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set DStar Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set DStar Line4 1 4 15 4 h 3 \"\"");
	socketPrintf(m_socketfd, "output 8"); // Set LED4 color green
}

// LED 1 Green 1 Red 16 Yellow 17

void CLCDproc::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (!m_dmr) {
		m_clockDisplayTimer.stop();          // Stop the clock display

		socketPrintf(m_socketfd, "screen_set DMR -priority foreground");

		if (m_duplex) {
			if (m_rows > 2U)
				socketPrintf(m_socketfd, "widget_set DMR Mode 1 1 DMR");
			if (slotNo == 1U)
				socketPrintf(m_socketfd, "widget_set DMR Slot2 3 %u %u %u h 3 \"Listening\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1);
			else
				socketPrintf(m_socketfd, "widget_set DMR Slot1 3 %u %u %u h 3 \"Listening\"", m_rows / 2, m_cols - 1, m_rows / 2);
		} else {
			socketPrintf(m_socketfd, "widget_set DMR Slot1_ 1 %u \"\"", m_rows / 2);
			socketPrintf(m_socketfd, "widget_set DMR Slot2_ 1 %u \"\"", m_rows / 2 + 1);

			socketPrintf(m_socketfd, "widget_set DMR Slot1 1 %u %u %u h 3 \"Listening\"", m_rows / 2, m_cols - 1, m_rows / 2);
			socketPrintf(m_socketfd, "widget_set DMR Slot2 1 %u %u %u h 3 \"\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1);
		}
	}

	if (m_duplex) {
		if (m_rows > 2U)
			socketPrintf(m_socketfd, "widget_set DMR Mode 1 1 DMR");

		if (slotNo == 1U)
			socketPrintf(m_socketfd, "widget_set DMR Slot1 3 %u %u %u h 3 \"%s > %s%s\"", m_rows / 2, m_cols - 1, m_rows / 2, src.c_str(), group ? "TG" : "", dst.c_str());
		else
			socketPrintf(m_socketfd, "widget_set DMR Slot2 3 %u %u %u h 3 \"%s > %s%s\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1, src.c_str(), group ? "TG" : "", dst.c_str());
	} else {
		socketPrintf(m_socketfd, "widget_set DMR Mode 1 1 DMR");

		if (m_rows == 2U) {
			socketPrintf(m_socketfd, "widget_set DMR Slot1 1 2 %u 2 h 3 \"%s > %s%s\"", m_cols - 1, src.c_str(), group ? "TG" : "", dst.c_str());
		} else {
			socketPrintf(m_socketfd, "widget_set DMR Slot1 1 2 %u 2 h 3 \"%s >\"", m_cols - 1, src.c_str());
			socketPrintf(m_socketfd, "widget_set DMR Slot2 1 3 %u 3 h 3 \"%s%s\"", m_cols - 1, group ? "TG" : "", dst.c_str());
		}
	}
	socketPrintf(m_socketfd, "output 16"); // Set LED1 color red
	m_dmr = true;
	m_rssiCount1 = 0U; 
  m_rssiCount2 = 0U; 
} 
 
void CLCDproc::writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi) 
{ 
	if (m_rows > 2) {	
	  if (slotNo == 1U) {
		  if (m_rssiCount1 == 0U)
				socketPrintf(m_socketfd, "widget_set DMR Slot1RSSI %u %u -%3udBm", 1, 4, rssi); 

			m_rssiCount1++; 

			if (m_rssiCount1 >= DMR_RSSI_COUNT)
				m_rssiCount1 = 0U; 
		} else { 
			if (m_rssiCount2 == 0U)
				socketPrintf(m_socketfd, "widget_set DMR Slot2RSSI %u %u -%3udBm", (m_cols / 2) + 1, 4, rssi); 

			m_rssiCount2++; 

			if (m_rssiCount2 >= DMR_RSSI_COUNT)
				m_rssiCount2 = 0U; 
		} 
	}
}

void CLCDproc::clearDMRInt(unsigned int slotNo)
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_duplex) {
		if (slotNo == 1U) {
			socketPrintf(m_socketfd, "widget_set DMR Slot1 3 %u %u %u h 3 \"Listening\"", m_rows / 2, m_cols - 1, m_rows / 2);
			socketPrintf(m_socketfd, "widget_set DMR Slot1RSSI %u %u %*.s", 1, 4, m_cols / 2, "          ");
		} else {
			socketPrintf(m_socketfd, "widget_set DMR Slot2 3 %u %u %u h 3 \"Listening\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1);
			socketPrintf(m_socketfd, "widget_set DMR Slot2RSSI %u %u %*.s", (m_cols / 2) + 1, 4, m_cols / 2, "          ");
		}
	} else {
		socketPrintf(m_socketfd, "widget_set DMR Slot1 1 2 15 2 h 3 Listening");
		socketPrintf(m_socketfd, "widget_set DMR Slot2 1 3 15 3 h 3 \"\"");
		socketPrintf(m_socketfd, "widget_set DMR Slot2RSSI %u %u %*.s", (m_cols / 2) + 1, 4, m_cols / 2, "          ");
	}
	socketPrintf(m_socketfd, "output 1"); // Set LED1 color green
}

// LED 3 Green 4 Red 64 Yellow 68

void CLCDproc::writeFusionInt(const char* source, const char* dest, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "screen_set YSF -priority foreground");
	socketPrintf(m_socketfd, "widget_set YSF Mode 1 1 \"System Fusion\"");

	if (m_rows == 2U) {
		socketPrintf(m_socketfd, "widget_set YSF Line2 1 2 15 2 h 3 \"%.10s > %s%u\"", source, dest);
	} else {
		socketPrintf(m_socketfd, "widget_set YSF Line2 1 2 15 2 h 3 \"%.10s >\"", source);
		socketPrintf(m_socketfd, "widget_set YSF Line3 1 3 15 3 h 3 \"%s%u\"", dest);
		socketPrintf(m_socketfd, "output 64"); // Set LED3 color red
	}

	m_dmr = false;
	m_rssiCount1 = 0U;
}

void CLCDproc::writeFusionRSSIInt(unsigned char rssi)
{
	if (m_rssiCount1 == 0U) {
		socketPrintf(m_socketfd, "widget_set YSF Line4 1 4 %u 4 h 3 \"-%3udBm\"", m_cols - 1, rssi);
	}
 
	m_rssiCount1++;
 	if (m_rssiCount1 >= YSF_RSSI_COUNT)
 		m_rssiCount1 = 0U;
}

void CLCDproc::clearFusionInt()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "widget_set YSF Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set YSF Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set YSF Line4 1 4 15 4 h 3 \"\"");
	socketPrintf(m_socketfd, "output 4"); // Set LED3 color green
}

// LED 2 Green 2 Red 32 Yellow 34

void CLCDproc::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "screen_set P25 -priority foreground");
	socketPrintf(m_socketfd, "widget_set P25 Mode 1 1 P25");

	if (m_rows == 2U) {
		socketPrintf(m_socketfd, "widget_set P25 Line2 1 2 15 2 h 3 \"%.10s > %s%u\"", source, group ? "TG" : "", dest);
	} else {
		socketPrintf(m_socketfd, "widget_set P25 Line2 1 2 15 2 h 3 \"%.10s >\"", source);
		socketPrintf(m_socketfd, "widget_set P25 Line3 1 3 15 3 h 3 \"%s%u\"", group ? "TG" : "", dest);
		socketPrintf(m_socketfd, "output 32"); // Set LED2 color red
	}

	m_dmr = false;
	m_rssiCount1 = 0U;
}

void CLCDproc::writeP25RSSIInt(unsigned char rssi)
{
	if (m_rssiCount1 == 0U) {
		socketPrintf(m_socketfd, "widget_set P25 Line4 1 4 %u 4 h 3 \"-%3udBm\"", m_cols - 1, rssi);
	}
 
	m_rssiCount1++;
 	if (m_rssiCount1 >= P25_RSSI_COUNT)
 		m_rssiCount1 = 0U;
}

void CLCDproc::clearP25Int()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "widget_set P25 Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set P25 Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set P25 Line4 1 4 15 4 h 3 \"\"");
	socketPrintf(m_socketfd, "output 2"); // Set LED2 color green
}

// LED 5 Green 16 Red 255 Yellow 255

void CLCDproc::writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "screen_set NXDN -priority foreground");
	socketPrintf(m_socketfd, "widget_set NXDN Mode 1 1 NXDN");

	if (m_rows == 2U) {
		socketPrintf(m_socketfd, "widget_set NXDN Line2 1 2 15 2 h 3 \"%.10s > %s%u\"", source, group ? "TG" : "", dest);
	} else {
		socketPrintf(m_socketfd, "widget_set NXDN Line2 1 2 15 2 h 3 \"%.10s >\"", source);
		socketPrintf(m_socketfd, "widget_set NXDN Line3 1 3 15 3 h 3 \"%s%u\"", group ? "TG" : "", dest);
		socketPrintf(m_socketfd, "output 255"); // Set LED5 color red
	}

	m_dmr = false;
	m_rssiCount1 = 0U;
}

void CLCDproc::writeNXDNRSSIInt(unsigned char rssi)
{
	if (m_rssiCount1 == 0U) {
		socketPrintf(m_socketfd, "widget_set NXDN Line4 1 4 %u 4 h 3 \"-%3udBm\"", m_cols - 1, rssi);
	}

	m_rssiCount1++;
	if (m_rssiCount1 >= NXDN_RSSI_COUNT)
		m_rssiCount1 = 0U;
}

void CLCDproc::clearNXDNInt()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "widget_set NXDN Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set NXDN Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set NXDN Line4 1 4 15 4 h 3 \"\"");
	socketPrintf(m_socketfd, "output 16"); // Set LED5 color green
}

void CLCDproc::writePOCSAGInt(uint32_t ric, const std::string& message)
{
}

void CLCDproc::clearPOCSAGInt()
{
}

void CLCDproc::writeCWInt()
{
}

void CLCDproc::clearCWInt()
{
}

void CLCDproc::clockInt(unsigned int ms)
{
	m_clockDisplayTimer.clock(ms);

	// Idle clock display
	if (m_displayClock && m_clockDisplayTimer.isRunning() && m_clockDisplayTimer.hasExpired()) {
		time_t currentTime;
		struct tm *Time;
		time(&currentTime);

		if (m_utc)
			Time = gmtime(&currentTime);
		else
			Time = localtime(&currentTime);

		setlocale(LC_TIME, "");
		strftime(m_displayBuffer1, 128, "%X", Time);  // Time
		strftime(m_displayBuffer2, 128, "%x", Time);  // Date

		if (m_cols < 26U && m_rows == 2U) {
			socketPrintf(m_socketfd, "widget_set Status Time %u 2 \"%s%s\"", m_cols - 9, strlen(m_displayBuffer1) > 8 ? "" : "  ", m_displayBuffer1);
		} else {
			socketPrintf(m_socketfd, "widget_set Status Time %u %u \"%s\"", (m_cols - (strlen(m_displayBuffer1) == 8 ? 6 : 8)) / 2, m_rows / 2, m_displayBuffer1);
			socketPrintf(m_socketfd, "widget_set Status Date %u %u \"%s\"", (m_cols - (strlen(m_displayBuffer1) == 8 ? 6 : 8)) / 2, m_rows / 2 + 1, m_displayBuffer2);
		}

		m_clockDisplayTimer.start();
	}

	// We must set all this information on each select we do
	FD_ZERO(&m_readfds);   // empty readfds

	// Then we put all the descriptors we want to wait for in a mask = m_readfds
	FD_SET(m_socketfd, &m_readfds);

	// Timeout, we will stop waiting for information
	m_timeout.tv_sec = 0;
	m_timeout.tv_usec = 0;

	/* The first parameter is the biggest descriptor + 1. The first one was 0, so
	 * every other descriptor will be bigger
	 *
	 * readfds = &m_readfds
	 * writefds = we are not waiting for writefds
	 * exceptfds = we are not waiting for exception fds
	 */

	if (select(m_socketfd + 1, &m_readfds, NULL, NULL, &m_timeout) == -1)
		LogError("LCDproc, error on select");

	// If something was received from the server...
	if (FD_ISSET(m_socketfd, &m_readfds)) {
		m_recvsize = recv(m_socketfd, m_buffer, BUFFER_MAX_LEN, 0);

		if (m_recvsize == -1)
			LogError("LCDproc, cannot receive information");

		m_buffer[m_recvsize] = '\0';

		char *argv[256];
		size_t len = strlen(m_buffer);

		// Now split the string into tokens...
		int argc = 0;
		int newtoken = 1;

		for (size_t i = 0U; i < len; i++) {
			switch (m_buffer[i]) {
				case ' ':
					newtoken = 1;
					m_buffer[i] = 0;
					break;
				default:	/* regular chars, keep tokenizing */
					if (newtoken)
						argv[argc++] = m_buffer + i;
					newtoken = 0;
					break;
				case '\0':
				case '\n':
					m_buffer[i] = 0;
					if (argc > 0) {
						if (0 == strcmp(argv[0], "listen")) {
							LogDebug("LCDproc, the %s screen is displayed", argv[1]);
						} else if (0 == strcmp(argv[0], "ignore")) {
							LogDebug("LCDproc, the %s screen is hidden", argv[1]);
						} else if (0 == strcmp(argv[0], "key")) {
							LogDebug("LCDproc, Key %s", argv[1]);
						} else if (0 == strcmp(argv[0], "menu")) {
						} else if (0 == strcmp(argv[0], "connect")) {
		 					// connect LCDproc 0.5.7 protocol 0.3 lcd wid 16 hgt 2 cellwid 5 cellhgt 8
							int a;

							for (a = 1; a < argc; a++) {
								if (0 == strcmp(argv[a], "wid"))
									m_cols = atoi(argv[++a]);
								else if (0 == strcmp(argv[a], "hgt"))
									m_rows = atoi(argv[++a]);
								else if (0 == strcmp(argv[a], "cellwid")) {
									//lcd_cellwid = atoi(argv[++a]);
								} else if (0 == strcmp(argv[a], "cellhgt")) {
									//lcd_cellhgt = atoi(argv[++a]);
								}
							}

							m_connected = true;
							socketPrintf(m_socketfd, "client_set -name MMDVMHost");
						} else if (0 == strcmp(argv[0], "bye")) {
							//close the socket- todo
						} else if (0 == strcmp(argv[0], "success")) {
							//LogDebug("LCDproc, command successful");
						} else if (0 == strcmp(argv[0], "huh?")) {
							sprintf(m_displayBuffer1, "LCDproc, command failed:");
							sprintf(m_displayBuffer2, " ");

							int j;
							for (j = 1; j < argc; j++) {
								strcat(m_displayBuffer1, m_displayBuffer2);
								strcat(m_displayBuffer1, argv[j]);
							}
							LogDebug("%s", m_displayBuffer1);
						}
					}

					/* Restart tokenizing */
					argc = 0;
					newtoken = 1;
					break;
			}	/* switch( m_buffer[i] ) */
		}
	}

	if (!m_screensDefined && m_connected)
		defineScreens();
}

void CLCDproc::close()
{
}

int CLCDproc::socketPrintf(int fd, const char *format, ...)
{
	char buf[BUFFER_MAX_LEN];
	va_list ap;

	va_start(ap, format);
	int size = vsnprintf(buf, BUFFER_MAX_LEN, format, ap);
	va_end(ap);

	if (size < 0) {
		LogError("LCDproc, socketPrintf: vsnprintf failed");
		return -1;
	}

	if (size > BUFFER_MAX_LEN)
		LogWarning("LCDproc, socketPrintf: vsnprintf truncated message");

	FD_ZERO(&m_writefds);   // empty writefds 
	FD_SET(m_socketfd, &m_writefds);

	m_timeout.tv_sec = 0;
	m_timeout.tv_usec = 0;

	if (select(m_socketfd + 1, NULL, &m_writefds, NULL, &m_timeout) == -1)
		LogError("LCDproc, error on select");

	if (FD_ISSET(m_socketfd, &m_writefds)) {
		if (send(m_socketfd, buf, int(strlen(buf) + 1U), 0) == -1) {
			LogError("LCDproc, cannot send data");
			return -1;
		}
	}

	return 0;
}

void CLCDproc::defineScreens()
{
	// The Status Screen

	socketPrintf(m_socketfd, "screen_add Status");
	socketPrintf(m_socketfd, "screen_set Status -name Status -heartbeat on -priority info -backlight %s", m_dimOnIdle ? "off" : "on");

	socketPrintf(m_socketfd, "widget_add Status Callsign string");
	socketPrintf(m_socketfd, "widget_add Status DMRNumber string");
	socketPrintf(m_socketfd, "widget_add Status Title string");
	socketPrintf(m_socketfd, "widget_add Status Status string");
	socketPrintf(m_socketfd, "widget_add Status Time string");
	socketPrintf(m_socketfd, "widget_add Status Date string");

	socketPrintf(m_socketfd, "widget_set Status Callsign 1 1 %s", m_callsign.c_str());
	socketPrintf(m_socketfd, "widget_set Status DMRNumber %u 1 %u", m_cols - 7, m_dmrid);
	socketPrintf(m_socketfd, "widget_set Status Title 1 %u MMDVM", m_rows);
	socketPrintf(m_socketfd, "widget_set Status Status %u %u Idle", m_cols - 3, m_rows);

	// The DStar Screen

	socketPrintf(m_socketfd, "screen_add DStar");
	socketPrintf(m_socketfd, "screen_set DStar -name DStar -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add DStar Mode string");
	socketPrintf(m_socketfd, "widget_add DStar Line2 scroller");
	socketPrintf(m_socketfd, "widget_add DStar Line3 scroller");
	socketPrintf(m_socketfd, "widget_add DStar Line4 scroller");

/* Do we need to pre-populate the values??
	socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set DStar Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set DStar Line4 1 4 15 4 h 3 \"\"");
*/

	// The DMR Screen

	socketPrintf(m_socketfd, "screen_add DMR");
	socketPrintf(m_socketfd, "screen_set DMR -name DMR -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add DMR Mode string");
	socketPrintf(m_socketfd, "widget_add DMR Slot1_ string");
	socketPrintf(m_socketfd, "widget_add DMR Slot2_ string");
	socketPrintf(m_socketfd, "widget_add DMR Slot1 scroller");
	socketPrintf(m_socketfd, "widget_add DMR Slot2 scroller");
	socketPrintf(m_socketfd, "widget_add DMR Slot1RSSI string");
	socketPrintf(m_socketfd, "widget_add DMR Slot2RSSI string");

/* Do we need to pre-populate the values??
	socketPrintf(m_socketfd, "widget_set DMR Slot1_ 1 %u 1", m_rows / 2);
	socketPrintf(m_socketfd, "widget_set DMR Slot2_ 1 %u 2", m_rows / 2 + 1);
	socketPrintf(m_socketfd, "widget_set DMR Slot1 3 1 15 1 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set DMR Slot2 3 2 15 2 h 3 Listening");
*/

	// The YSF Screen

	socketPrintf(m_socketfd, "screen_add YSF");
	socketPrintf(m_socketfd, "screen_set YSF -name YSF -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add YSF Mode string");
	socketPrintf(m_socketfd, "widget_add YSF Line2 scroller");
	socketPrintf(m_socketfd, "widget_add YSF Line3 scroller");
	socketPrintf(m_socketfd, "widget_add YSF Line4 scroller");

/* Do we need to pre-populate the values??
	socketPrintf(m_socketfd, "widget_set YSF Line2 2 1 15 1 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set YSF Line3 3 1 15 1 h 3 \" \"");
	socketPrintf(m_socketfd, "widget_set YSF Line4 4 2 15 2 h 3 \" \"");
*/

	// The P25 Screen

	socketPrintf(m_socketfd, "screen_add P25");
	socketPrintf(m_socketfd, "screen_set P25 -name P25 -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add P25 Mode string");
	socketPrintf(m_socketfd, "widget_add P25 Line2 scroller");
	socketPrintf(m_socketfd, "widget_add P25 Line3 scroller");
	socketPrintf(m_socketfd, "widget_add P25 Line4 scroller");

/* Do we need to pre-populate the values??
	socketPrintf(m_socketfd, "widget_set P25 Line3 2 1 15 1 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set P25 Line3 3 1 15 1 h 3 \" \"");
	socketPrintf(m_socketfd, "widget_set P25 Line4 4 2 15 2 h 3 \" \"");
*/

	// The NXDN Screen

	socketPrintf(m_socketfd, "screen_add NXDN");
	socketPrintf(m_socketfd, "screen_set NXDN -name NXDN -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add NXDN Mode string");
	socketPrintf(m_socketfd, "widget_add NXDN Line2 scroller");
	socketPrintf(m_socketfd, "widget_add NXDN Line3 scroller");
	socketPrintf(m_socketfd, "widget_add NXDN Line4 scroller");

/* Do we need to pre-populate the values??
	socketPrintf(m_socketfd, "widget_set NXDN Line3 2 1 15 1 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set NXDN Line3 3 1 15 1 h 3 \" \"");
	socketPrintf(m_socketfd, "widget_set NXDN Line4 4 2 15 2 h 3 \" \"");
*/

	m_screensDefined = true;
}
