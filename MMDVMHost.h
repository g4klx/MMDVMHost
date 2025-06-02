/*
 *   Copyright (C) 2015-2021 by Jonathan Naylor G4KLX
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

#if !defined(MMDVMHOST_H)
#define	MMDVMHOST_H

#include "RemoteControl.h"
#include "POCSAGNetwork.h"
#include "POCSAGControl.h"
#include "DStarNetwork.h"
#include "AX25Network.h"
#include "NXDNNetwork.h"
#include "DStarControl.h"
#include "AX25Control.h"
#include "DMRControl.h"
#include "YSFControl.h"
#include "P25Control.h"
#include "NXDNControl.h"
#include "M17Control.h"
#include "NXDNLookup.h"
#include "YSFNetwork.h"
#include "P25Network.h"
#include "DMRNetwork.h"
#include "M17Network.h"
#include "FMNetwork.h"
#include "DMRLookup.h"
#include "FMControl.h"
#include "Display.h"
#include "Timer.h"
#include "Modem.h"
#include "Conf.h"

#include <string>


class CMMDVMHost
{
public:
  CMMDVMHost(const std::string& confFile);
  ~CMMDVMHost();

  int run();

  void buildNetworkStatusString(std::string &str);
  void buildNetworkHostsString(std::string &str);

private:
  CConf           m_conf;
  CModem*         m_modem;
  CDStarControl*  m_dstar;
  CDMRControl*    m_dmr;
  CYSFControl*    m_ysf;
  CP25Control*    m_p25;
  CNXDNControl*   m_nxdn;
  CM17Control*    m_m17;
  CPOCSAGControl* m_pocsag;
  CFMControl*     m_fm;
  CAX25Control*   m_ax25;
  CDStarNetwork*  m_dstarNetwork;
  IDMRNetwork*    m_dmrNetwork;
  CYSFNetwork*    m_ysfNetwork;
  CP25Network*    m_p25Network;
  INXDNNetwork*   m_nxdnNetwork;
  CM17Network*    m_m17Network;
  CPOCSAGNetwork* m_pocsagNetwork;
  CFMNetwork*     m_fmNetwork;
  CAX25Network*   m_ax25Network;
  CDisplay*       m_display;
  unsigned char   m_mode;
  unsigned int    m_dstarRFModeHang;
  unsigned int    m_dmrRFModeHang;
  unsigned int    m_ysfRFModeHang;
  unsigned int    m_p25RFModeHang;
  unsigned int    m_nxdnRFModeHang;
  unsigned int    m_m17RFModeHang;
  unsigned int    m_fmRFModeHang;
  unsigned int    m_dstarNetModeHang;
  unsigned int    m_dmrNetModeHang;
  unsigned int    m_ysfNetModeHang;
  unsigned int    m_p25NetModeHang;
  unsigned int    m_nxdnNetModeHang;
  unsigned int    m_m17NetModeHang;
  unsigned int    m_pocsagNetModeHang;
  unsigned int    m_fmNetModeHang;
  CTimer          m_modeTimer;
  CTimer          m_dmrTXTimer;
  CTimer          m_cwIdTimer;
  bool            m_duplex;
  unsigned int    m_timeout;
  bool            m_dstarEnabled;
  bool            m_dmrEnabled;
  bool            m_ysfEnabled;
  bool            m_p25Enabled;
  bool            m_nxdnEnabled;
  bool            m_m17Enabled;
  bool            m_pocsagEnabled;
  bool            m_fmEnabled;
  bool            m_ax25Enabled;
  unsigned int    m_cwIdTime;
  CDMRLookup*     m_dmrLookup;
  CNXDNLookup*    m_nxdnLookup;
  std::string     m_callsign;
  unsigned int    m_id;
  std::string     m_cwCallsign;
  bool            m_lockFileEnabled;
  std::string     m_lockFileName;
  CRemoteControl* m_remoteControl;
  bool            m_fixedMode;

  void readParams();
  bool createModem();
  bool createDStarNetwork();
  bool createDMRNetwork();
  bool createYSFNetwork();
  bool createP25Network();
  bool createNXDNNetwork();
  bool createM17Network();
  bool createPOCSAGNetwork();
  bool createFMNetwork();
  bool createAX25Network();

  void remoteControl();
  void processModeCommand(unsigned char mode, unsigned int timeout);

  void setMode(unsigned char mode);
  void enableModemMode(bool& mode, bool enabled);
  void processEnableModeCommand(unsigned char mode, bool hasController, bool& modeEnabled, bool enableMode, bool isAX25 = false);

  void createLockFile(const char* mode) const;
  void removeLockFile() const;
};

#endif
