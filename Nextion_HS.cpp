/*
 *   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
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

#include "Nextion.h"
#include "Log.h"
#include "NetworkInfo.h"
#include "DMRSlot.h"


#include <iostream>
#include <cstring>
#include <stdint.h>

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>
#include <clocale>

/*
#include "Nextion.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>
#include <clocale>

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>


//#include <sys/socket.h>
#include <netdb.h>
//#include <ifaddrs.h>
//#include <linux/if_link.h>
*/


const unsigned int DSTAR_RSSI_COUNT = 3U;		  // 3 * 420ms = 1260ms
const unsigned int DSTAR_BER_COUNT  = 63U;		// 63 * 20ms = 1260ms
const unsigned int DMR_RSSI_COUNT   = 4U;		  // 4 * 360ms = 1440ms
const unsigned int DMR_BER_COUNT    = 24U;		// 24 * 60ms = 1440ms
const unsigned int YSF_RSSI_COUNT   = 13U;		// 13 * 100ms = 1300ms
const unsigned int YSF_BER_COUNT    = 13U;		// 13 * 100ms = 1300ms
const unsigned int P25_RSSI_COUNT   = 7U;		  // 7 * 180ms = 1260ms
const unsigned int P25_BER_COUNT    = 7U;		  // 7 * 180ms = 1260ms



CNextion::CNextion(const std::string& callsign, unsigned int dmrid, ISerialPort* serial, unsigned int brightness, bool displayClock, bool utc, unsigned int idleBrightness, unsigned int screenLayout, const std::string& filesConfig) :
CDisplay(),
m_callsign(callsign),
m_ipaddress("(ip unknown)"),
m_dmrid(dmrid),
m_serial(serial),
m_brightness(brightness),
m_mode(MODE_IDLE),
m_displayClock(displayClock),
m_utc(utc),
m_idleBrightness(idleBrightness),
m_screenLayout(screenLayout),
m_filesConfig(filesConfig),
m_clockDisplayTimer(1000U, 0U, 400U),
m_rssiAccum1(0U),
m_rssiAccum2(0U),
m_berAccum1(0.0F),
m_berAccum2(0.0F),
m_rssiCount1(0U),
m_rssiCount2(0U),
m_berCount1(0U),
m_berCount2(0U)



{
	assert(serial != NULL);
	assert(brightness >= 0U && brightness <= 100U);
}



CNextion::~CNextion()
{
}


//using namespace std;



bool CNextion::open()
{
        unsigned char info[100U];
        CNetworkInfo* m_network;

	bool ret = m_serial->open();
	if (!ret) {
		LogError("Cannot open the port for the Nextion display");
		delete m_serial;
		return false;
	}

        info[0]=0;
        m_network = new CNetworkInfo;
        m_network->getNetworkInterface(info);
        m_ipaddress = (char*)info;

	sendCommand("bkcmd=0");


	setIdle();

	return true;
}





void CNextion::setIdleInt()

{



// INI File Read

FILE *cfgfile;
 int maxline = 256;
 int i=0;
 char idles[4][256];

std::string info ("info.ini"); 
std::string path ("");
path = m_filesConfig + info;
const char* file = path.c_str();
cfgfile = fopen (file, "r");
if (cfgfile==NULL) printf("can't open file\n");
  while (fgets(idles[i], maxline, cfgfile)) {
     idles[i][strlen(idles[i])-1] = '\0';
    i++;
   }  
 fclose(cfgfile);
 


	sendCommand("page MMDVM");


	char command0[40];
	::sprintf(command0, "dim=%u", m_idleBrightness);
	sendCommand(command0);


	char command1[40U];
	::sprintf(command1, "t0.txt=\"%s/%u\"", m_callsign.c_str(), m_dmrid);
	sendCommand(command1);

	char command2[40U];
        ::sprintf(command2, "t30.txt=\"%s\"", idles[1]);
	sendCommand(command2);

	char command3[40U];
        ::sprintf(command3, "t31.txt=\"%s\"", idles[2]);      
	sendCommand(command3);

	char command4[40U];
        ::sprintf(command4, "t1.txt=\"%s\"", idles[3]);


	sendCommand(command4);



        sendCommand("t36.pco=60965");
	sendCommand("t36.txt=\"Idle\"");



	char text2[30U];
	::sprintf(text2, "t33.txt=\"%s\"", m_ipaddress.c_str());
	sendCommand(text2);



	m_clockDisplayTimer.start();

	m_mode = MODE_IDLE;

  //Parses file into objects in memory

}


