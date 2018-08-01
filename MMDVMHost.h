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
#include "DMRLookup.h"
#include "Display.h"
#include "Timer.h"
#include "Modem.h"
#include "Conf.h"
#include "UMP.h"

#include <string>

class CRSSIInterpolator;
class CDStarControl;
class CDMRTask;
class CYSFTask;
class CP25Task;
class CNXDNControl;
class CPOCSAGControl;

class CMMDVMHost
{
public:
  friend class CDMRTask;
  friend class CYSFTask;
  friend class CP25Task;
  CMMDVMHost(const std::string& confFile);
  ~CMMDVMHost();

  int run();

private:
  CConf           m_conf;
  CModem*         m_modem;
  CDStarNetwork*  m_dstarNetwork;
  CDMRTask*       m_dmrTask;
  CYSFTask*       m_ysfTask;
  CP25Task*       m_p25Task;
  CNXDNNetwork*   m_nxdnNetwork;
  CPOCSAGNetwork* m_pocsagNetwork;
  CDisplay*       m_display;
  CUMP*           m_ump;
  unsigned char   m_mode;
  unsigned int    m_dstarRFModeHang;
  unsigned int    m_nxdnRFModeHang;
  unsigned int    m_dstarNetModeHang;
  unsigned int    m_nxdnNetModeHang;
  unsigned int    m_pocsagNetModeHang;
  CTimer          m_modeTimer;
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
  bool createNXDNNetwork();
  bool createPOCSAGNetwork();

  CDStarControl* createDStarControl(CRSSIInterpolator* rssi);
  CNXDNControl* createNXDNControl(CRSSIInterpolator* rssi);
  CPOCSAGControl* createPOCSAGControl(CTimer& pocsagTimer);

  bool daemonize();

  void setMode(unsigned char mode);
};

#endif
