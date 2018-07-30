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

#if !defined(MMDVMHOST_H)
#define	MMDVMHOST_H

#include "POCSAGNetwork.h"
#include "DStarNetwork.h"
#include "NXDNNetwork.h"
#include "NXDNLookup.h"
#include "YSFNetwork.h"
#include "P25Network.h"
#include "DMRNetwork.h"
#include "DMRLookup.h"
#include "Display.h"
#include "Timer.h"
#include "Modem.h"
#include "Conf.h"
#include "UMP.h"

#include <string>

class CMMDVMHost
{
public:
  CMMDVMHost(const std::string& confFile);
  ~CMMDVMHost();

  int run();

private:
  CConf           m_conf;
  CModem*         m_modem;
  CDStarNetwork*  m_dstarNetwork;
  CDMRNetwork*    m_dmrNetwork;
  CYSFNetwork*    m_ysfNetwork;
  CP25Network*    m_p25Network;
  CNXDNNetwork*   m_nxdnNetwork;
  CPOCSAGNetwork* m_pocsagNetwork;
  CDisplay*       m_display;
  CUMP*           m_ump;
  unsigned char   m_mode;
  unsigned int    m_dstarRFModeHang;
  unsigned int    m_dmrRFModeHang;
  unsigned int    m_ysfRFModeHang;
  unsigned int    m_p25RFModeHang;
  unsigned int    m_nxdnRFModeHang;
  unsigned int    m_dstarNetModeHang;
  unsigned int    m_dmrNetModeHang;
  unsigned int    m_ysfNetModeHang;
  unsigned int    m_p25NetModeHang;
  unsigned int    m_nxdnNetModeHang;
  unsigned int    m_pocsagNetModeHang;
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
  bool            m_pocsagEnabled;
  unsigned int    m_cwIdTime;
  CDMRLookup*     m_dmrLookup;
  CNXDNLookup*    m_nxdnLookup;
  std::string     m_callsign;
  unsigned int    m_id;
  std::string     m_cwCallsign;

  void readParams();
  bool createModem();
  bool createDStarNetwork();
  bool createDMRNetwork();
  bool createYSFNetwork();
  bool createP25Network();
  bool createNXDNNetwork();
  bool createPOCSAGNetwork();
  void createDisplay();

  void setMode(unsigned char mode);
};

#endif
