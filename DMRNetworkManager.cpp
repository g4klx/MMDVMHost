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

#include "DMRNetworkManager.h"

#include "Log.h"

#include <cstdio>
#include <cassert>

CDMRNetworkManager::CDMRNetworkManager(CDMRNetwork* primary, CDMRNetwork* secondary) :
m_primary(primary),
m_secondary(secondary),
m_state(DMRNS_NONE),
m_poll(1000U, 300U)
{
  assert(primary != NULL);
  assert(secondary != NULL);
}

CDMRNetworkManager::~CDMRNetworkManager()
{
  delete m_primary;
  delete m_secondary;
}

void CDMRNetworkManager::setOptions(const std::string& options)
{
  m_primary->setOptions(options);
  m_secondary->setOptions(options);
}

void CDMRNetworkManager::setConfig(const std::string& callsign, unsigned int rxFrequency, unsigned int txFrequency, unsigned int power, unsigned int colorCode, float latitude, float longitude, int height, const std::string& location, const std::string& description, const std::string& url)
{
  m_primary->setConfig(callsign, rxFrequency, txFrequency, power, colorCode, latitude, longitude, height, location, description, url);
  m_secondary->setConfig(callsign, rxFrequency, txFrequency, power, colorCode, latitude, longitude, height, location, description, url);
}

bool CDMRNetworkManager::open()
{
  LogMessage("DMR, Using Primary DMR Network");

  bool ret = m_primary->open();
  if (!ret)
    return false;

  m_state = DMRNS_PRIMARY_RUNNING;

  m_poll.start();

  return true;
}

void CDMRNetworkManager::enable(bool enabled)
{
  m_primary->enable(enabled);
  m_secondary->enable(enabled);
}

bool CDMRNetworkManager::read(CDMRData& data)
{
  switch (m_state) {
  case DMRNS_PRIMARY_RUNNING: {
      m_secondary->read(data);
      bool ret = m_primary->read(data);
      if (ret)
        m_poll.start();
      return ret;
    }

  case DMRNS_SECONDARY_RUNNING: {
      m_primary->read(data);
      bool ret = m_secondary->read(data);
      if (ret)
        m_poll.start();
      return ret;
    }

  default:
    return false;
  }
}

bool CDMRNetworkManager::write(const CDMRData& data)
{
  switch (m_state) {
  case DMRNS_PRIMARY_RUNNING:
    return m_primary->write(data);

  case DMRNS_SECONDARY_RUNNING:
    return m_secondary->write(data);

  default:
    return false;
  }
}

void CDMRNetworkManager::close()
{
  switch (m_state) {
  case DMRNS_PRIMARY_RUNNING:
    m_primary->close();
    break;

  case DMRNS_SECONDARY_RUNNING:
    m_secondary->close();
    break;

  default:
    break;
  }
}

void CDMRNetworkManager::clock(unsigned int ms)
{
  m_poll.clock(ms);
  if (m_poll.isRunning() && m_poll.hasExpired()) {
    switch (m_state) {
    case DMRNS_PRIMARY_RUNNING: {
        bool ret = m_primary->isRunning();
        if (!ret) {
          LogMessage("DMR, Using Secondary DMR Network");
          m_secondary->open();
          m_state = DMRNS_SECONDARY_RUNNING;
        }
      }
      break;

    case DMRNS_SECONDARY_RUNNING: {
        bool ret = m_primary->isRunning();
        if (ret) {
          LogMessage("DMR, Using Primary DMR Network");
          m_secondary->close();
          m_state = DMRNS_PRIMARY_RUNNING;
        }
      }
      break;

    default:
      break;
    }

    m_poll.start();
  }
}

bool CDMRNetworkManager::wantsBeacon()
{
  switch (m_state) {
  case DMRNS_PRIMARY_RUNNING:
    return m_primary->wantsBeacon();

  case DMRNS_SECONDARY_RUNNING:
    return m_secondary->wantsBeacon();

  default:
    return false;
  }
}
