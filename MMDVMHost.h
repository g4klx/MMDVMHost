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

#if !defined(MMDVMHOST_H)
#define	MMDVMHOST_H

#include "DStarNetwork.h"
#include "YSFNetwork.h"
#include "P25Network.h"
#include "DMRNetwork.h"
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

private:
  CConf          m_conf;
  CModem*        m_modem;
  CDStarNetwork* m_dstarNetwork;
  CDMRNetwork*   m_dmrNetwork;
  CYSFNetwork*   m_ysfNetwork;
  CP25Network*   m_p25Network;
  CDisplay*      m_display;
  unsigned char  m_mode;
  unsigned int   m_rfModeHang;
  unsigned int   m_netModeHang;
  CTimer         m_modeTimer;
  CTimer         m_dmrTXTimer;
  CTimer         m_cwIdTimer;
  bool           m_duplex;
  unsigned int   m_timeout;
  bool           m_dstarEnabled;
  bool           m_dmrEnabled;
  bool           m_ysfEnabled;
  bool           m_p25Enabled;
  std::string    m_callsign;

  void readParams();
  bool createModem();
  bool createDStarNetwork();
  bool createDMRNetwork();
  bool createYSFNetwork();
  bool createP25Network();
  void createDisplay();

  void setMode(unsigned char mode);
};

#endif