void CNextion::setErrorInt(const char* text)
{
	assert(text != NULL);

	sendCommand("page MMDVM");

	char command[20];
	::sprintf(command, "dim=%u", m_brightness);
	sendCommand(command);
        sendCommand("t0.font=11");
	::sprintf(command, "t0.txt=\"%s\"", text);

	sendCommand(command);
	sendCommand("t1.txt=\"ERROR\"");

	m_clockDisplayTimer.stop();

	m_mode = MODE_ERROR;
}

void CNextion::setLockoutInt()
{
	sendCommand("page MMDVM");

	char command[20];
	::sprintf(command, "dim=%u", m_brightness);
	sendCommand(command);

	sendCommand("t0.txt=\"LOCKOUT\"");

	m_clockDisplayTimer.stop();

	m_mode = MODE_LOCKOUT;
}

void CNextion::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	if (m_mode != MODE_DSTAR)
		sendCommand("page DStar");

	char text[30U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"%s %.8s/%4.4s\"", type, my1, my2);
	sendCommand(text);

	::sprintf(text, "t1.txt=\"%.8s\"", your);
	sendCommand(text);

	if (::strcmp(reflector, "        ") != 0) {
		::sprintf(text, "t2.txt=\"via %.8s\"", reflector);
		sendCommand(text);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DSTAR;
	m_rssiAccum1 = 0U;
	m_berAccum1  = 0.0F;
	m_rssiCount1 = 0U;
	m_berCount1  = 0U;
}

void CNextion::writeDStarRSSIInt(unsigned char rssi)
{
	if (m_rssiCount1 == 0U) {
		char text[20U];
		::sprintf(text, "t3.txt=\"-%udBm\"", rssi);
		sendCommand(text);
		m_rssiCount1 = 1U;
		return;
	}

	m_rssiAccum1 += rssi;
	m_rssiCount1++;

	if (m_rssiCount1 == DSTAR_RSSI_COUNT) {
		char text[20U];
		::sprintf(text, "t3.txt=\"-%udBm\"", m_rssiAccum1 / DSTAR_RSSI_COUNT);
		sendCommand(text);
		m_rssiAccum1 = 0U;
		m_rssiCount1 = 1U;
	}
}

void CNextion::writeDStarBERInt(float ber)
{
	if (m_berCount1 == 0U) {
		char text[20U];
		::sprintf(text, "t4.txt=\"%.1f%%\"", ber);
		sendCommand(text);
		m_berCount1 = 1U;
		return;
	}

	m_berAccum1 += ber;
	m_berCount1++;

	if (m_berCount1 == DSTAR_BER_COUNT) {
		char text[20U];
		::sprintf(text, "t4.txt=\"%.1f%%\"", m_berAccum1 / float(DSTAR_BER_COUNT));
		sendCommand(text);
		m_berAccum1 = 0.0F;
		m_berCount1 = 1U;
	}
}

void CNextion::clearDStarInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
	sendCommand("t4.txt=\"\"");
}

