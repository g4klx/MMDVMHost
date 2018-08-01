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
    //CRSSIInterpolator* rssi;
    void* data;
};

//---------------------------------------------------------
// Abstract Task Inteface
//---------------------------------------------------------
class CMMDVMTask{
public:
    CMMDVMTask(CMMDVMHost* host);
    virtual ~CMMDVMTask() = 0;
    virtual bool run(CMMDVMTaskContext* ctx) = 0;
    virtual void enableNetwork(bool enabled) = 0;
protected:
    CMMDVMHost*  m_host;
    CStopWatch   m_stopWatch;
};

//---------------------------------------------------------
// DMR Task
//---------------------------------------------------------
class CDMRControl;
class CDMRNetwork;
class CDMRTask : CMMDVMTask{
public:
    CDMRTask(CMMDVMHost* host);
    virtual ~CDMRTask();

    static CDMRTask* create(CMMDVMHost* host, CRSSIInterpolator* rssi);

    virtual bool run(CMMDVMTaskContext* ctx);
    virtual void enableNetwork(bool enabled);

    void startDMR(bool started);

private:
    CDMRControl* m_dmrControl;
    CDMRNetwork* m_dmrNetwork;

    CTimer m_dmrBeaconDurationTimer;
    CTimer m_dmrBeaconIntervalTimer;
    CTimer m_dmrTXTimer;

    unsigned int m_dmrRFModeHang;
    unsigned int m_dmrNetModeHang;

    bool createDMRControl(CRSSIInterpolator* rssi);
    bool createDMRNetwork();
};

//---------------------------------------------------------
// YSF Task
//---------------------------------------------------------
#pragma mark -
class CYSFControl;
class CYSFNetwork;
class CYSFTask : CMMDVMTask{

public:
    CYSFTask(CMMDVMHost* host);
    virtual ~CYSFTask();

    static CYSFTask* create(CMMDVMHost* host, CRSSIInterpolator* rssi);

    virtual bool run(CMMDVMTaskContext* ctx);
    virtual void enableNetwork(bool enabled);

private:
    CYSFControl* m_ysfControl;
    CYSFNetwork* m_ysfNetwork;

    unsigned int m_ysfRFModeHang;
    unsigned int m_ysfNetModeHang;

    bool createYSFControl(CRSSIInterpolator* rssi);
    bool createYSFNetwork();
};

//---------------------------------------------------------
// P25 Task
//---------------------------------------------------------
#pragma mark -
class CP25Control;
class CP25Network;
class CP25Task : CMMDVMTask{

public:
    CP25Task(CMMDVMHost* host);
    virtual ~CP25Task();

    static CP25Task* create(CMMDVMHost* host, CRSSIInterpolator* rssi);

    virtual bool run(CMMDVMTaskContext* ctx);
    virtual void enableNetwork(bool enabled);

private:
    CP25Control* m_p25Control;
    CP25Network* m_p25Network;

    unsigned int m_p25RFModeHang;
    unsigned int m_p25NetModeHang;

    bool createP25Control(CRSSIInterpolator* rssi);
    bool createP25Network();
};
#endif