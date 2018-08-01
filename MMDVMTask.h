/*
*   Copyright (C) 2018 by shawn.chain@gmail.com BG5HHP
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

#ifndef MMDVM_TASK_H
#define MMDVM_TASK_H

#include "StopWatch.h"
#include "Timer.h"


#include <cstdint>

class CMMDVMHost;
class CRSSIInterpolator;

struct CMMDVMTaskContext
{
    CMMDVMHost* host;
    CRSSIInterpolator* rssi;
    void* data;
};

/*
 * Task Abstract Class
 */
class CMMDVMTask{
public:
    CMMDVMTask();
    virtual ~CMMDVMTask() = 0;
    virtual bool run(CMMDVMTaskContext* ctx) = 0;
protected:
    CMMDVMHost*  m_host;
    CStopWatch   m_stopWatch;
};

/*
 * DMR Task
 */
class CDMRControl;
class CDMRNetwork;
class CDMRTask : CMMDVMTask{
public:
    CDMRTask();
    virtual ~CDMRTask();

    static CDMRTask* create(CMMDVMTaskContext& ctx);

    virtual bool run(CMMDVMTaskContext* ctx);

    void enableNetwork(bool enabled);
    void startDMR(bool started);

private:
    CDMRControl* m_dmrControl;
    CDMRNetwork* m_dmrNetwork;

    CTimer m_dmrBeaconDurationTimer;
    CTimer m_dmrBeaconIntervalTimer;
    CTimer m_dmrTXTimer;

    unsigned int m_dmrRFModeHang;
    unsigned int m_dmrNetModeHang;

    bool createDMRControl(CMMDVMTaskContext& ctx);
    bool createDMRNetwork(CMMDVMTaskContext& ctx);
};
#endif