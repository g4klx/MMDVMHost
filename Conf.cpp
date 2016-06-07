/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "DStarDefines.h"
#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum SECTION {
  SECTION_NONE,
  SECTION_GENERAL,
  SECTION_INFO,
  SECTION_LOG,
  SECTION_CWID,
  SECTION_MODEM,
  SECTION_DSTAR,
  SECTION_DMR,
  SECTION_FUSION,
  SECTION_DSTAR_NETWORK,
  SECTION_DMR_NETWORK,
  SECTION_FUSION_NETWORK,
  SECTION_TFTSERIAL,
  SECTION_HD44780,
  SECTION_NEXTION,
  SECTION_OLED
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_timeout(120U),
m_duplex(true),
m_modeHang(10U),
m_display(),
m_daemon(false),
m_rxFrequency(0U),
m_txFrequency(0U),
m_power(0U),
m_latitude(0.0F),
m_longitude(0.0F),
m_height(0),
m_location(),
m_description(),
m_url(),
m_logDisplayLevel(0U),
m_logFileLevel(0U),
m_logFilePath(),
m_logFileRoot(),
m_cwIdEnabled(false),
m_cwIdTime(10U),
m_modemPort(),
m_modemRXInvert(false),
m_modemTXInvert(false),
m_modemPTTInvert(false),
m_modemTXDelay(100U),
m_modemDMRDelay(0U),
m_modemRXLevel(100U),
m_modemTXLevel(100U),
m_modemOscOffset(0),
m_modemDebug(false),
m_dstarEnabled(true),
m_dstarModule("C"),
m_dstarSelfOnly(false),
m_dstarBlackList(),
m_dmrEnabled(true),
m_dmrBeacons(false),
m_dmrId(0U),
m_dmrColorCode(2U),
m_dmrSelfOnly(false),
m_dmrPrefixes(),
m_dmrBlackList(),
m_dmrDstIdBlacklistSlot1(),
m_dmrDstIdBlacklistSlot2(),
m_dmrDstIdWhitelistSlot1(),
m_dmrDstIdWhitelistSlot2(),
m_dmrLookupFile(),
m_dmrTXHang(4U),
m_fusionEnabled(true),
m_dstarNetworkEnabled(true),
m_dstarGatewayAddress(),
m_dstarGatewayPort(0U),
m_dstarLocalPort(0U),
m_dstarNetworkDebug(false),
m_dmrNetworkEnabled(true),
m_dmrNetworkAddress(),
m_dmrNetworkPort(0U),
m_dmrNetworkLocal(0U),
m_dmrNetworkPassword(),
m_dmrNetworkDebug(false),
m_dmrNetworkSlot1(true),
m_dmrNetworkSlot2(true),
m_fusionNetworkEnabled(false),
m_fusionNetworkAddress(),
m_fusionNetworkPort(0U),
m_fusionNetworkDebug(false),
m_tftSerialPort("/dev/ttyAMA0"),
m_tftSerialBrightness(50U),
m_hd44780Rows(2U),
m_hd44780Columns(16U),
m_hd44780Pins(),
m_hd44780PWM(false),
m_hd44780PWMPin(),
m_hd44780PWMBright(),
m_hd44780PWMDim(),
m_hd44780DisplayClock(false),
m_hd44780UTC(false),
m_nextionPort("/dev/ttyAMA0"),
m_nextionBrightness(50U),
m_nextionDisplayClock(false),
m_nextionUTC(false),
m_oledType(3),
m_oledBrightness(0),
m_oledInvert(0)
{
}

CConf::~CConf()
{
}