void CNextion::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (m_mode != MODE_DMR) {
		sendCommand("page DMR");

//printf ("Pagina DMR\n");


		if (slotNo == 1U) {
			sendCommand("t2.pco=0");
			sendCommand("t2.font=11");
			sendCommand("t2.txt=\"DMR RX\"");
		} else {
			//SLOT1
		}
	}

	char text[130U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	if (slotNo == 1U) {
// SLOT1

	} else {

if (strcmp(type,"R") == 0) {
        sendCommand("t2.pco=2016");
        sendCommand("t36.pco=2016");
        sendCommand("t36.txt=\"Busy\"");
	sendCommand(text);

} else {
        sendCommand("t2.pco=1024");
        sendCommand("j0.val=99");
        sendCommand("j0.pco=63488");
        sendCommand("t36.pco=63488");
        sendCommand("t36.txt=\"Tx\"");
	sendCommand(text);
}


		::sprintf(text, "t2.txt=\"%s %s\"", type, src.c_str());
		

// INI Read Section TG

// INI File Read

FILE *cfgfileTG;
 int maxlineTG = 256;
 int iTG=0 ;
 char TGx[131][256];
  std::string tgs ("tg.ini"); 
  std::string pathtg ("");
  pathtg = m_filesConfig + tgs;
  const char* filetg = pathtg.c_str();
  cfgfileTG = fopen (filetg, "r");
  if (cfgfileTG==NULL) printf("can't open file\n");
  while (fgets(TGx[iTG], maxlineTG, cfgfileTG)) {
     TGx[iTG][strlen(TGx[iTG])-1] = '\0';
    iTG++;
   }
  fclose(cfgfileTG);





// READ INI SYSOP & CONTROL

 FILE *cfgfileCT;
 int maxlineCT = 256;
 int iCT=0;
 char ctrl[7][256];
  std::string tct ("ctrl.ini"); 
  std::string pathct ("");
  pathct = m_filesConfig + tct;
  const char* filect = pathct.c_str();
  cfgfileCT = fopen (filect, "r");
  if (cfgfileCT==NULL) printf("can't open file\n");
  while (fgets(ctrl[iCT], maxlineCT, cfgfileCT)) {
     ctrl[iCT][strlen(ctrl[iCT])-1] = '\0';
    iCT++;
   }  
 fclose(cfgfileCT);


/* READ INI PREFIXES
*
* A prefixes
*
*/

FILE *cfgfileA;
 int maxlineA = 256;
 int iA=0;
 char pfxA[10][256];
  std::string pfA ("prefixA.ini"); 
  std::string pathpfA ("");
  pathpfA = m_filesConfig + pfA;
  const char* filepfA = pathpfA.c_str();
  cfgfileA = fopen (filepfA, "r");
  if (cfgfileA==NULL) printf("can't open file\n");
  while (fgets(pfxA[iA], maxlineA, cfgfileA)) {
     pfxA[iA][strlen(pfxA[iA])-1] = '\0';
    iA++;
   }
 fclose(cfgfileA);


/* READ INI PREFIXES
*
* B prefixes
*
*/


FILE *cfgfileB;
 int maxlineB = 256;
 int iB=0;
  char pfxB[10][256];
  std::string pfB ("prefixB.ini"); 
  std::string pathpfB ("");
  pathpfB = m_filesConfig + pfB;
  const char* filepfB = pathpfB.c_str();
  cfgfileB = fopen (filepfB, "r");
  if (cfgfileB==NULL) printf("can't open file\n");
  while (fgets(pfxB[iB], maxlineB, cfgfileB)) {      // while not EOF
     pfxB[iB][strlen(pfxB[iB])-1] = '\0';      // remove CR from line
    iB++;
   }
 fclose(cfgfileB);

/* READ INI PREFIXES
*
* A prefixes
*
*/


FILE *cfgfileC;
 int maxlineC = 256;
 int iC=0;
 char pfxC[10][256];
 std::string pfC ("prefixC.ini"); 
  std::string pathpfC ("");
  pathpfC = m_filesConfig + pfC;
  const char* filepfC = pathpfC.c_str();
  cfgfileC = fopen (filepfC, "r");
  if (cfgfileC==NULL) printf("can't open file\n");
  while (fgets(pfxC[iC], maxlineC, cfgfileC)) {      // while not EOF
     pfxC[iC][strlen(pfxC[iC])-1] = '\0';      // remove CR from line
    iC++;
   }  
 fclose(cfgfileC);


/*
*
* Compare Prefixes
*/

for (int p_f=0;p_f<10;p_f=p_f+1)

if ((strncmp (src.c_str(),pfxA[p_f],3) == 0) || (strncmp (src.c_str(),pfxB[p_f],3) == 0) || (strncmp (src.c_str(),pfxC[p_f],3) == 0))
{
        char text[130U];
	int p_i=p_f+20;
	::sprintf(text, "p0.pic=%d",p_i);
	sendCommand(text);
}

// FIXED PREFIXES

else if (strcmp (src.c_str(),"9990") == 0) 
{
        char text[130U];
	::sprintf(text, "p0.pic=6");
	sendCommand(text);
}

else if (strcmp (src.c_str(),"4000") == 0) 
{
        char text[130U];
	::sprintf(text, "p0.pic=10");
	sendCommand(text);
}

else {
}

		sendCommand("t2.font=11");
		sendCommand(text);

// Compare ShutDown Routines

if ((strcmp (ctrl[3],dst.c_str()) ==0) && (strncmp (src.c_str(),ctrl[0],6) == 0) && (strcmp ("1",ctrl[1]) == 0)) 
{
	printf ("Reboot NOW\n");
	system("sudo shutdown -r now");
}

else if ((strcmp (ctrl[2],dst.c_str()) ==0) && (strncmp (src.c_str(),ctrl[0],6) == 0) && (strcmp ("1",ctrl[1]) == 0)) 
{
printf ("Shutdown NOW\n");
system("sudo shutdown -h now");
close();
}

else if ((strcmp (ctrl[4],dst.c_str()) ==0) && (strncmp (src.c_str(),ctrl[0],6) == 0) && (strcmp ("1",ctrl[1]) == 0)) 
{
printf ("DmrPlusmode\n");
system("mm_plus");
}

else if ((strcmp (ctrl[5],dst.c_str()) ==0) && (strncmp (src.c_str(),ctrl[0],6) == 0) && (strcmp ("1",ctrl[1]) == 0)) 
{
printf ("DMRGateway mode\n");
system("mm_gate");
}

else if ((strcmp (ctrl[6],dst.c_str()) ==0) && (strncmp (src.c_str(),ctrl[0],6) == 0) && (strcmp ("1",ctrl[1]) == 0)) 
{
printf ("BrandMeister mode\n");
system("mm_BM");
}


		::sprintf(text, "t3.txt=\"%s%s\"", group ? "TG: " : "", dst.c_str());


// Compare TG and write Flags


for (int flag=0;flag<131;flag=flag+1)

{

if (strcmp (TGx[flag],dst.c_str()) ==0) {
        char text[130U];
	int pi=flag+30;
	::sprintf(text, "p5.pic=%d",pi);
	sendCommand(text);
}



/*
*
* Fixed TG's
*
*/ 

else if ((strcmp ("9",dst.c_str()) ==0)||(strcmp ("8",dst.c_str()) ==0)) {
        char text[130U];
	::sprintf(text, "p5.pic=12");
	sendCommand(text);
}
else if (strcmp ("4000",dst.c_str()) ==0) {
        char text[130U];
	::sprintf(text, "p5.pic=11");
	sendCommand(text);
}
else if (strcmp (ctrl[0],dst.c_str()) ==0) {
        char text[130U];
	::sprintf(text, "p5.pic=9");
	sendCommand(text);
}
else if (strcmp ("9990",dst.c_str()) ==0) {
        char text[130U];
	::sprintf(text, "p5.pic=7");
	sendCommand(text);
}
else if (strcmp ("6",dst.c_str()) ==0) {
        char text[130U];
	::sprintf(text, "p5.pic=13");
	sendCommand(text);
}

else 
 {
// No group programmed
        char text[130U];
//	::sprintf(text, "p5.pic=5");
	sendCommand(text);
}}


                sendCommand ("t5.txt=\"RSSI: -10udBm\"");
                sendCommand ("t7.txt=\"Ber: 0.0%\"");
		sendCommand(text);


	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DMR;
	m_rssiAccum1 = 0U;
	m_rssiAccum2 = 0U;
	m_berAccum1  = 0.0F;
	m_berAccum2  = 0.0F;
	m_rssiCount1 = 0U;
	m_rssiCount2 = 0U;
	m_berCount1  = 0U;
	m_berCount2  = 0U;
}

void CNextion::writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi)
{
	if (slotNo == 1U) {
		//SLOT1
		
	} else {


		if (m_rssiCount2 == 0U) {
			char text[20U];


			::sprintf(text, "t5.txt=\"RSSI: -%udBm\"", rssi);

 



			sendCommand(text);
			m_rssiCount2 = 1U;

			return;
		}

		m_rssiAccum2 += rssi;
		m_rssiCount2++;

		if (m_rssiCount2 == DMR_RSSI_COUNT) {
			char text[20U];
			::sprintf(text, "t5.txt=\"RSSI: -%udBm\"", m_rssiAccum2 / DMR_RSSI_COUNT);
                        sendCommand(text);

//                 int smeter = (m_rssiAccum2 / DMR_RSSI_COUNT);


//int smeter2= (m_rssiAccum2 / DMR_RSSI_COUNT);

int smeter= (m_rssiAccum2/3);

//printf ("meterOK:%d\n",smeter);

if (smeter <= 150 && smeter >= 141) //S1 =8

{
        char command[30];
	::sprintf(command, "j0.val=8");
	sendCommand(command);
}
else if (smeter <= 141 && smeter >= 135) //S2= 18
{  
        char command[30];
	::sprintf(command, "j0.val=18");
	sendCommand(command);
}
else if (smeter <= 135 && smeter >= 129) //S3= 20
{  
        char command[30];
	::sprintf(command, "j0.val=20");
	sendCommand(command);
}
else if (smeter <= 129 && smeter>= 123) //S4=23
{   
        char command[30];
	::sprintf(command, "j0.val=23");
	sendCommand(command);
}
if (smeter <= 123 && smeter >= 117) //S5=28
{
        char command[30];
	::sprintf(command, "j0.val=28");
	sendCommand(command);
}
else if (smeter <= 117 && smeter >= 111) //S6=32
{  
        char command[30];
	::sprintf(command, "j0.val=32");
	sendCommand(command);
}
else if (smeter <=111  && smeter >= 105) //S7=38
{
         char command[30];
	::sprintf(command, "j0.val=38");
	sendCommand(command);
}
else if (smeter <= 105 && smeter>= 99) //S8=43
{
        char command[30];
	::sprintf(command, "j0.val=43");
	sendCommand(command);
}
else if (smeter <= 99 && smeter >= 93) //S9= 48
{
        char command[30];
	::sprintf(command, "j0.val=48");
	sendCommand(command);
}
else if (smeter <= 93 && smeter >= 83) //S9+10=55
{  
        char command[30];
	::sprintf(command, "j0.val=55");
	sendCommand(command);
}
else if (smeter <= 83 && smeter >= 73) //S9+20 =67
{  
        char command[30];
	::sprintf(command, "j0.val=67");
	sendCommand(command);
}
else if (smeter <= 73 && smeter>= 63) //S9+30 = 72
{   
        char command[30];
	::sprintf(command, "j0.val=72");
	sendCommand(command);
}
else if (smeter <= 53 && smeter >= 43) //S9+40=82
{
        char command[30];
	::sprintf(command, "j0.val=82");
	sendCommand(command);
}
else if (smeter <= 43 && smeter >= 33) //S9+50 =88
{ 
        char command[30];
	::sprintf(command, "j0.val=88");
	sendCommand(command);
}
else if (smeter <= 33 && smeter >= 10) //S9+60 =100
{ 
        char command[30];
	::sprintf(command, "j0.val=100");
	sendCommand(command);
}


			m_rssiAccum2 = 0U;
			m_rssiCount2 = 1U;

		}
	}
}


