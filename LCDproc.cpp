/*
 *   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "LCDproc.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

#define BUFFER_MAX_LEN 128
#define SOCKET_TIMEOUT 2500            //2500us (2.5ms) works, but 10ms was enough not to disprupt audio processing

const char* LISTENING = "Listening                               ";
const char* DEADSPACE = "                                        ";

int            m_socketfd;
char           m_buffer[BUFFER_MAX_LEN];
fd_set         m_readmask;
struct timeval m_timeout;
int            m_recvsize;
unsigned int   m_rows(0);
unsigned int   m_cols(0);
bool           m_screensDefined(false);
bool           m_connected(false);

char           m_buffer1[BUFFER_MAX_LEN];
char           m_buffer2[BUFFER_MAX_LEN];

CLCDproc::CLCDproc(std::string address, unsigned int port, unsigned int localPort, const std::string& callsign, unsigned int dmrid, bool displayClock, bool utc, bool duplex) :
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
m_dmr(false),
m_clockDisplayTimer(1000U, 0U, 250U)   // Update the clock display every 250ms
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
  memcpy((char*) &serverAddress.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
  serverAddress.sin_port = htons(port);

  if (connect(m_socketfd, (struct sockaddr * )&serverAddress, sizeof(serverAddress))==-1) {
    LogError("LCDproc, cannot connect to server");
		return false;
	}

	socketPrintf(m_socketfd, "hello");   // Login to the LCD server
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
		socketPrintf(m_socketfd, "widget_set Status Status %u %u Idle", m_cols - 3, m_rows);
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
		socketPrintf(m_socketfd, "widget_set Status Status %u %u Error", m_cols - 4, m_rows);
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
		socketPrintf(m_socketfd, "widget_set Status Status %u %u Lockout", m_cols - 6, m_rows);
	}

	m_dmr = false;
}

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

	::sprintf(m_buffer1, "%.8s", your);
	
	char *p = m_buffer1;
	for (; *p; ++p) {
		if (*p == ' ')
			*p = '_';
	}

	if (strcmp(reflector, "        ") != 0) {
		sprintf(m_buffer2, " via %.8s", reflector);
	} else {
		//bzero(m_buffer2, BUFFER_MAX_LEN);
		memset(m_buffer2, 0, BUFFER_MAX_LEN);
	}

	if (m_rows == 2) {
		socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 %u 2 h 3 \"%.8s/%.4s to %s%s\"", m_cols - 1, my1, my2, m_buffer1, m_buffer2);
	} else {
		socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 %u 2 h 3 \"%.8s/%.4s\"", m_cols - 1, my1, my2);
		socketPrintf(m_socketfd, "widget_set DStar Line3 1 3 %u 3 h 3 \"%s%s\"", m_cols - 1, m_buffer1, m_buffer2);
	}

	m_dmr = false;
}

void CLCDproc::clearDStarInt()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set DStar Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set DStar Line4 1 4 15 4 h 3 \"\"");
}

void CLCDproc::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (!m_dmr) {
		m_clockDisplayTimer.stop();          // Stop the clock display

		socketPrintf(m_socketfd, "screen_set DMR -priority foreground");

		if (m_duplex) {
			if (m_rows > 2U) {
				socketPrintf(m_socketfd, "widget_set DMR Mode 1 1 DMR");
			}

			if (slotNo == 1U) {
				socketPrintf(m_socketfd, "widget_set DMR Slot2 3 %u %u %u h 3 \"Listening\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1);
			} else {
				socketPrintf(m_socketfd, "widget_set DMR Slot1 3 %u %u %u h 3 \"Listening\"", m_rows / 2, m_cols - 1, m_rows / 2);
			}
		} else {
			socketPrintf(m_socketfd, "widget_set DMR Slot1_ 1 %u \"\"", m_rows / 2);
			socketPrintf(m_socketfd, "widget_set DMR Slot2_ 1 %u \"\"", m_rows / 2 + 1);
	
			socketPrintf(m_socketfd, "widget_set DMR Slot1 1 %u %u %u h 3 \"Listening\"", m_rows / 2, m_cols - 1, m_rows / 2);
			socketPrintf(m_socketfd, "widget_set DMR Slot2 1 %u %u %u h 3 \"\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1);
		}
	}

	if (m_duplex) {
		if (m_rows > 2U) {
			socketPrintf(m_socketfd, "widget_set DMR Mode 1 1 DMR");
		}

		if (slotNo == 1U) {
			socketPrintf(m_socketfd, "widget_set DMR Slot1 3 %u %u %u h 3 \"%s > %s%s\"", m_rows / 2, m_cols - 1, m_rows / 2, src.c_str(), group ? "TG" : "", dst.c_str());
		} else {
			socketPrintf(m_socketfd, "widget_set DMR Slot2 3 %u %u %u h 3 \"%s > %s%s\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1, src.c_str(), group ? "TG" : "", dst.c_str());
		}
	} else {
		socketPrintf(m_socketfd, "widget_set DMR Mode 1 1 DMR");

		if (m_rows == 2U) {
			socketPrintf(m_socketfd, "widget_set DMR Slot1 1 2 %u 2 h 3 \"%s > %s%s\"", m_cols - 1, src.c_str(), group ? "TG" : "", dst.c_str());
		} else {
			socketPrintf(m_socketfd, "widget_set DMR Slot1 1 2 %u 2 h 3 \"%s >\"", m_cols - 1, src.c_str());
			socketPrintf(m_socketfd, "widget_set DMR Slot2 1 3 %u 3 h 3 \"%s%s\"", m_cols - 1, group ? "TG" : "", dst.c_str());
		}
	}

	m_dmr = true;
}

void CLCDproc::clearDMRInt(unsigned int slotNo)
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_duplex) {
		if (slotNo == 1U) {
			socketPrintf(m_socketfd, "widget_set DMR Slot1 3 %u %u %u h 3 \"Listening\"", m_rows / 2, m_cols - 1, m_rows / 2);
		} else {
			socketPrintf(m_socketfd, "widget_set DMR Slot2 3 %u %u %u h 3 \"Listening\"", m_rows / 2 + 1, m_cols - 1, m_rows / 2 + 1);
		}
	} else {
		socketPrintf(m_socketfd, "widget_set DMR Slot1 1 2 15 2 h 3 Listening");
		socketPrintf(m_socketfd, "widget_set DMR Slot2 1 3 15 3 h 3 \"\"");
	}
}

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
	}

	m_dmr = false;
}

void CLCDproc::clearFusionInt()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "widget_set YSF Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set YSF Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set YSF Line4 1 4 15 4 h 3 \"\"");
}

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
	}

	m_dmr = false;
}

void CLCDproc::clearP25Int()
{
	m_clockDisplayTimer.stop();           // Stop the clock display

	socketPrintf(m_socketfd, "widget_set P25 Line3 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set P25 Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set P25 Line4 1 4 15 4 h 3 \"\"");
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

		if (m_utc) {
			Time = gmtime(&currentTime);
		} else {
			Time = localtime(&currentTime);
		}
			
		setlocale(LC_TIME,"");
		strftime(m_buffer1, 128, "%X", Time);  // Time
		strftime(m_buffer2, 128, "%x", Time);  // Date

		if (m_cols < 26U && m_rows == 2U) {
			socketPrintf(m_socketfd, "widget_set Status Time %u 2 \"%s%s\"", m_cols - 9, strlen(m_buffer1) > 8 ? "" : "  ", m_buffer1);
		} else {
			socketPrintf(m_socketfd, "widget_set Status Time %u %u %s", (m_cols - (strlen(m_buffer1) == 8 ? 6 : 8)) / 2, m_rows / 2, m_buffer1);
			socketPrintf(m_socketfd, "widget_set Status Date %u %u %s", (m_cols - (strlen(m_buffer1) == 8 ? 6 : 8)) / 2, m_rows / 2 + 1, m_buffer2);
		}

		m_clockDisplayTimer.start();
	}

	// We must set all this information on each select we do
  FD_ZERO(&m_readmask);   // empty readmask 

  // Then we put all the descriptors we want to wait for in a mask = m_readmask
  FD_SET(m_socketfd, &m_readmask);
  FD_SET(STDIN_FILENO, &m_readmask); // STDIN_FILENO = 0 (standard input);

  // Timeout, we will stop waiting for information
  m_timeout.tv_sec = 0;
  m_timeout.tv_usec = SOCKET_TIMEOUT;

  /* The first parameter is the biggest descriptor + 1. The first one was 0, so 
   * every other descriptor will be bigger
   *
   * readfds = &m_readmask
   * writefds = we are not waiting for writefds
   * exceptfds = we are not waiting for exception fds 
	 */

  if (select(m_socketfd + 1, &m_readmask, NULL, NULL, &m_timeout) == -1)
	  LogError("LCDproc, error on select");

  // If something was received from the server...
  if (FD_ISSET(m_socketfd, &m_readmask)) {
	  m_recvsize = recv(m_socketfd, m_buffer, BUFFER_MAX_LEN, 0);

  	if (m_recvsize == -1)
  		LogError("LCDproc, cannot receive information");

  	m_buffer[m_recvsize] = '\0';

		int i = 0;
		char *argv[256];
		int argc, newtoken;
		int len = strlen(m_buffer);

		// Now split the string into tokens...
		argc = 0;
		newtoken = 1;

		for (i = 0; i < len; i++) {
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
							sprintf(m_buffer1, "LCDproc, command failed:");
							sprintf(m_buffer2, " ");

							int j;
							for (j = 1; j < argc; j++) {
								strcat(m_buffer1, m_buffer2);
								strcat(m_buffer1, argv[j]);
							}
							LogDebug("%s", m_buffer1);
						}
					}
	
					/* Restart tokenizing */
					argc = 0;
					newtoken = 1;
					break;
			}	/* switch( m_buffer[i] ) */
		}
	}

	if (!m_screensDefined && m_connected) {
		defineScreens();
	}

	// Uncomment the next section of code to test server commands from STDIN
	// only for debugging purposes!

	/*
	if (FD_ISSET(STDIN_FILENO, &m_readmask)) {
		fgets(m_buffer, BUFFER_MAX_LEN, stdin);

		if (send(m_socketfd, m_buffer, strlen(m_buffer) + 1, 0) == -1)
			LogError("LCDproc, cannot send data");
	}
	*/
}