bool CConf::read()
{
  FILE* fp = ::fopen(m_file.c_str(), "rt");
  if (fp == NULL) {
    ::fprintf(stderr, "Couldn't open the .ini file - %s\n", m_file.c_str());
    return false;
  }

  SECTION section = SECTION_NONE;

  char buffer[BUFFER_SIZE];
  while (::fgets(buffer, BUFFER_SIZE, fp) != NULL) {
    if (buffer[0U] == '#')
      continue;

    if (buffer[0U] == '[') {
      if (::strncmp(buffer, "[General]", 9U) == 0)
        section = SECTION_GENERAL;
	  else if (::strncmp(buffer, "[Info]", 6U) == 0)
		  section = SECTION_INFO;
	  else if (::strncmp(buffer, "[Log]", 5U) == 0)
		  section = SECTION_LOG;
	  else if (::strncmp(buffer, "[CW Id]", 7U) == 0)
		  section = SECTION_CWID;
	  else if (::strncmp(buffer, "[Modem]", 7U) == 0)
        section = SECTION_MODEM;
	  else if (::strncmp(buffer, "[D-Star]", 8U) == 0)
		  section = SECTION_DSTAR;
	  else if (::strncmp(buffer, "[DMR]", 5U) == 0)
		  section = SECTION_DMR;
	  else if (::strncmp(buffer, "[System Fusion]", 15U) == 0)
		  section = SECTION_FUSION;
	  else if (::strncmp(buffer, "[D-Star Network]", 16U) == 0)
        section = SECTION_DSTAR_NETWORK;
      else if (::strncmp(buffer, "[DMR Network]", 13U) == 0)
        section = SECTION_DMR_NETWORK;
      else if (::strncmp(buffer, "[System Fusion Network]", 23U) == 0)
        section = SECTION_FUSION_NETWORK;
      else if (::strncmp(buffer, "[TFT Serial]", 12U) == 0)
        section = SECTION_TFTSERIAL;
	  else if (::strncmp(buffer, "[HD44780]", 9U) == 0)
		  section = SECTION_HD44780;
	  else if (::strncmp(buffer, "[Nextion]", 9U) == 0)
		  section = SECTION_NEXTION;
	  else if (::strncmp(buffer, "[OLED]", 6U) == 0)
		  section = SECTION_OLED;
	  else
        section = SECTION_NONE;

      continue;
    }

    char* key   = ::strtok(buffer, " \t=\r\n");
    if (key == NULL)
      continue;

    char* value = ::strtok(NULL, "\r\n");
	if (section == SECTION_GENERAL) {
		if (::strcmp(key, "Callsign") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_callsign = value;
		} else if (::strcmp(key, "Timeout") == 0)
			m_timeout = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Duplex") == 0)
			m_duplex = ::atoi(value) == 1;
		else if (::strcmp(key, "ModeHang") == 0)
			m_modeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Display") == 0)
			m_display = value;
		else if (::strcmp(key, "Daemon") == 0)
			m_daemon = ::atoi(value) == 1;
	} else if (section == SECTION_INFO) {
		if (::strcmp(key, "TXFrequency") == 0)
			m_txFrequency = (unsigned int)::atoi(value);
		else if (::strcmp(key, "RXFrequency") == 0)
			m_rxFrequency = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Power") == 0)
			m_power = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Latitude") == 0)
			m_latitude = float(::atof(value));
		else if (::strcmp(key, "Longitude") == 0)
			m_longitude = float(::atof(value));
		else if (::strcmp(key, "Height") == 0)
			m_height = ::atoi(value);
		else if (::strcmp(key, "Location") == 0)
			m_location = value;
		else if (::strcmp(key, "Description") == 0)
			m_description = value;
		else if (::strcmp(key, "URL") == 0)
			m_url = value;
	} else if (section == SECTION_LOG) {
		if (::strcmp(key, "FilePath") == 0)
			m_logFilePath = value;
		else if (::strcmp(key, "FileRoot") == 0)
			m_logFileRoot = value;
		else if (::strcmp(key, "FileLevel") == 0)
			m_logFileLevel = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DisplayLevel") == 0)
			m_logDisplayLevel = (unsigned int)::atoi(value);
	} else if (section == SECTION_CWID) {
		if (::strcmp(key, "Enable") == 0)
			m_cwIdEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Time") == 0)
			m_cwIdTime = (unsigned int)::atoi(value);
	} else if (section == SECTION_MODEM) {
		if (::strcmp(key, "Port") == 0)
			m_modemPort = value;
		else if (::strcmp(key, "RXInvert") == 0)
			m_modemRXInvert = ::atoi(value) == 1;
		else if (::strcmp(key, "TXInvert") == 0)
			m_modemTXInvert = ::atoi(value) == 1;
		else if (::strcmp(key, "PTTInvert") == 0)
			m_modemPTTInvert = ::atoi(value) == 1;
		else if (::strcmp(key, "TXDelay") == 0)
			m_modemTXDelay = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DMRDelay") == 0)
			m_modemDMRDelay = (unsigned int)::atoi(value);
		else if (::strcmp(key, "RXLevel") == 0)
			m_modemRXLevel = (unsigned int)::atoi(value);
		else if (::strcmp(key, "TXLevel") == 0)
			m_modemTXLevel = (unsigned int)::atoi(value);
		else if (::strcmp(key, "OscOffset") == 0)
			m_modemOscOffset = ::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_modemDebug = ::atoi(value) == 1;
	} else if (section == SECTION_DSTAR) {
		if (::strcmp(key, "Enable") == 0)
			m_dstarEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Module") == 0) {
			// Convert the module to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_dstarModule = value;
		} else if (::strcmp(key, "SelfOnly") == 0)
			m_dstarSelfOnly = ::atoi(value) == 1;
		else if (::strcmp(key, "BlackList") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				if (::strlen(p) > 0U) {
					for (unsigned int i = 0U; p[i] != 0; i++)
						p[i] = ::toupper(p[i]);
					std::string callsign = std::string(p);
					callsign.resize(DSTAR_LONG_CALLSIGN_LENGTH, ' ');
					m_dstarBlackList.push_back(callsign);
				}
				p = ::strtok(NULL, ",\r\n");
			}
		}
	} else if (section == SECTION_DMR) {
		if (::strcmp(key, "Enable") == 0)
			m_dmrEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Beacons") == 0)
			m_dmrBeacons = ::atoi(value) == 1;
		else if (::strcmp(key, "Id") == 0)
			m_dmrId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ColorCode") == 0)
			m_dmrColorCode = (unsigned int)::atoi(value);
		else if (::strcmp(key, "SelfOnly") == 0)
			m_dmrSelfOnly = ::atoi(value) == 1;
		else if (::strcmp(key, "Prefixes") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int prefix = (unsigned int)::atoi(p);
				if (prefix > 0U && prefix <= 999U)
					m_dmrPrefixes.push_back(prefix);
				p = ::strtok(NULL, ",\r\n");
			}
		} else if (::strcmp(key, "BlackList") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrBlackList.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}
		}  else if (::strcmp(key, "DstIdBlackListSlot1") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrDstIdBlacklistSlot1.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}	
		} else if (::strcmp(key, "DstIdBlackListSlot2") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrDstIdBlacklistSlot2.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}
		} else if (::strcmp(key, "DstIdWhiteListSlot1") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrDstIdWhitelistSlot1.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}
		} else if (::strcmp(key, "DstIdWhiteListSlot2") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrDstIdWhitelistSlot2.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}
		} else if (::strcmp(key, "LookupFile") == 0)
			m_dmrLookupFile = value;
		else if (::strcmp(key, "TXHang") == 0)
			m_dmrTXHang = (unsigned int)::atoi(value);
	} else if (section == SECTION_FUSION) {
		if (::strcmp(key, "Enable") == 0)
			m_fusionEnabled = ::atoi(value) == 1;
	} else if (section == SECTION_DSTAR_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_dstarNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "GatewayAddress") == 0)
			m_dstarGatewayAddress = value;
		else if (::strcmp(key, "GatewayPort") == 0)
			m_dstarGatewayPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalPort") == 0)
			m_dstarLocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_dstarNetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_DMR_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_dmrNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Address") == 0)
			m_dmrNetworkAddress = value;
		else if (::strcmp(key, "Port") == 0)
			m_dmrNetworkPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Local") == 0)
			m_dmrNetworkLocal = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Password") == 0)
			m_dmrNetworkPassword = value;
		else if (::strcmp(key, "Debug") == 0)
			m_dmrNetworkDebug = ::atoi(value) == 1;
		else if (::strcmp(key, "Slot1") == 0)
			m_dmrNetworkSlot1 = ::atoi(value) == 1;
		else if (::strcmp(key, "Slot2") == 0)
			m_dmrNetworkSlot2 = ::atoi(value) == 1;
	} else if (section == SECTION_FUSION_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_fusionNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Address") == 0)
			m_fusionNetworkAddress = value;
		else if (::strcmp(key, "Port") == 0)
			m_fusionNetworkPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_fusionNetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_TFTSERIAL) {
		if (::strcmp(key, "Port") == 0)
			m_tftSerialPort = value;
		else if (::strcmp(key, "Brightness") == 0)
			m_tftSerialBrightness = (unsigned int)::atoi(value);
	} else if (section == SECTION_HD44780) {
		if (::strcmp(key, "Rows") == 0)
			m_hd44780Rows = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Columns") == 0)
			m_hd44780Columns = (unsigned int)::atoi(value);
		else if (::strcmp(key, "PWM") == 0)
			m_hd44780PWM = ::atoi(value) == 1;
		else if (::strcmp(key, "PWMPin") == 0)
			m_hd44780PWMPin = (unsigned int)::atoi(value);
		else if (::strcmp(key, "PWMBright") == 0)
			m_hd44780PWMBright = (unsigned int)::atoi(value);
		else if (::strcmp(key, "PWMDim") == 0)
			m_hd44780PWMDim = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DisplayClock") == 0)
			m_hd44780DisplayClock = ::atoi(value) == 1;
		else if (::strcmp(key, "UTC") == 0)
			m_hd44780UTC = ::atoi(value) == 1;
		else if (::strcmp(key, "Pins") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int pin = (unsigned int)::atoi(p);
				m_hd44780Pins.push_back(pin);
				p = ::strtok(NULL, ",\r\n");
			}
		}
	} else if (section == SECTION_NEXTION) {
		if (::strcmp(key, "Port") == 0)
			m_nextionPort = value;
		else if (::strcmp(key, "Brightness") == 0)
			m_nextionBrightness = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DisplayClock") == 0)
			m_nextionDisplayClock = ::atoi(value) == 1;
		else if (::strcmp(key, "UTC") == 0)
			m_nextionUTC = ::atoi(value) == 1;
	} else if (section == SECTION_OLED) {
		if (::strcmp(key, "Type") == 0)
			m_oledType = (unsigned char)::atoi(value);
		else if (::strcmp(key, "Port") == 0)
			m_oledBrightness = (unsigned char)::atoi(value);
		else if (::strcmp(key, "Brightness") == 0)
			m_oledInvert = (unsigned char)::atoi(value);
	}

  }

  ::fclose(fp);

  return true;
}

