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
  unsigned int getId() const;
  unsigned int getTimeout() const;
  bool         getDuplex() const;
  std::string  getDisplay() const;
  bool         getDaemon() const;

  // The Info section
  unsigned int getRXFrequency() const;
  unsigned int getTXFrequency() const;
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

  // The CW ID section
  bool         getCWIdEnabled() const;
  unsigned int getCWIdTime() const;
  std::string  getCWIdCallsign() const;

  // The DMR Id section
  std::string  getDMRIdLookupFile() const;
  unsigned int getDMRIdLookupTime() const;

  // The NXDN Id section
  std::string  getNXDNIdLookupFile() const;
  unsigned int getNXDNIdLookupTime() const;

  // The Modem section
  std::string  getModemPort() const;
  std::string  getModemProtocol() const;
  unsigned int getModemAddress() const;
  bool         getModemRXInvert() const;
  bool         getModemTXInvert() const;
  bool         getModemPTTInvert() const;
  unsigned int getModemTXDelay() const;
  unsigned int getModemDMRDelay() const;
  int          getModemTXOffset() const;
  int          getModemRXOffset() const;
  int          getModemRXDCOffset() const;
  int          getModemTXDCOffset() const;
  float        getModemRFLevel() const;
  float        getModemRXLevel() const;
  float        getModemCWIdTXLevel() const;
  float        getModemDStarTXLevel() const;
  float        getModemDMRTXLevel() const;
  float        getModemYSFTXLevel() const;
  float        getModemP25TXLevel() const;
  float        getModemNXDNTXLevel() const;
  float        getModemPOCSAGTXLevel() const;
  std::string  getModemRSSIMappingFile() const;
  bool         getModemTrace() const;
  bool         getModemDebug() const;

  // The Transparent Data section
  bool         getTransparentEnabled() const;
  std::string  getTransparentRemoteAddress() const;
  unsigned int getTransparentRemotePort() const;
  unsigned int getTransparentLocalPort() const;

  // The UMP section
  bool         getUMPEnabled() const;
  std::string  getUMPPort() const;

  // The D-Star section
  bool         getDStarEnabled() const;
  std::string  getDStarModule() const;
  bool         getDStarSelfOnly() const;
  std::vector<std::string> getDStarBlackList() const;
  bool         getDStarAckReply() const;
  unsigned int getDStarAckTime() const;
  bool         getDStarErrorReply() const;
  bool         getDStarRemoteGateway() const;
  unsigned int getDStarModeHang() const;

  // The DMR section
  bool         getDMREnabled() const;
  bool         getDMRBeacons() const;
  unsigned int getDMRBeaconInterval() const;
  unsigned int getDMRBeaconDuration() const;
  unsigned int getDMRId() const;
  unsigned int getDMRColorCode() const;
  bool         getDMREmbeddedLCOnly() const;
  bool         getDMRDumpTAData() const;
  bool         getDMRSelfOnly() const;
  std::vector<unsigned int> getDMRPrefixes() const;
  std::vector<unsigned int> getDMRBlackList() const;
  std::vector<unsigned int> getDMRWhiteList() const;
  std::vector<unsigned int> getDMRSlot1TGWhiteList() const;
  std::vector<unsigned int> getDMRSlot2TGWhiteList() const;
  unsigned int getDMRCallHang() const;
  unsigned int getDMRTXHang() const;
  unsigned int getDMRModeHang() const;

  // The System Fusion section
  bool          getFusionEnabled() const;
  bool          getFusionLowDeviation() const;
  bool          getFusionRemoteGateway() const;
  bool          getFusionSelfOnly() const;
  unsigned int  getFusionTXHang() const;
  bool          getFusionSQLEnabled() const;
  unsigned char getFusionSQL() const;
  unsigned int  getFusionModeHang() const;

  // The P25 section
  bool         getP25Enabled() const;
  unsigned int getP25Id() const;
  unsigned int getP25NAC() const;
  bool         getP25SelfOnly() const;
  bool         getP25OverrideUID() const;
  bool         getP25RemoteGateway() const;
  unsigned int getP25ModeHang() const;

  // The NXDN section
  bool         getNXDNEnabled() const;
  unsigned int getNXDNId() const;
  unsigned int getNXDNRAN() const;
  bool         getNXDNSelfOnly() const;
  bool         getNXDNRemoteGateway() const;
  unsigned int getNXDNModeHang() const;

  // The POCSAG section
  bool         getPOCSAGEnabled() const;
  unsigned int getPOCSAGFrequency() const;

  // The D-Star Network section
  bool         getDStarNetworkEnabled() const;
  std::string  getDStarGatewayAddress() const;
  unsigned int getDStarGatewayPort() const;
  unsigned int getDStarLocalPort() const;
  unsigned int getDStarNetworkModeHang() const;
  bool         getDStarNetworkDebug() const;

  // The DMR Network section
  bool         getDMRNetworkEnabled() const;
  std::string  getDMRNetworkAddress() const;
  unsigned int getDMRNetworkPort() const;
  unsigned int getDMRNetworkLocal() const;
  std::string  getDMRNetworkPassword() const;
  std::string  getDMRNetworkOptions() const;
  bool         getDMRNetworkDebug() const;
  unsigned int getDMRNetworkJitter() const;
  bool         getDMRNetworkSlot1() const;
  bool         getDMRNetworkSlot2() const;
  unsigned int getDMRNetworkModeHang() const;

  // The System Fusion Network section
  bool         getFusionNetworkEnabled() const;
  std::string  getFusionNetworkMyAddress() const;
  unsigned int getFusionNetworkMyPort() const;
  std::string  getFusionNetworkGatewayAddress() const;
  unsigned int getFusionNetworkGatewayPort() const;
  unsigned int getFusionNetworkModeHang() const;
  bool         getFusionNetworkDebug() const;

  // The P25 Network section
  bool         getP25NetworkEnabled() const;
  std::string  getP25GatewayAddress() const;
  unsigned int getP25GatewayPort() const;
  unsigned int getP25LocalPort() const;
  unsigned int getP25NetworkModeHang() const;
  bool         getP25NetworkDebug() const;

  // The NXDN Network section
  bool         getNXDNNetworkEnabled() const;
  std::string  getNXDNGatewayAddress() const;
  unsigned int getNXDNGatewayPort() const;
  std::string  getNXDNLocalAddress() const;
  unsigned int getNXDNLocalPort() const;
  unsigned int getNXDNNetworkModeHang() const;
  bool         getNXDNNetworkDebug() const;

  // The POCSAG Network section
  bool         getPOCSAGNetworkEnabled() const;
  std::string  getPOCSAGGatewayAddress() const;
  unsigned int getPOCSAGGatewayPort() const;
  std::string  getPOCSAGLocalAddress() const;
  unsigned int getPOCSAGLocalPort() const;
  unsigned int getPOCSAGNetworkModeHang() const;
  bool         getPOCSAGNetworkDebug() const;

  // The TFTSERIAL section
  std::string  getTFTSerialPort() const;
  unsigned int getTFTSerialBrightness() const;

  // The HD44780 section
  unsigned int getHD44780Rows() const;
  unsigned int getHD44780Columns() const;
  std::vector<unsigned int> getHD44780Pins() const;
  unsigned int getHD44780i2cAddress() const;
  bool         getHD44780PWM() const;
  unsigned int getHD44780PWMPin() const;
  unsigned int getHD44780PWMBright() const;
  unsigned int getHD44780PWMDim() const;
  bool         getHD44780DisplayClock() const;
  bool         getHD44780UTC() const;

  // The Nextion section
  std::string  getNextionPort() const;
  unsigned int getNextionBrightness() const;
  bool         getNextionDisplayClock() const;
  bool         getNextionUTC() const;
  unsigned int getNextionIdleBrightness() const;
  unsigned int getNextionScreenLayout() const;

  // The OLED section
  unsigned char  getOLEDType() const;
  unsigned char  getOLEDBrightness() const;
  bool           getOLEDInvert() const;
  bool           getOLEDScroll() const;

  // The LCDproc section
  std::string  getLCDprocAddress() const;
  unsigned int getLCDprocPort() const;
  unsigned int getLCDprocLocalPort() const;
  bool         getLCDprocDisplayClock() const;
  bool         getLCDprocUTC() const;
  bool         getLCDprocDimOnIdle() const;