void CNextion::writeDMRTAInt(unsigned int slotNo,  unsigned char* talkerAlias, const char* type)

{
    char text[40U];

    if (type[0]==' ') {
        if (slotNo == 1U) {
	    sendCommand("t0.pco=33808");
        } else {
	    sendCommand("t2.pco=33808");
        }
        return;
    }


    if (slotNo == 1U) {
	//SLOT1
    } else {
	::sprintf(text, "t2.txt=\"%s %s\"",type,talkerAlias);

	if (strlen((char*)talkerAlias)>16-4) sendCommand("t2.font=11");
	if (strlen((char*)talkerAlias)>20-4) sendCommand("t2.font=11");
	if (strlen((char*)talkerAlias)>24-4) sendCommand("t2.font=11");

    if (strcmp(type,"R") == 0) {
        sendCommand("t2.pco=63488");
	sendCommand(text);
} else {
        sendCommand("t2.pco=1024");
	sendCommand(text);
}
    }
}


void CNextion::writeDMRBERInt(unsigned int slotNo, float ber)
{
	if (slotNo == 1U) {
		if (m_berCount1 == 0U) {
			char text[20U];
			::sprintf(text, "t6.txt=\"%.1f%%\"", ber);
			sendCommand(text);
			m_berCount1 = 1U;
			return;
		}

		m_berAccum1 += ber;
		m_berCount1++;

		if (m_berCount1 == DMR_BER_COUNT) {
			char text[20U];
			::sprintf(text, "t6.txt=\"%.1f%%\"", m_berAccum1 / DMR_BER_COUNT);
			sendCommand(text);
			m_berAccum1 = 0U;
			m_berCount1 = 1U;
		}
	} else {
		if (m_berCount2 == 0U) {
			char text[20U];
			::sprintf(text, "t7.txt=\"Ber: %.1f%%\"", ber);
			sendCommand(text);
			m_berCount2 = 1U;
			return;
		}

		m_berAccum2 += ber;
		m_berCount2++;

		if (m_berCount2 == DMR_BER_COUNT) {
			char text[20U];
			::sprintf(text, "t7.txt=\"Ber: %.1f%%\"", m_berAccum2 / DMR_BER_COUNT);
			sendCommand(text);
			m_berAccum2 = 0U;
			m_berCount2 = 1U;


		}
	}
}