std::string CConf::getCallsign() const
{
  return m_callsign;
}

unsigned int CConf::getTimeout() const
{
  return m_timeout;
}

bool CConf::getDuplex() const
{
  return m_duplex;
}

unsigned int CConf::getModeHang() const
{
  return m_modeHang;
}

std::string CConf::getDisplay() const
{
  return m_display;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

unsigned int CConf::getRxFrequency() const
{
	return m_rxFrequency;
}

unsigned int CConf::getTxFrequency() const
{
	return m_txFrequency;
}

unsigned int CConf::getPower() const
{
	return m_power;
}

float CConf::getLatitude() const
{
	return m_latitude;
}

float CConf::getLongitude() const
{
	return m_longitude;
}

int CConf::getHeight() const
{
	return m_height;
}

std::string CConf::getLocation() const
{
	return m_location;
}

std::string CConf::getDescription() const
{
	return m_description;
}

std::string CConf::getURL() const
{
	return m_url;
}

unsigned int CConf::getLogDisplayLevel() const
{
	return m_logDisplayLevel;
}

unsigned int CConf::getLogFileLevel() const
{
	return m_logFileLevel;
}

std::string CConf::getLogFilePath() const
{
  return m_logFilePath;
}

std::string CConf::getLogFileRoot() const
{
  return m_logFileRoot;
}

bool CConf::getCWIdEnabled() const
{
	return m_cwIdEnabled;
}

unsigned int CConf::getCWIdTime() const
{
	return m_cwIdTime;
}

std::string CConf::getModemPort() const
{
	return m_modemPort;
}

bool CConf::getModemRXInvert() const
{
	return m_modemRXInvert;
}

bool CConf::getModemTXInvert() const
{
	return m_modemTXInvert;
}

bool CConf::getModemPTTInvert() const
{
	return m_modemPTTInvert;
}

unsigned int CConf::getModemTXDelay() const
{
	return m_modemTXDelay;
}

unsigned int CConf::getModemDMRDelay() const
{
	return m_modemDMRDelay;
}

unsigned int CConf::getModemRXLevel() const
{
	return m_modemRXLevel;
}

unsigned int CConf::getModemTXLevel() const
{
	return m_modemTXLevel;
}

int CConf::getModemOscOffset() const
{
	return m_modemOscOffset;
}

bool CConf::getModemDebug() const
{
	return m_modemDebug;
}

bool CConf::getDStarEnabled() const
{
	return m_dstarEnabled;
}

std::string CConf::getDStarModule() const
{
	return m_dstarModule;
}

bool CConf::getDStarSelfOnly() const
{
	return m_dstarSelfOnly;
}

std::vector<std::string> CConf::getDStarBlackList() const
{
	return m_dstarBlackList;
}

bool CConf::getDMREnabled() const
{
	return m_dmrEnabled;
}

bool CConf::getDMRBeacons() const
{
	return m_dmrBeacons;
}

unsigned int CConf::getDMRId() const
{
	return m_dmrId;
}

unsigned int CConf::getDMRColorCode() const
{
	return m_dmrColorCode;
}

bool CConf::getDMRSelfOnly() const
{
	return m_dmrSelfOnly;
}

std::vector<unsigned int> CConf::getDMRPrefixes() const
{
	return m_dmrPrefixes;
}

std::vector<unsigned int> CConf::getDMRBlackList() const
{
	return m_dmrBlackList;
}
std::vector<unsigned int> CConf::getDMRDstIdBlacklistSlot1() const
{
	return m_dmrDstIdBlacklistSlot1;
}
std::vector<unsigned int> CConf::getDMRDstIdBlacklistSlot2() const
{
	return m_dmrDstIdBlacklistSlot2;
}
std::vector<unsigned int> CConf::getDMRDstIdWhitelistSlot1() const
{
	return m_dmrDstIdWhitelistSlot1;
}std::vector<unsigned int> CConf::getDMRDstIdWhitelistSlot2() const
{
	return m_dmrDstIdWhitelistSlot2;
}
std::string CConf::getDMRLookupFile() const
{
	return m_dmrLookupFile;
}

unsigned int CConf::getDMRTXHang() const
{
	return m_dmrTXHang;
}

bool CConf::getFusionEnabled() const
{
	return m_fusionEnabled;
}

bool CConf::getDStarNetworkEnabled() const
{
	return m_dstarNetworkEnabled;
}

std::string CConf::getDStarGatewayAddress() const
{
  return m_dstarGatewayAddress;
}

unsigned int CConf::getDStarGatewayPort() const
{
  return m_dstarGatewayPort;
}

unsigned int CConf::getDStarLocalPort() const
{
  return m_dstarLocalPort;
}

bool CConf::getDStarNetworkDebug() const
{
	return m_dstarNetworkDebug;
}

bool CConf::getDMRNetworkEnabled() const
{
	return m_dmrNetworkEnabled;
}

std::string CConf::getDMRNetworkAddress() const
{
  return m_dmrNetworkAddress;
}

unsigned int CConf::getDMRNetworkPort() const
{
  return m_dmrNetworkPort;
}

unsigned int CConf::getDMRNetworkLocal() const
{
	return m_dmrNetworkLocal;
}

std::string CConf::getDMRNetworkPassword() const
{
  return m_dmrNetworkPassword;
}

bool CConf::getDMRNetworkDebug() const
{
	return m_dmrNetworkDebug;
}

bool CConf::getDMRNetworkSlot1() const
{
	return m_dmrNetworkSlot1;
}

bool CConf::getDMRNetworkSlot2() const
{
	return m_dmrNetworkSlot2;
}

bool CConf::getFusionNetworkEnabled() const
{
	return m_fusionNetworkEnabled;
}

std::string CConf::getFusionNetworkAddress() const
{
  return m_fusionNetworkAddress;
}

unsigned int CConf::getFusionNetworkPort() const
{
  return m_fusionNetworkPort;
}

bool CConf::getFusionNetworkDebug() const
{
	return m_fusionNetworkDebug;
}

std::string CConf::getTFTSerialPort() const
{
  return m_tftSerialPort;
}

unsigned int CConf::getTFTSerialBrightness() const
{
	return m_tftSerialBrightness;
}

unsigned int CConf::getHD44780Rows() const
{
	return m_hd44780Rows;
}

unsigned int CConf::getHD44780Columns() const
{
	return m_hd44780Columns;
}

std::vector<unsigned int> CConf::getHD44780Pins() const
{
	return m_hd44780Pins;
}

bool CConf::getHD44780PWM() const
{
	return m_hd44780PWM;
}

unsigned int CConf::getHD44780PWMPin() const
{
	return m_hd44780PWMPin;
}

unsigned int CConf::getHD44780PWMBright() const
{
	return m_hd44780PWMBright;
}

unsigned int CConf::getHD44780PWMDim() const
{
	return m_hd44780PWMDim;
}

bool CConf::getHD44780DisplayClock() const
{
	return m_hd44780DisplayClock;
}

bool CConf::getHD44780UTC() const
{
	return m_hd44780UTC;
}

std::string CConf::getNextionPort() const
{
	return m_nextionPort;
}

unsigned int CConf::getNextionBrightness() const
{
	return m_nextionBrightness;
}

bool CConf::getNextionDisplayClock() const
{
	return m_nextionDisplayClock;
}

bool CConf::getNextionUTC() const
{
	return m_nextionUTC;
}

unsigned char CConf::getOLEDType() const
{
	return m_oledType;
}

unsigned char CConf::getOLEDBrightness() const
{
	return m_oledBrightness;
}

unsigned char CConf::getOLEDInvert() const
{
	return m_oledInvert;
}
