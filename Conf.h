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

#if !defined(CONF_H)
#define	CONF_H

#include <string>
#include <vector>

class CConf
{
public:
  CConf(const std::string& file);
  ~CConf();

  bool read();

  // The General section
  std::string  getCallsign() const;
  unsigned int getTimeout() const;
  bool         getDuplex() const;
  unsigned int getModeHang() const;
  std::string  getDisplay() const;
  std::string	 getNagiosStatusFile() const;

  // The Info section
  unsigned int getRxFrequency() const;
  unsigned int getTxFrequency() const;
  unsigned int getPower() const;
  float        getLatitude() const;
  float        getLongitude() const;
  int          getHeight() const;
  std::string  getLocation() const;
  std::string  getDescription() const;
  std::string  getURL() const;

  // The Log section
  unsigned int getLogDisplayLevel() const;
  unsigned int getLogFileLevel() const;
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;

  // The Modem section
  std::string  getModemPort() const;
  bool         getModemRXInvert() const;
  bool         getModemTXInvert() const;
  bool         getModemPTTInvert() const;
  unsigned int getModemTXDelay() const;
  unsigned int getModemDMRDelay() const;
  unsigned int getModemRXLevel() const;
  unsigned int getModemTXLevel() const;
  int          getModemOscOffset() const;
  bool         getModemDebug() const;

  // The D-Star section
  bool         getDStarEnabled() const;
  std::string  getDStarModule() const;
  bool         getDStarSelfOnly() const;
  std::vector<std::string> getDStarBlackList() const;

  // The DMR section
  bool         getDMREnabled() const;
  bool         getDMRBeacons() const;
  unsigned int getDMRId() const;
  unsigned int getDMRColorCode() const;
  bool         getDMRSelfOnly() const;
  std::vector<unsigned int> getDMRPrefixes() const;
  std::vector<unsigned int> getDMRBlackList() const;
  std::string  getDMRLookupFile() const;

  // The System Fusion section
  bool         getFusionEnabled() const;
  bool         getFusionParrotEnabled() const;

  // The D-Star Network section
  bool         getDStarNetworkEnabled() const;
  std::string  getDStarGatewayAddress() const;
  unsigned int getDStarGatewayPort() const;
  unsigned int getDStarLocalPort() const;
  bool         getDStarNetworkDebug() const;

  // The DMR Network section
  bool         getDMRNetworkEnabled() const;
  std::string  getDMRNetworkAddress() const;
  unsigned int getDMRNetworkPort() const;
  unsigned int getDMRNetworkLocal() const;
  std::string  getDMRNetworkPassword() const;
  bool         getDMRNetworkDebug() const;
  bool         getDMRNetworkSlot1() const;
  bool         getDMRNetworkSlot2() const;

  // The System Fusion Network section
  bool         getFusionNetworkEnabled() const;
  std::string  getFusionNetworkAddress() const;
  unsigned int getFusionNetworkPort() const;
  bool         getFusionNetworkDebug() const;

  // The TFTSERIAL section
  std::string  getTFTSerialPort() const;
  unsigned int getTFTSerialBrightness() const;

  // The HD44780 section
  unsigned int getHD44780Rows() const;
  unsigned int getHD44780Columns() const;
  std::vector<unsigned int> getHD44780Pins() const;

  // The Nextion section
  std::string  getNextionPort() const;
  unsigned int getNextionBrightness() const;

private:
  std::string  m_file;
  std::string  m_callsign;
  unsigned int m_timeout;
  bool         m_duplex;
  unsigned int m_modeHang;
  std::string  m_display;
  std::string  m_NagiosStatusFile;

  unsigned int m_rxFrequency;
  unsigned int m_txFrequency;
  unsigned int m_power;
  float        m_latitude;
  float        m_longitude;
  int          m_height;
  std::string  m_location;
  std::string  m_description;
  std::string  m_url;

  unsigned int m_logDisplayLevel;
  unsigned int m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;

  std::string  m_modemPort;
  bool         m_modemRXInvert;
  bool         m_modemTXInvert;
  bool         m_modemPTTInvert;
  unsigned int m_modemTXDelay;
  unsigned int m_modemDMRDelay;
  unsigned int m_modemRXLevel;
  unsigned int m_modemTXLevel;
  int          m_modemOscOffset;
  bool         m_modemDebug;

  bool         m_dstarEnabled;
  std::string  m_dstarModule;
  bool         m_dstarSelfOnly;
  std::vector<std::string> m_dstarBlackList;

  bool         m_dmrEnabled;
  bool         m_dmrBeacons;
  unsigned int m_dmrId;
  unsigned int m_dmrColorCode;
  bool         m_dmrSelfOnly;
  std::vector<unsigned int> m_dmrPrefixes;
  std::vector<unsigned int> m_dmrBlackList;
  std::string  m_dmrLookupFile;

  bool         m_fusionEnabled;
  bool         m_fusionParrotEnabled;

  bool         m_dstarNetworkEnabled;
  std::string  m_dstarGatewayAddress;
  unsigned int m_dstarGatewayPort;
  unsigned int m_dstarLocalPort;
  bool         m_dstarNetworkDebug;

  bool         m_dmrNetworkEnabled;
  std::string  m_dmrNetworkAddress;
  unsigned int m_dmrNetworkPort;
  unsigned int m_dmrNetworkLocal;
  std::string  m_dmrNetworkPassword;
  bool         m_dmrNetworkDebug;
  bool         m_dmrNetworkSlot1;
  bool         m_dmrNetworkSlot2;

  bool         m_fusionNetworkEnabled;
  std::string  m_fusionNetworkAddress;
  unsigned int m_fusionNetworkPort;
  bool         m_fusionNetworkDebug;

  std::string  m_tftSerialPort;
  unsigned int m_tftSerialBrightness;

  unsigned int m_hd44780Rows;
  unsigned int m_hd44780Columns;
  std::vector<unsigned int> m_hd44780Pins;

  std::string  m_nextionPort;
  unsigned int m_nextionBrightness;
};

#endif