void CNextion::clearDMRInt(unsigned int slotNo)
{
	if (slotNo == 1U) {
		sendCommand("t0.txt=\"1 Listening\"");
		sendCommand("t0.pco=0");
		sendCommand("t0.font=0");
		sendCommand("t1.txt=\"\"");
		sendCommand("t4.txt=\"\"");
		sendCommand("t6.txt=\"\"");
	} else {
		sendCommand("t2.txt=\"RX DMR\"");
		sendCommand("t2.pco=0");
		sendCommand("t2.font=11");
		sendCommand("t3.txt=\"\"");
		sendCommand("t5.txt=\"\"");
		sendCommand("t7.txt=\"\"");
		sendCommand("p0.pic=4");
		sendCommand("p5.pic=5");
	}
}

void CNextion::writeFusionInt(const char* source, const char* dest, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	if (m_mode != MODE_YSF)
		sendCommand("page YSF");

	char text[30U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"%s %.10s\"", type, source);
	sendCommand(text);

	::sprintf(text, "t1.txt=\"%.10s\"", dest);
	sendCommand(text);

	if (::strcmp(origin, "          ") != 0) {
		::sprintf(text, "t2.txt=\"at %.10s\"", origin);
		sendCommand(text);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_YSF;
	m_rssiAccum1 = 0U;
	m_berAccum1  = 0.0F;
	m_rssiCount1 = 0U;
	m_berCount1  = 0U;
}

void CNextion::writeFusionRSSIInt(unsigned char rssi)
{
	if (m_rssiCount1 == 0U) {
		char text[20U];
		::sprintf(text, "t3.txt=\"-%udBm\"", rssi);
		sendCommand(text);
		m_rssiCount1 = 1U;
		return;
	}

	m_rssiAccum1 += rssi;
	m_rssiCount1++;

	if (m_rssiCount1 == YSF_RSSI_COUNT) {
		char text[20U];
		::sprintf(text, "t3.txt=\"-%udBm\"", m_rssiAccum1 / YSF_RSSI_COUNT);
		sendCommand(text);
		m_rssiAccum1 = 0U;
		m_rssiCount1 = 1U;
	}
}

void CNextion::writeFusionBERInt(float ber)
{
	if (m_berCount1 == 0U) {
		char text[20U];
		::sprintf(text, "t4.txt=\"%.1f%%\"", ber);
		sendCommand(text);
		m_berCount1 = 1U;
		return;
	}

	m_berAccum1 += ber;
	m_berCount1++;

	if (m_berCount1 == YSF_BER_COUNT) {
		char text[20U];
		::sprintf(text, "t4.txt=\"%.1f%%\"", m_berAccum1 / float(YSF_BER_COUNT));
		sendCommand(text);
		m_berAccum1 = 0.0F;
		m_berCount1 = 1U;
	}
}

void CNextion::clearFusionInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
	sendCommand("t4.txt=\"\"");
}