void CLCDproc::close()
{
}

int CLCDproc::socketPrintf(int fd, const char *format, ...)
{
	char buf[BUFFER_MAX_LEN];
	va_list ap;
	unsigned int size = 0;

	va_start(ap, format);
  size = vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  if (size < 0) {
    LogError("LCDproc, socketPrintf: vsnprintf failed");
    return -1;
  }

  if (size > sizeof(buf))
    LogWarning("LCDproc, socketPrintf: vsnprintf truncated message");

	if (send(fd, buf, strlen(buf) + 1, 0) == 1) {
		LogError("LCDproc, socketSend: cannot send data");
		return -1;
	}
	return 1;
}

void CLCDproc::defineScreens()
{
	// The Status Screen

	socketPrintf(m_socketfd, "screen_add Status");
	socketPrintf(m_socketfd, "screen_set Status -name Status -heartbeat on -priority info -backlight off");

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

	socketPrintf(m_socketfd, "widget_set DStar Line2 1 2 15 2 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set DStar Line3 1 3 15 3 h 3 \"\"");
	socketPrintf(m_socketfd, "widget_set DStar Line4 1 4 15 4 h 3 \"\"");

	// The DMR Screen

	socketPrintf(m_socketfd, "screen_add DMR");
	socketPrintf(m_socketfd, "screen_set DMR -name DMR -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add DMR Mode string");
	socketPrintf(m_socketfd, "widget_add DMR Slot1_ string");
	socketPrintf(m_socketfd, "widget_add DMR Slot2_ string");
	socketPrintf(m_socketfd, "widget_add DMR Slot1 scroller");
	socketPrintf(m_socketfd, "widget_add DMR Slot2 scroller");

	socketPrintf(m_socketfd, "widget_set DMR Slot1_ 1 %u 1", m_rows / 2);
	socketPrintf(m_socketfd, "widget_set DMR Slot2_ 1 %u 2", m_rows / 2 + 1);
	socketPrintf(m_socketfd, "widget_set DMR Slot1 3 1 15 1 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set DMR Slot2 3 2 15 2 h 3 Listening");

	// The YSF Screen

	socketPrintf(m_socketfd, "screen_add YSF");
	socketPrintf(m_socketfd, "screen_set YSF -name YSF -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add YSF Mode string");
	socketPrintf(m_socketfd, "widget_add YSF Line2 scroller");
	socketPrintf(m_socketfd, "widget_add YSF Line3 scroller");
	socketPrintf(m_socketfd, "widget_add YSF Line4 scroller");

	socketPrintf(m_socketfd, "widget_set YSF Line2 2 1 15 1 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set YSF Line3 3 1 15 1 h 3 \" \"");
	socketPrintf(m_socketfd, "widget_set YSF Line4 4 2 15 2 h 3 \" \"");

	// The P25 Screen

	socketPrintf(m_socketfd, "screen_add P25");
	socketPrintf(m_socketfd, "screen_set P25 -name P25 -heartbeat on -priority hidden -backlight on");

	socketPrintf(m_socketfd, "widget_add P25 Mode string");
	socketPrintf(m_socketfd, "widget_add P25 Line2 scroller");
	socketPrintf(m_socketfd, "widget_add P25 Line3 scroller");
	socketPrintf(m_socketfd, "widget_add P25 Line4 scroller");

	socketPrintf(m_socketfd, "widget_set P25 Line3 2 1 15 1 h 3 Listening");
	socketPrintf(m_socketfd, "widget_set P25 Line3 3 1 15 1 h 3 \" \"");
	socketPrintf(m_socketfd, "widget_set P25 Line4 4 2 15 2 h 3 \" \"");

	m_screensDefined = true;
}
