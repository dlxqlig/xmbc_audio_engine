/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *  You can also contact with mail(gangx.li@intel.com)
 */
#pragma once

#include <vector>

#include "Condition.h"
#include "SingleLock.h"

class CEvent : public NonCopyable
{
    bool manualReset;
    volatile bool signaled;
    unsigned int numWaits;

    // To satisfy the TightConditionVariable requirements and allow the
    // predicate being monitored to include both the signaled and interrupted states.
    ConditionVariable actualCv;
    TightConditionVariable<volatile bool&> condVar;
    CCriticalSection mutex;

    inline bool prepReturn() {
        bool ret = signaled;
        if (!manualReset && numWaits == 0) signaled = false;
        return ret;
    }

public:
    inline CEvent(bool manual = false, bool signaled_ = false) :
    manualReset(manual), signaled(signaled_), numWaits(0), condVar(actualCv, signaled) {}

    inline void Reset() { CSingleLock lock(mutex); signaled = false; }
    void Set();

    // This will wait up to 'milliSeconds' milliseconds for the Event
    // to be triggered. The method will return 'true' if the Event
    // was triggered. Otherwise it will return false.
    inline bool WaitMSec(unsigned int milliSeconds) {
        CSingleLock lock(mutex);
        numWaits++;
        condVar.wait(mutex, milliSeconds);
        numWaits--;
        return prepReturn();
    }

    // This will wait for the Event to be triggered. The method will return
    // 'true' if the Event was triggered. If it was either interrupted
    // it will return false. Otherwise it will return false.
    inline bool Wait() {
        CSingleLock lock(mutex);
        numWaits++;
        condVar.wait(mutex);
        numWaits--;
        return prepReturn();
    }

    // This is mostly for testing. It allows a thread to make sure there are
    // the right amount of other threads waiting.
    inline int getNumWaits() { CSingleLock lock(mutex); return numWaits; }

};