void CNextion::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	if (m_mode != MODE_P25)
		sendCommand("page P25");

	char text[30U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"%s %.10s\"", type, source);
	sendCommand(text);

	::sprintf(text, "t1.txt=\"%s%u\"", group ? "TG" : "", dest);
	sendCommand(text);

	m_clockDisplayTimer.stop();

	m_mode = MODE_P25;
	m_rssiAccum1 = 0U;
	m_berAccum1  = 0.0F;
	m_rssiCount1 = 0U;
	m_berCount1  = 0U;
}

void CNextion::writeP25RSSIInt(unsigned char rssi)
{
	if (m_rssiCount1 == 0U) {
		char text[20U];
		::sprintf(text, "t2.txt=\"-%udBm\"", rssi);
		sendCommand(text);
		m_rssiCount1 = 1U;
		return;
	}

	m_rssiAccum1 += rssi;
	m_rssiCount1++;

	if (m_rssiCount1 == P25_RSSI_COUNT) {
		char text[20U];
		::sprintf(text, "t2.txt=\"-%udBm\"", m_rssiAccum1 / P25_RSSI_COUNT);
		sendCommand(text);
		m_rssiAccum1 = 0U;
		m_rssiCount1 = 1U;
	}
}

void CNextion::writeP25BERInt(float ber)
{
	if (m_berCount1 == 0U) {
		char text[20U];
		::sprintf(text, "t3.txt=\"%.1f%%\"", ber);
		sendCommand(text);
		m_berCount1 = 1U;
		return;
	}

	m_berAccum1 += ber;
	m_berCount1++;

	if (m_berCount1 == P25_BER_COUNT) {
		char text[20U];
		::sprintf(text, "t3.txt=\"%.1f%%\"", m_berAccum1 / float(P25_BER_COUNT));
		sendCommand(text);
		m_berAccum1 = 0.0F;
		m_berCount1 = 1U;
	}
}