private:
  std::string  m_file;
  std::string  m_callsign;
  unsigned int m_id;
  unsigned int m_timeout;
  bool         m_duplex;
  std::string  m_display;
  bool         m_daemon;

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

  bool         m_cwIdEnabled;
  unsigned int m_cwIdTime;
  std::string  m_cwIdCallsign;

  std::string  m_dmrIdLookupFile;
  unsigned int m_dmrIdLookupTime;

  std::string  m_nxdnIdLookupFile;
  unsigned int m_nxdnIdLookupTime;

  std::string  m_modemPort;
  std::string  m_modemProtocol;
  unsigned int m_modemAddress;
  bool         m_modemRXInvert;
  bool         m_modemTXInvert;
  bool         m_modemPTTInvert;
  unsigned int m_modemTXDelay;
  unsigned int m_modemDMRDelay;
  int          m_modemTXOffset;
  int          m_modemRXOffset;
  int          m_modemRXDCOffset;
  int          m_modemTXDCOffset;
  float        m_modemRFLevel;
  float        m_modemRXLevel;
  float        m_modemCWIdTXLevel;
  float        m_modemDStarTXLevel;
  float        m_modemDMRTXLevel;
  float        m_modemYSFTXLevel;
  float        m_modemP25TXLevel;
  float        m_modemNXDNTXLevel;
  float        m_modemPOCSAGTXLevel;
  std::string  m_modemRSSIMappingFile;
  bool         m_modemTrace;
  bool         m_modemDebug;

  bool         m_transparentEnabled;
  std::string  m_transparentRemoteAddress;
  unsigned int m_transparentRemotePort;
  unsigned int m_transparentLocalPort;

  bool         m_umpEnabled;
  std::string  m_umpPort;

  bool         m_dstarEnabled;
  std::string  m_dstarModule;
  bool         m_dstarSelfOnly;
  std::vector<std::string> m_dstarBlackList;
  bool         m_dstarAckReply;
  unsigned int m_dstarAckTime;
  bool         m_dstarErrorReply;
  bool         m_dstarRemoteGateway;
  unsigned int m_dstarModeHang;

  bool         m_dmrEnabled;
  bool         m_dmrBeacons;
  unsigned int m_dmrBeaconInterval;
  unsigned int m_dmrBeaconDuration;
  unsigned int m_dmrId;
  unsigned int m_dmrColorCode;
  bool         m_dmrSelfOnly;
  bool         m_dmrEmbeddedLCOnly;
  bool         m_dmrDumpTAData;
  std::vector<unsigned int> m_dmrPrefixes;
  std::vector<unsigned int> m_dmrBlackList;
  std::vector<unsigned int> m_dmrWhiteList;
  std::vector<unsigned int> m_dmrSlot1TGWhiteList;
  std::vector<unsigned int> m_dmrSlot2TGWhiteList;
  unsigned int m_dmrCallHang;
  unsigned int m_dmrTXHang;
  unsigned int m_dmrModeHang;

  bool          m_fusionEnabled;
  bool          m_fusionLowDeviation;
  bool          m_fusionRemoteGateway;
  bool          m_fusionSelfOnly;
  unsigned int  m_fusionTXHang;
  bool          m_fusionSQLEnabled;
  unsigned char m_fusionSQL;
  unsigned int  m_fusionModeHang;

  bool         m_p25Enabled;
  unsigned int m_p25Id;
  unsigned int m_p25NAC;
  bool         m_p25SelfOnly;
  bool         m_p25OverrideUID;
  bool         m_p25RemoteGateway;
  unsigned int m_p25ModeHang;

  bool         m_nxdnEnabled;
  unsigned int m_nxdnId;
  unsigned int m_nxdnRAN;
  bool         m_nxdnSelfOnly;
  bool         m_nxdnRemoteGateway;
  unsigned int m_nxdnModeHang;

  bool         m_pocsagEnabled;
  unsigned int m_pocsagFrequency;

  bool         m_dstarNetworkEnabled;
  std::string  m_dstarGatewayAddress;
  unsigned int m_dstarGatewayPort;
  unsigned int m_dstarLocalPort;
  unsigned int m_dstarNetworkModeHang;
  bool         m_dstarNetworkDebug;

  bool         m_dmrNetworkEnabled;
  std::string  m_dmrNetworkAddress;
  unsigned int m_dmrNetworkPort;
  unsigned int m_dmrNetworkLocal;
  std::string  m_dmrNetworkPassword;
  std::string  m_dmrNetworkOptions;
  bool         m_dmrNetworkDebug;
  unsigned int m_dmrNetworkJitter;
  bool         m_dmrNetworkSlot1;
  bool         m_dmrNetworkSlot2;
  unsigned int m_dmrNetworkModeHang;

  bool         m_fusionNetworkEnabled;
  std::string  m_fusionNetworkMyAddress;
  unsigned int m_fusionNetworkMyPort;
  std::string  m_fusionNetworkGatewayAddress;
  unsigned int m_fusionNetworkGatewayPort;
  unsigned int m_fusionNetworkModeHang;
  bool         m_fusionNetworkDebug;

  bool         m_p25NetworkEnabled;
  std::string  m_p25GatewayAddress;
  unsigned int m_p25GatewayPort;
  unsigned int m_p25LocalPort;
  unsigned int m_p25NetworkModeHang;
  bool         m_p25NetworkDebug;

  bool         m_nxdnNetworkEnabled;
  std::string  m_nxdnGatewayAddress;
  unsigned int m_nxdnGatewayPort;
  std::string  m_nxdnLocalAddress;
  unsigned int m_nxdnLocalPort;
  unsigned int m_nxdnNetworkModeHang;
  bool         m_nxdnNetworkDebug;

  bool         m_pocsagNetworkEnabled;
  std::string  m_pocsagGatewayAddress;
  unsigned int m_pocsagGatewayPort;
  std::string  m_pocsagLocalAddress;
  unsigned int m_pocsagLocalPort;
  unsigned int m_pocsagNetworkModeHang;
  bool         m_pocsagNetworkDebug;

  std::string  m_tftSerialPort;
  unsigned int m_tftSerialBrightness;

  unsigned int m_hd44780Rows;
  unsigned int m_hd44780Columns;
  std::vector<unsigned int> m_hd44780Pins;
  unsigned int m_hd44780i2cAddress;
  bool         m_hd44780PWM;
  unsigned int m_hd44780PWMPin;
  unsigned int m_hd44780PWMBright;
  unsigned int m_hd44780PWMDim;
  bool         m_hd44780DisplayClock;
  bool         m_hd44780UTC;

  std::string  m_nextionPort;
  unsigned int m_nextionBrightness;
  bool         m_nextionDisplayClock;
  bool         m_nextionUTC;
  unsigned int m_nextionIdleBrightness;
  unsigned int m_nextionScreenLayout;

  unsigned char m_oledType;
  unsigned char m_oledBrightness;
  bool          m_oledInvert;
  bool          m_oledScroll;

  std::string  m_lcdprocAddress;
  unsigned int m_lcdprocPort;
  unsigned int m_lcdprocLocalPort;
  bool         m_lcdprocDisplayClock;
  bool         m_lcdprocUTC;
  bool         m_lcdprocDimOnIdle;
};

#endif
