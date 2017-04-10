/*
*   Copyright (C) 2017 by Jonathan Naylor G4KLX
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

#if !defined(DMRNetworkManager_H)
#define	DMRNetworkManager_H

#include "DMRNetwork.h"
#include "Timer.h"
#include "DMRData.h"
#include "Defines.h"

#include <string>
#include <cstdint>

enum DMR_NETWORK_STATE {
  DMRNS_NONE,
  DMRNS_PRIMARY_RUNNING,
  DMRNS_SECONDARY_RUNNING
};

class CDMRNetworkManager : public IDMRNetwork {
public:
  CDMRNetworkManager(CDMRNetwork* primary, CDMRNetwork* secondary);
  virtual ~CDMRNetworkManager();

  virtual void setOptions(const std::string& options);

  virtual void setConfig(const std::string& callsign, unsigned int rxFrequency, unsigned int txFrequency, unsigned int power, unsigned int colorCode, float latitude, float longitude, int height, const std::string& location, const std::string& description, const std::string& url);

  virtual bool open();

  virtual void enable(bool enabled);

  virtual bool read(CDMRData& data);

  virtual bool write(const CDMRData& data);

  virtual bool wantsBeacon();

  virtual void clock(unsigned int ms);

  virtual void close();

private:
  CDMRNetwork*      m_primary;
  CDMRNetwork*      m_secondary;
  DMR_NETWORK_STATE m_state;
  CTimer            m_poll;
};

#endif
