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

#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

const int BUFFER_SIZE = 500;

enum SECTION {
  SECTION_NONE,
  SECTION_GENERAL,
  SECTION_INFO,
  SECTION_LOG,
  SECTION_MODEM,
  SECTION_DSTAR,
  SECTION_DMR,
  SECTION_FUSION,
  SECTION_DSTAR_NETWORK,
  SECTION_DMR_NETWORK,
  SECTION_FUSION_NETWORK,
  SECTION_TFTSERIAL
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_timeout(120U),
m_duplex(true),
m_modeHang(10U),
m_display(),
m_rxFrequency(0U),
m_txFrequency(0U),
m_power(0U),
m_latitude(0.0F),
m_longitude(0.0F),
m_height(0),
m_location(),
m_description(),
m_url(),
m_logLevel(0U),
m_logPath(),
m_logRoot(),
m_logDisplay(true),
m_modemPort(),
m_modemRXInvert(false),
m_modemTXInvert(false),
m_modemPTTInvert(false),
m_modemTXDelay(100U),
m_modemRXLevel(100U),
m_modemTXLevel(100U),
m_modemDebug(false),
m_dstarEnabled(true),
m_dstarModule("C"),
m_dmrEnabled(true),
m_dmrId(0U),
m_dmrColorCode(2U),
m_fusionEnabled(true),
m_dstarNetworkEnabled(true),
m_dstarGatewayAddress(),
m_dstarGatewayPort(0U),
m_dstarLocalPort(0U),
m_dstarNetworkDebug(false),
m_dmrNetworkEnabled(true),
m_dmrNetworkAddress(),
m_dmrNetworkPort(0U),
m_dmrNetworkPassword(),
m_dmrNetworkDebug(false),
m_fusionNetworkEnabled(false),
m_fusionNetworkAddress(),
m_fusionNetworkPort(0U),
m_fusionNetworkDebug(false),
m_tftSerialPort()
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
      else if (::strncmp(buffer, "[TFT Serial]", 11U) == 0)
        section = SECTION_TFTSERIAL;
      else
        section = SECTION_NONE;

      continue;
    }

    char* key   = ::strtok(buffer, " \t=\r\n");
    if (key == NULL)
      continue;

    char* value = ::strtok(NULL, "\r\n");
	if (section == SECTION_GENERAL) {
		if (::strcmp(key, "Callsign") == 0)
			m_callsign = value;
		else if (::strcmp(key, "Timeout") == 0)
			m_timeout = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Duplex") == 0)
			m_duplex = ::atoi(value) == 1;
		else if (::strcmp(key, "ModeHang") == 0)
			m_modeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Display") == 0)
			m_display = value;
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
		if (::strcmp(key, "Path") == 0)
			m_logPath = value;
		else if (::strcmp(key, "Root") == 0)
			m_logRoot = value;
		else if (::strcmp(key, "Level") == 0)
			m_logLevel = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Display") == 0)
			m_logDisplay = ::atoi(value) == 1;
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
		else if (::strcmp(key, "RXLevel") == 0)
			m_modemRXLevel = (unsigned int)::atoi(value);
		else if (::strcmp(key, "TXLevel") == 0)
			m_modemTXLevel = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_modemDebug = ::atoi(value) == 1;
	} else if (section == SECTION_DSTAR) {
		if (::strcmp(key, "Enabled") == 0)
			m_dstarEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Module") == 0)
			m_dstarModule = value;
	} else if (section == SECTION_DMR) {
		if (::strcmp(key, "Enabled") == 0)
			m_dmrEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Id") == 0)
			m_dmrId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ColorCode") == 0)
			m_dmrColorCode = (unsigned int)::atoi(value);
	} else if (section == SECTION_FUSION) {
		if (::strcmp(key, "Enabled") == 0)
			m_fusionEnabled = ::atoi(value) == 1;
	} else if (section == SECTION_DSTAR_NETWORK) {
		if (::strcmp(key, "Enabled") == 0)
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
		if (::strcmp(key, "Enabled") == 0)
			m_dmrNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Address") == 0)
			m_dmrNetworkAddress = value;
		else if (::strcmp(key, "Port") == 0)
			m_dmrNetworkPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Password") == 0)
			m_dmrNetworkPassword = value;
		else if (::strcmp(key, "Debug") == 0)
			m_dmrNetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_FUSION_NETWORK) {
		if (::strcmp(key, "Enabled") == 0)
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

unsigned int CConf::getLogLevel() const
{
	return m_logLevel;
}

std::string CConf::getLogPath() const
{
  return m_logPath;
}

std::string CConf::getLogRoot() const
{
  return m_logRoot;
}

bool CConf::getLogDisplay() const
{
  return m_logDisplay;
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

unsigned int CConf::getModemRXLevel() const
{
	return m_modemRXLevel;
}

unsigned int CConf::getModemTXLevel() const
{
	return m_modemTXLevel;
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

bool CConf::getDMREnabled() const
{
	return m_dmrEnabled;
}

unsigned int CConf::getDMRId() const
{
	return m_dmrId;
}

unsigned int CConf::getDMRColorCode() const
{
	return m_dmrColorCode;
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

std::string CConf::getDMRNetworkPassword() const
{
  return m_dmrNetworkPassword;
}

bool CConf::getDMRNetworkDebug() const
{
	return m_dmrNetworkDebug;
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