void CNextion::clearP25Int()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
}

void CNextion::writeCWInt()
{
	sendCommand("t1.txt=\"Sending CW Ident\"");

	m_clockDisplayTimer.start();

	m_mode = MODE_CW;
}

void CNextion::clearCWInt()
{
	sendCommand("t1.txt=\"MMDVM IDLE\"");
}

void CNextion::clockInt(unsigned int ms)
{
	// Update the clock display in IDLE mode every 400ms
	m_clockDisplayTimer.clock(ms);
	if (m_displayClock && (m_mode == MODE_IDLE || m_mode == MODE_CW) && m_clockDisplayTimer.isRunning() && m_clockDisplayTimer.hasExpired()) {
		time_t currentTime;
		struct tm *Time;
		::time(&currentTime);                   // Get the current time

		if (m_utc)
			Time = ::gmtime(&currentTime);
		else
			Time = ::localtime(&currentTime);

		setlocale(LC_TIME,"");
		char text[50U];
		strftime(text, 50, "t2.txt=\"%x %X\"", Time);
		sendCommand(text);


FILE *temperatureFile;
double T;
temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
if (temperatureFile == NULL)
  ; //print some message
fscanf (temperatureFile, "%lf", &T);
T /= 1000;
//printf ("The temperature is %6.3f C.\n", T);



// Temperatura sin decimales

//int temp = (int)T;


//printf ("Mitemp:%d\n",temp);

        char text1[30U];
	::sprintf(text1, "t32.txt=\"Temp:%6.3f\"", T);
//	::sprintf(text1, "t32.txt=\"Temp:%d\"", temp);
        sendCommand(text1);

fclose (temperatureFile);


		m_clockDisplayTimer.start(); // restart the clock display timer
	}
}



// CPU USAGE

//



void CNextion::close()
{
	sendCommand("page MMDVM");
	sendCommand("t1.txt=\"MMDVM STOP\"");
	sendCommand("t0.txt=\"MMDVM STOP\"");
	sendCommand("t30.txt=\"MMDVM STOP\"");
	m_serial->close();
	delete m_serial;
}

void CNextion::sendCommand(const char* command)
{
	assert(command != NULL);

	m_serial->write((unsigned char*)command, ::strlen(command));
	m_serial->write((unsigned char*)"\xFF\xFF\xFF", 3U);
}

