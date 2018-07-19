/*
 *   Copyright (C) 2015,2016,2017,2018 by Jonathan Naylor G4KLX
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
  SECTION_DMRID_LOOKUP,
  SECTION_NXDNID_LOOKUP,
  SECTION_MODEM,
  SECTION_TRANSPARENT,
  SECTION_UMP,
  SECTION_DSTAR,
  SECTION_DMR,
  SECTION_FUSION,
  SECTION_P25,
  SECTION_NXDN,
  SECTION_POCSAG,
  SECTION_DSTAR_NETWORK,
  SECTION_DMR_NETWORK,
  SECTION_FUSION_NETWORK,
  SECTION_P25_NETWORK,
  SECTION_NXDN_NETWORK,
  SECTION_POCSAG_NETWORK,
  SECTION_TFTSERIAL,
  SECTION_HD44780,
  SECTION_NEXTION,
  SECTION_OLED,
  SECTION_LCDPROC,
  SECTION_TGREWRITE
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_id(0U),
m_timeout(120U),
m_duplex(true),
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
m_cwIdCallsign(),
m_dmrIdLookupFile(),
m_dmrIdLookupTime(0U),
m_nxdnIdLookupFile(),
m_nxdnIdLookupTime(0U),
m_modemPort(),
m_modemProtocol("uart"),
m_modemAddress(0x22),
m_modemRXInvert(false),
m_modemTXInvert(false),
m_modemPTTInvert(false),
m_modemTXDelay(100U),
m_modemDMRDelay(0U),
m_modemTXOffset(0),
m_modemRXOffset(0),
m_modemRXDCOffset(0),
m_modemTXDCOffset(0),
m_modemRFLevel(100.0F),
m_modemRXLevel(50.0F),
m_modemCWIdTXLevel(50.0F),
m_modemDStarTXLevel(50.0F),
m_modemDMRTXLevel(50.0F),
m_modemYSFTXLevel(50.0F),
m_modemP25TXLevel(50.0F),
m_modemNXDNTXLevel(50.0F),
m_modemPOCSAGTXLevel(50.0F),
m_modemRSSIMappingFile(),
m_modemTrace(false),
m_modemDebug(false),
m_transparentEnabled(false),
m_transparentRemoteAddress(),
m_transparentRemotePort(0U),
m_transparentLocalPort(0U),
m_umpEnabled(false),
m_umpPort(),
m_dstarEnabled(false),
m_dstarModule("C"),
m_dstarSelfOnly(false),
m_dstarBlackList(),
m_dstarAckReply(true),
m_dstarAckTime(750U),
m_dstarErrorReply(true),
m_dstarRemoteGateway(false),
m_dstarModeHang(10U),
m_dmrEnabled(false),
m_dmrBeacons(false),
m_dmrBeaconInterval(60U),
m_dmrBeaconDuration(3U),
m_dmrId(0U),
m_dmrColorCode(2U),
m_dmrSelfOnly(false),
m_dmrEmbeddedLCOnly(false),
m_dmrDumpTAData(true),
m_dmrPrefixes(),
m_dmrBlackList(),
m_dmrWhiteList(),
m_dmrSlot1TGWhiteList(),
m_dmrSlot2TGWhiteList(),
m_dmrCallHang(10U),
m_dmrTXHang(4U),
m_dmrModeHang(10U),
m_fusionEnabled(false),
m_fusionLowDeviation(false),
m_fusionRemoteGateway(false),
m_fusionSelfOnly(false),
m_fusionTXHang(4U),
m_fusionSQLEnabled(false),
m_fusionSQL(0U),
m_fusionModeHang(10U),
m_p25Enabled(false),
m_p25Id(0U),
m_p25NAC(0x293U),
m_p25SelfOnly(false),
m_p25OverrideUID(false),
m_p25RemoteGateway(false),
m_p25ModeHang(10U),
m_nxdnEnabled(false),
m_nxdnId(0U),
m_nxdnRAN(1U),
m_nxdnSelfOnly(false),
m_nxdnRemoteGateway(false),
m_nxdnModeHang(10U),
m_pocsagEnabled(false),
m_pocsagFrequency(0U),
m_dstarNetworkEnabled(false),
m_dstarGatewayAddress(),
m_dstarGatewayPort(0U),
m_dstarLocalPort(0U),
m_dstarNetworkModeHang(3U),
m_dstarNetworkDebug(false),
m_dmrNetworkEnabled(false),
m_dmrNetworkAddress(),
m_dmrNetworkPort(0U),
m_dmrNetworkLocal(0U),
m_dmrNetworkPassword(),
m_dmrNetworkOptions(),
m_dmrNetworkDebug(false),
m_dmrNetworkJitter(360U),
m_dmrNetworkSlot1(true),
m_dmrNetworkSlot2(true),
m_dmrNetworkModeHang(3U),
m_fusionNetworkEnabled(false),
m_fusionNetworkMyAddress(),
m_fusionNetworkMyPort(0U),
m_fusionNetworkGatewayAddress(),
m_fusionNetworkGatewayPort(0U),
m_fusionNetworkModeHang(3U),
m_fusionNetworkDebug(false),
m_p25NetworkEnabled(false),
m_p25GatewayAddress(),
m_p25GatewayPort(0U),
m_p25LocalPort(0U),
m_p25NetworkModeHang(3U),
m_p25NetworkDebug(false),
m_nxdnNetworkEnabled(false),
m_nxdnGatewayAddress(),
m_nxdnGatewayPort(0U),
m_nxdnLocalAddress(),
m_nxdnLocalPort(0U),
m_nxdnNetworkModeHang(3U),
m_nxdnNetworkDebug(false),
m_pocsagNetworkEnabled(false),
m_pocsagGatewayAddress(),
m_pocsagGatewayPort(0U),
m_pocsagLocalAddress(),
m_pocsagLocalPort(0U),
m_pocsagNetworkModeHang(3U),
m_pocsagNetworkDebug(false),
m_tftSerialPort("/dev/ttyAMA0"),
m_tftSerialBrightness(50U),
m_hd44780Rows(2U),
m_hd44780Columns(16U),
m_hd44780Pins(),
m_hd44780i2cAddress(),
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
m_nextionIdleBrightness(20U),
m_nextionScreenLayout(0U),
m_oledType(3U),
m_oledBrightness(0U),
m_oledInvert(false),
m_oledScroll(false),
m_lcdprocAddress(),
m_lcdprocPort(0U),
m_lcdprocLocalPort(0U),
m_lcdprocDisplayClock(false),
m_lcdprocUTC(false)
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
	  else if (::strncmp(buffer, "[DMR Id Lookup]", 15U) == 0)
		  section = SECTION_DMRID_LOOKUP;
	  else if (::strncmp(buffer, "[NXDN Id Lookup]", 16U) == 0)
		  section = SECTION_NXDNID_LOOKUP;
	  else if (::strncmp(buffer, "[Modem]", 7U) == 0)
		  section = SECTION_MODEM;
	  else if (::strncmp(buffer, "[Transparent Data]", 18U) == 0)
		  section = SECTION_TRANSPARENT;
	  else if (::strncmp(buffer, "[UMP]", 5U) == 0)
		  section = SECTION_UMP;
	  else if (::strncmp(buffer, "[D-Star]", 8U) == 0)
		  section = SECTION_DSTAR;
	  else if (::strncmp(buffer, "[DMR]", 5U) == 0)
		  section = SECTION_DMR;
	  else if (::strncmp(buffer, "[System Fusion]", 15U) == 0)
		  section = SECTION_FUSION;
	  else if (::strncmp(buffer, "[P25]", 5U) == 0)
		  section = SECTION_P25;
	  else if (::strncmp(buffer, "[NXDN]", 6U) == 0)
		  section = SECTION_NXDN;
	  else if (::strncmp(buffer, "[POCSAG]", 8U) == 0)
		  section = SECTION_POCSAG;
	  else if (::strncmp(buffer, "[D-Star Network]", 16U) == 0)
		  section = SECTION_DSTAR_NETWORK;
	  else if (::strncmp(buffer, "[DMR Network]", 13U) == 0)
		  section = SECTION_DMR_NETWORK;
	  else if (::strncmp(buffer, "[System Fusion Network]", 23U) == 0)
		  section = SECTION_FUSION_NETWORK;
	  else if (::strncmp(buffer, "[P25 Network]", 13U) == 0)
		  section = SECTION_P25_NETWORK;
	  else if (::strncmp(buffer, "[NXDN Network]", 14U) == 0)
		  section = SECTION_NXDN_NETWORK;
	  else if (::strncmp(buffer, "[POCSAG Network]", 16U) == 0)
		  section = SECTION_POCSAG_NETWORK;
	  else if (::strncmp(buffer, "[TFT Serial]", 12U) == 0)
		  section = SECTION_TFTSERIAL;
	  else if (::strncmp(buffer, "[HD44780]", 9U) == 0)
		  section = SECTION_HD44780;
	  else if (::strncmp(buffer, "[Nextion]", 9U) == 0)
		  section = SECTION_NEXTION;
	  else if (::strncmp(buffer, "[OLED]", 6U) == 0)
		  section = SECTION_OLED;
	  else if (::strncmp(buffer, "[LCDproc]", 9U) == 0)
		  section = SECTION_LCDPROC;
	  else
		  section = SECTION_NONE;

	  continue;
  }

  char* key   = ::strtok(buffer, " \t=\r\n");
  if (key == NULL)
    continue;

  char* value = ::strtok(NULL, "\r\n");
  if (value == NULL)
    continue;

  // Remove quotes from the value
  size_t len = ::strlen(value);
  if (len > 1U && *value == '"' && value[len - 1U] == '"') {
	  value[len - 1U] = '\0';
	  value++;
  }

  if (section == SECTION_GENERAL) {
		if (::strcmp(key, "Callsign") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_cwIdCallsign = m_callsign = value;
		} else if (::strcmp(key, "Id") == 0)
			m_id = m_p25Id = m_dmrId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Timeout") == 0)
			m_timeout = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Duplex") == 0)
			m_duplex = ::atoi(value) == 1;
		else if (::strcmp(key, "ModeHang") == 0)
			m_dstarNetworkModeHang = m_dmrNetworkModeHang = m_fusionNetworkModeHang = m_p25NetworkModeHang =
			m_dstarModeHang        = m_dmrModeHang        = m_fusionModeHang        = m_p25ModeHang        = (unsigned int)::atoi(value);
		else if (::strcmp(key, "RFModeHang") == 0)
			m_dstarModeHang = m_dmrModeHang = m_fusionModeHang = m_p25ModeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "NetModeHang") == 0)
			m_dstarNetworkModeHang = m_dmrNetworkModeHang = m_fusionNetworkModeHang = m_p25NetworkModeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Display") == 0)
			m_display = value;
		else if (::strcmp(key, "Daemon") == 0)
			m_daemon = ::atoi(value) == 1;
	} else if (section == SECTION_INFO) {
		if (::strcmp(key, "TXFrequency") == 0)
			m_pocsagFrequency = m_txFrequency = (unsigned int)::atoi(value);
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
		else if (::strcmp(key, "Callsign") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_cwIdCallsign = value;
		}
	} else if (section == SECTION_DMRID_LOOKUP) {
		if (::strcmp(key, "File") == 0)
			m_dmrIdLookupFile = value;
		else if (::strcmp(key, "Time") == 0)
			m_dmrIdLookupTime = (unsigned int)::atoi(value);
	} else if (section == SECTION_NXDNID_LOOKUP) {
		if (::strcmp(key, "File") == 0)
			m_nxdnIdLookupFile = value;
		else if (::strcmp(key, "Time") == 0)
			m_nxdnIdLookupTime = (unsigned int)::atoi(value);
	} else if (section == SECTION_MODEM) {
		if (::strcmp(key, "Port") == 0)
			m_modemPort = value;
        else if (::strcmp(key, "Protocol") == 0)
            m_modemProtocol = value;
        else if (::strcmp(key, "Address") == 0)
            m_modemAddress = (unsigned int)::strtoul(value, NULL, 16);
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
		else if (::strcmp(key, "RXOffset") == 0)
			m_modemRXOffset = ::atoi(value);
		else if (::strcmp(key, "TXOffset") == 0)
			m_modemTXOffset = ::atoi(value);
		else if (::strcmp(key, "RXDCOffset") == 0)
			m_modemRXDCOffset = ::atoi(value);
		else if (::strcmp(key, "TXDCOffset") == 0)
			m_modemTXDCOffset = ::atoi(value);
		else if (::strcmp(key, "RFLevel") == 0)
			m_modemRFLevel = float(::atof(value));
		else if (::strcmp(key, "RXLevel") == 0)
			m_modemRXLevel = float(::atof(value));
		else if (::strcmp(key, "TXLevel") == 0)
			m_modemCWIdTXLevel = m_modemDStarTXLevel = m_modemDMRTXLevel = m_modemYSFTXLevel = m_modemP25TXLevel = m_modemNXDNTXLevel = float(::atof(value));
		else if (::strcmp(key, "CWIdTXLevel") == 0)
			m_modemCWIdTXLevel = float(::atof(value));
		else if (::strcmp(key, "D-StarTXLevel") == 0)
			m_modemDStarTXLevel = float(::atof(value));
		else if (::strcmp(key, "DMRTXLevel") == 0)
			m_modemDMRTXLevel = float(::atof(value));
		else if (::strcmp(key, "YSFTXLevel") == 0)
			m_modemYSFTXLevel = float(::atof(value));
		else if (::strcmp(key, "P25TXLevel") == 0)
			m_modemP25TXLevel = float(::atof(value));
		else if (::strcmp(key, "NXDNTXLevel") == 0)
			m_modemNXDNTXLevel = float(::atof(value));
		else if (::strcmp(key, "POCSAGTXLevel") == 0)
			m_modemPOCSAGTXLevel = float(::atof(value));
		else if (::strcmp(key, "RSSIMappingFile") == 0)
			m_modemRSSIMappingFile = value;
		else if (::strcmp(key, "Trace") == 0)
			m_modemTrace = ::atoi(value) == 1;
		else if (::strcmp(key, "Debug") == 0)
			m_modemDebug = ::atoi(value) == 1;
	} else if (section == SECTION_TRANSPARENT) {
		if (::strcmp(key, "Enable") == 0)
			m_transparentEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "RemoteAddress") == 0)
			m_transparentRemoteAddress = value;
		else if (::strcmp(key, "RemotePort") == 0)
			m_transparentRemotePort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalPort") == 0)
			m_transparentLocalPort = (unsigned int)::atoi(value);
	} else if (section == SECTION_UMP) {
		if (::strcmp(key, "Enable") == 0)
			m_umpEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Port") == 0)
			m_umpPort = value;
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
		} else if (::strcmp(key, "AckReply") == 0)
			m_dstarAckReply = ::atoi(value) == 1;
		else if (::strcmp(key, "AckTime") == 0)
			m_dstarAckTime = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ErrorReply") == 0)
			m_dstarErrorReply = ::atoi(value) == 1;
		else if (::strcmp(key, "RemoteGateway") == 0)
			m_dstarRemoteGateway = ::atoi(value) == 1;
		else if (::strcmp(key, "ModeHang") == 0)
			m_dstarModeHang = (unsigned int)::atoi(value);
	} else if (section == SECTION_DMR) {
		if (::strcmp(key, "Enable") == 0)
			m_dmrEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Beacons") == 0)
			m_dmrBeacons = ::atoi(value) == 1;
		else if (::strcmp(key, "BeaconInterval") == 0)
			m_dmrBeaconInterval = (unsigned int)::atoi(value);
		else if (::strcmp(key, "BeaconDuration") == 0)
			m_dmrBeaconDuration = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Id") == 0)
			m_dmrId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ColorCode") == 0)
			m_dmrColorCode = (unsigned int)::atoi(value);
		else if (::strcmp(key, "SelfOnly") == 0)
			m_dmrSelfOnly = ::atoi(value) == 1;
		else if (::strcmp(key, "EmbeddedLCOnly") == 0)
			m_dmrEmbeddedLCOnly = ::atoi(value) == 1;
		else if (::strcmp(key, "DumpTAData") == 0)
			m_dmrDumpTAData = ::atoi(value) == 1;
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
		} else if (::strcmp(key, "WhiteList") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrWhiteList.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}
		} else if (::strcmp(key, "Slot1TGWhiteList") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrSlot1TGWhiteList.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}
		} else if (::strcmp(key, "Slot2TGWhiteList") == 0) {
			char* p = ::strtok(value, ",\r\n");
			while (p != NULL) {
				unsigned int id = (unsigned int)::atoi(p);
				if (id > 0U)
					m_dmrSlot2TGWhiteList.push_back(id);
				p = ::strtok(NULL, ",\r\n");
			}
		} else if (::strcmp(key, "TXHang") == 0)
			m_dmrTXHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "CallHang") == 0)
			m_dmrCallHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ModeHang") == 0)
			m_dmrModeHang = (unsigned int)::atoi(value);
	} else if (section == SECTION_FUSION) {
		if (::strcmp(key, "Enable") == 0)
			m_fusionEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "LowDeviation") == 0)
			m_fusionLowDeviation = ::atoi(value) == 1;
		else if (::strcmp(key, "DSQ") == 0 || ::strcmp(key, "DGID") == 0) {
			m_fusionSQLEnabled = true;
			m_fusionSQL        = (unsigned int)::atoi(value);
		} else if (::strcmp(key, "RemoteGateway") == 0)
			m_fusionRemoteGateway = ::atoi(value) == 1;
		else if (::strcmp(key, "SelfOnly") == 0)
			m_fusionSelfOnly = ::atoi(value) == 1;
		else if (::strcmp(key, "TXHang") == 0)
			m_fusionTXHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ModeHang") == 0)
			m_fusionModeHang = (unsigned int)::atoi(value);
	} else if (section == SECTION_P25) {
		if (::strcmp(key, "Enable") == 0)
			m_p25Enabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Id") == 0)
			m_p25Id = (unsigned int)::atoi(value);
		else if (::strcmp(key, "NAC") == 0)
			m_p25NAC = (unsigned int)::strtoul(value, NULL, 16);
		else if (::strcmp(key, "OverrideUIDCheck") == 0)
			m_p25OverrideUID = ::atoi(value) == 1;
		else if (::strcmp(key, "SelfOnly") == 0)
			m_p25SelfOnly = ::atoi(value) == 1;
		else if (::strcmp(key, "RemoteGateway") == 0)
			m_p25RemoteGateway = ::atoi(value) == 1;
		else if (::strcmp(key, "ModeHang") == 0)
			m_p25ModeHang = (unsigned int)::atoi(value);
	} else if (section == SECTION_NXDN) {
		if (::strcmp(key, "Enable") == 0)
			m_nxdnEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Id") == 0)
			m_nxdnId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "RAN") == 0)
			m_nxdnRAN = (unsigned int)::atoi(value);
		else if (::strcmp(key, "SelfOnly") == 0)
			m_nxdnSelfOnly = ::atoi(value) == 1;
		else if (::strcmp(key, "RemoteGateway") == 0)
			m_nxdnRemoteGateway = ::atoi(value) == 1;
		else if (::strcmp(key, "ModeHang") == 0)
			m_nxdnModeHang = (unsigned int)::atoi(value);
	} else if (section == SECTION_POCSAG) {
		if (::strcmp(key, "Enable") == 0)
			m_pocsagEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Frequency") == 0)
			m_pocsagFrequency = (unsigned int)::atoi(value);
	} else if (section == SECTION_DSTAR_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_dstarNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "GatewayAddress") == 0)
			m_dstarGatewayAddress = value;
		else if (::strcmp(key, "GatewayPort") == 0)
			m_dstarGatewayPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalPort") == 0)
			m_dstarLocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ModeHang") == 0)
			m_dstarNetworkModeHang = (unsigned int)::atoi(value);
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
		else if (::strcmp(key, "Options") == 0)
			m_dmrNetworkOptions = value;
		else if (::strcmp(key, "Debug") == 0)
			m_dmrNetworkDebug = ::atoi(value) == 1;
		else if (::strcmp(key, "Jitter") == 0)
			m_dmrNetworkJitter = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Slot1") == 0)
			m_dmrNetworkSlot1 = ::atoi(value) == 1;
		else if (::strcmp(key, "Slot2") == 0)
			m_dmrNetworkSlot2 = ::atoi(value) == 1;
		else if (::strcmp(key, "ModeHang") == 0)
			m_dmrNetworkModeHang = (unsigned int)::atoi(value);
	} else if (section == SECTION_FUSION_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_fusionNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "LocalAddress") == 0)
			m_fusionNetworkMyAddress = value;
		else if (::strcmp(key, "LocalPort") == 0)
			m_fusionNetworkMyPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "GatewayAddress") == 0)
			m_fusionNetworkGatewayAddress = value;
		else if (::strcmp(key, "GatewayPort") == 0)
			m_fusionNetworkGatewayPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ModeHang") == 0)
			m_fusionNetworkModeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_fusionNetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_P25_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_p25NetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "GatewayAddress") == 0)
			m_p25GatewayAddress = value;
		else if (::strcmp(key, "GatewayPort") == 0)
			m_p25GatewayPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalPort") == 0)
			m_p25LocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ModeHang") == 0)
			m_p25NetworkModeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_p25NetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_NXDN_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_nxdnNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "LocalAddress") == 0)
			m_nxdnLocalAddress = value;
		else if (::strcmp(key, "LocalPort") == 0)
			m_nxdnLocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "GatewayAddress") == 0)
			m_nxdnGatewayAddress = value;
		else if (::strcmp(key, "GatewayPort") == 0)
			m_nxdnGatewayPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ModeHang") == 0)
			m_nxdnNetworkModeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_nxdnNetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_POCSAG_NETWORK) {
		if (::strcmp(key, "Enable") == 0)
			m_pocsagNetworkEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "LocalAddress") == 0)
			m_pocsagLocalAddress = value;
		else if (::strcmp(key, "LocalPort") == 0)
			m_pocsagLocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "GatewayAddress") == 0)
			m_pocsagGatewayAddress = value;
		else if (::strcmp(key, "GatewayPort") == 0)
			m_pocsagGatewayPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ModeHang") == 0)
			m_pocsagNetworkModeHang = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Debug") == 0)
			m_pocsagNetworkDebug = ::atoi(value) == 1;
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
		else if (::strcmp(key, "I2CAddress") == 0)
			m_hd44780i2cAddress = (unsigned int)::strtoul(value, NULL, 16);
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
			m_nextionIdleBrightness = m_nextionBrightness = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DisplayClock") == 0)
			m_nextionDisplayClock = ::atoi(value) == 1;
		else if (::strcmp(key, "UTC") == 0)
			m_nextionUTC = ::atoi(value) == 1;
		else if (::strcmp(key, "IdleBrightness") == 0)
			m_nextionIdleBrightness = (unsigned int)::atoi(value);
		else if (::strcmp(key, "ScreenLayout") == 0)
			m_nextionScreenLayout = (unsigned int)::atoi(value);
	} else if (section == SECTION_OLED) {
		if (::strcmp(key, "Type") == 0)
			m_oledType = (unsigned char)::atoi(value);
		else if (::strcmp(key, "Brightness") == 0)
			m_oledBrightness = (unsigned char)::atoi(value);
		else if (::strcmp(key, "Invert") == 0)
			m_oledInvert = ::atoi(value) == 1;
		else if (::strcmp(key, "Scroll") == 0)
			m_oledScroll = ::atoi(value) == 1;
	} else if (section == SECTION_LCDPROC) {
		if (::strcmp(key, "Address") == 0)
			m_lcdprocAddress = value;
		else if (::strcmp(key, "Port") == 0)
			m_lcdprocPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalPort") == 0)
			m_lcdprocLocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DisplayClock") == 0)
			m_lcdprocDisplayClock = ::atoi(value) == 1;
		else if (::strcmp(key, "UTC") == 0)
			m_lcdprocUTC = ::atoi(value) == 1;
		else if (::strcmp(key, "DimOnIdle") == 0)
			m_lcdprocDimOnIdle = ::atoi(value) == 1;
	}
  }

  ::fclose(fp);

  return true;
}

std::string CConf::getCallsign() const
{
	return m_callsign;
}

unsigned int CConf::getId() const
{
	return m_id;
}

unsigned int CConf::getTimeout() const
{
	return m_timeout;
}

bool CConf::getDuplex() const
{
	return m_duplex;
}

std::string CConf::getDisplay() const
{
	return m_display;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

unsigned int CConf::getRXFrequency() const
{
	return m_rxFrequency;
}

unsigned int CConf::getTXFrequency() const
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

std::string CConf::getCWIdCallsign() const
{
	return m_cwIdCallsign;
}

std::string CConf::getDMRIdLookupFile() const
{
	return m_dmrIdLookupFile;
}

unsigned int CConf::getDMRIdLookupTime() const
{
	return m_dmrIdLookupTime;
}

std::string CConf::getNXDNIdLookupFile() const
{
	return m_nxdnIdLookupFile;
}

unsigned int CConf::getNXDNIdLookupTime() const
{
	return m_nxdnIdLookupTime;
}

std::string CConf::getModemPort() const
{
	return m_modemPort;
}

std::string CConf::getModemProtocol() const
{
	return m_modemProtocol;
}

unsigned int CConf::getModemAddress() const
{
	return m_modemAddress;
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

int CConf::getModemRXOffset() const
{
	return m_modemRXOffset;
}

int CConf::getModemTXOffset() const
{
	return m_modemTXOffset;
}

int CConf::getModemRXDCOffset() const
{
	return m_modemRXDCOffset;
}

int CConf::getModemTXDCOffset() const
{
	return m_modemTXDCOffset;
}

float CConf::getModemRFLevel() const
{
	return m_modemRFLevel;
}

float CConf::getModemRXLevel() const
{
	return m_modemRXLevel;
}

float CConf::getModemCWIdTXLevel() const
{
	return m_modemCWIdTXLevel;
}

float CConf::getModemDStarTXLevel() const
{
	return m_modemDStarTXLevel;
}

float CConf::getModemDMRTXLevel() const
{
	return m_modemDMRTXLevel;
}

float CConf::getModemYSFTXLevel() const
{
	return m_modemYSFTXLevel;
}

float CConf::getModemP25TXLevel() const
{
	return m_modemP25TXLevel;
}

float CConf::getModemNXDNTXLevel() const
{
	return m_modemNXDNTXLevel;
}

float CConf::getModemPOCSAGTXLevel() const
{
	return m_modemPOCSAGTXLevel;
}

std::string CConf::getModemRSSIMappingFile () const
{
	return m_modemRSSIMappingFile;
}

bool CConf::getModemTrace() const
{
	return m_modemTrace;
}

bool CConf::getModemDebug() const
{
	return m_modemDebug;
}

bool CConf::getTransparentEnabled() const
{
	return m_transparentEnabled;
}

std::string CConf::getTransparentRemoteAddress() const
{
	return m_transparentRemoteAddress;
}

unsigned int CConf::getTransparentRemotePort() const
{
	return m_transparentRemotePort;
}

unsigned int CConf::getTransparentLocalPort() const
{
	return m_transparentLocalPort;
}

bool CConf::getUMPEnabled() const
{
	return m_umpEnabled;
}

std::string CConf::getUMPPort() const
{
	return m_umpPort;
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

bool CConf::getDStarAckReply() const
{
	return m_dstarAckReply;
}

unsigned int CConf::getDStarAckTime() const
{
	return m_dstarAckTime;
}

bool CConf::getDStarErrorReply() const
{
	return m_dstarErrorReply;
}

bool CConf::getDStarRemoteGateway() const
{
	return m_dstarRemoteGateway;
}

unsigned int CConf::getDStarModeHang() const
{
	return m_dstarModeHang;
}

bool CConf::getDMREnabled() const
{
	return m_dmrEnabled;
}

bool CConf::getDMRBeacons() const
{
	return m_dmrBeacons;
}

unsigned int CConf::getDMRBeaconInterval() const
{
	return m_dmrBeaconInterval;
}

unsigned int CConf::getDMRBeaconDuration() const
{
	return m_dmrBeaconDuration;
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

bool CConf::getDMREmbeddedLCOnly() const
{
	return m_dmrEmbeddedLCOnly;
}

bool CConf::getDMRDumpTAData() const
{
	return m_dmrDumpTAData;
}

std::vector<unsigned int> CConf::getDMRPrefixes() const
{
	return m_dmrPrefixes;
}

std::vector<unsigned int> CConf::getDMRBlackList() const
{
	return m_dmrBlackList;
}

std::vector<unsigned int> CConf::getDMRWhiteList() const
{
	return m_dmrWhiteList;
}

std::vector<unsigned int> CConf::getDMRSlot1TGWhiteList() const
{
	return m_dmrSlot1TGWhiteList;
}

std::vector<unsigned int> CConf::getDMRSlot2TGWhiteList() const
{
	return m_dmrSlot2TGWhiteList;
}

unsigned int CConf::getDMRCallHang() const
{
	return m_dmrCallHang;
}

unsigned int CConf::getDMRTXHang() const
{
	return m_dmrTXHang;
}

unsigned int CConf::getDMRModeHang() const
{
	return m_dmrModeHang;
}

bool CConf::getFusionEnabled() const
{
	return m_fusionEnabled;
}

bool CConf::getFusionLowDeviation() const
{
	return m_fusionLowDeviation;
}

bool CConf::getFusionRemoteGateway() const
{
	return m_fusionRemoteGateway;
}

unsigned int CConf::getFusionTXHang() const
{
	return m_fusionTXHang;
}

bool CConf::getFusionSelfOnly() const
{
	return m_fusionSelfOnly;
}

bool CConf::getFusionSQLEnabled() const
{
	return m_fusionSQLEnabled;
}

unsigned char CConf::getFusionSQL() const
{
	return m_fusionSQL;
}

unsigned int CConf::getFusionModeHang() const
{
	return m_fusionModeHang;
}

bool CConf::getP25Enabled() const
{
	return m_p25Enabled;
}

unsigned int CConf::getP25Id() const
{
	return m_p25Id;
}

unsigned int CConf::getP25NAC() const
{
	return m_p25NAC;
}

bool CConf::getP25OverrideUID() const
{
	return m_p25OverrideUID;
}

bool CConf::getP25SelfOnly() const
{
	return m_p25SelfOnly;
}

bool CConf::getP25RemoteGateway() const
{
	return m_p25RemoteGateway;
}

unsigned int CConf::getP25ModeHang() const
{
	return m_p25ModeHang;
}

bool CConf::getNXDNEnabled() const
{
	return m_nxdnEnabled;
}

unsigned int CConf::getNXDNId() const
{
	return m_nxdnId;
}

unsigned int CConf::getNXDNRAN() const
{
	return m_nxdnRAN;
}

bool CConf::getNXDNSelfOnly() const
{
	return m_nxdnSelfOnly;
}

bool CConf::getNXDNRemoteGateway() const
{
	return m_nxdnRemoteGateway;
}

unsigned int CConf::getNXDNModeHang() const
{
	return m_nxdnModeHang;
}

bool CConf::getPOCSAGEnabled() const
{
	return m_pocsagEnabled;
}

unsigned int CConf::getPOCSAGFrequency() const
{
	return m_pocsagFrequency;
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

unsigned int CConf::getDStarNetworkModeHang() const
{
	return m_dstarNetworkModeHang;
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

std::string CConf::getDMRNetworkOptions() const
{
	return m_dmrNetworkOptions;
}

unsigned int CConf::getDMRNetworkModeHang() const
{
	return m_dmrNetworkModeHang;
}

bool CConf::getDMRNetworkDebug() const
{
	return m_dmrNetworkDebug;
}

unsigned int CConf::getDMRNetworkJitter() const
{
	return m_dmrNetworkJitter;
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

std::string CConf::getFusionNetworkMyAddress() const
{
	return m_fusionNetworkMyAddress;
}

unsigned int CConf::getFusionNetworkMyPort() const
{
	return m_fusionNetworkMyPort;
}

std::string CConf::getFusionNetworkGatewayAddress() const
{
	return m_fusionNetworkGatewayAddress;
}

unsigned int CConf::getFusionNetworkGatewayPort() const
{
	return m_fusionNetworkGatewayPort;
}

unsigned int CConf::getFusionNetworkModeHang() const
{
	return m_fusionNetworkModeHang;
}

bool CConf::getFusionNetworkDebug() const
{
	return m_fusionNetworkDebug;
}

bool CConf::getP25NetworkEnabled() const
{
	return m_p25NetworkEnabled;
}

std::string CConf::getP25GatewayAddress() const
{
	return m_p25GatewayAddress;
}

unsigned int CConf::getP25GatewayPort() const
{
	return m_p25GatewayPort;
}

unsigned int CConf::getP25LocalPort() const
{
	return m_p25LocalPort;
}

unsigned int CConf::getP25NetworkModeHang() const
{
	return m_p25NetworkModeHang;
}

bool CConf::getP25NetworkDebug() const
{
	return m_p25NetworkDebug;
}

bool CConf::getNXDNNetworkEnabled() const
{
	return m_nxdnNetworkEnabled;
}

std::string CConf::getNXDNGatewayAddress() const
{
	return m_nxdnGatewayAddress;
}

unsigned int CConf::getNXDNGatewayPort() const
{
	return m_nxdnGatewayPort;
}

std::string CConf::getNXDNLocalAddress() const
{
	return m_nxdnLocalAddress;
}

unsigned int CConf::getNXDNLocalPort() const
{
	return m_nxdnLocalPort;
}

unsigned int CConf::getNXDNNetworkModeHang() const
{
	return m_nxdnNetworkModeHang;
}

bool CConf::getNXDNNetworkDebug() const
{
	return m_nxdnNetworkDebug;
}

bool CConf::getPOCSAGNetworkEnabled() const
{
	return m_pocsagNetworkEnabled;
}

std::string CConf::getPOCSAGGatewayAddress() const
{
	return m_pocsagGatewayAddress;
}

unsigned int CConf::getPOCSAGGatewayPort() const
{
	return m_pocsagGatewayPort;
}

std::string CConf::getPOCSAGLocalAddress() const
{
	return m_pocsagLocalAddress;
}

unsigned int CConf::getPOCSAGLocalPort() const
{
	return m_pocsagLocalPort;
}

unsigned int CConf::getPOCSAGNetworkModeHang() const
{
	return m_pocsagNetworkModeHang;
}

bool CConf::getPOCSAGNetworkDebug() const
{
	return m_pocsagNetworkDebug;
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

unsigned int CConf::getHD44780i2cAddress() const
{
  return m_hd44780i2cAddress;
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

unsigned int CConf::getNextionIdleBrightness() const
{
	return m_nextionIdleBrightness;
}

unsigned int CConf::getNextionScreenLayout() const
{
	return m_nextionScreenLayout;
}

unsigned char CConf::getOLEDType() const
{
	return m_oledType;
}

unsigned char CConf::getOLEDBrightness() const
{
	return m_oledBrightness;
}

bool CConf::getOLEDInvert() const
{
	return m_oledInvert;
}

bool CConf::getOLEDScroll() const
{
	return m_oledScroll;
}

std::string CConf::getLCDprocAddress() const
{
	return m_lcdprocAddress;
}

unsigned int CConf::getLCDprocPort() const
{
	return m_lcdprocPort;
}

unsigned int CConf::getLCDprocLocalPort() const
{
	return m_lcdprocLocalPort;
}

bool CConf::getLCDprocDisplayClock() const
{
	return m_lcdprocDisplayClock;
}

bool CConf::getLCDprocUTC() const
{
	return m_lcdprocUTC;
}

bool CConf::getLCDprocDimOnIdle() const
{
	return m_lcdprocDimOnIdle;
}
