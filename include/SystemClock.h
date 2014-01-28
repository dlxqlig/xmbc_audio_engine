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

#include <limits>

unsigned int SystemClockMillis();
void Sleep(unsigned int usec);

class Timer
{
    unsigned int startTime;
    unsigned int waitTime;

public:
    inline Timer() : startTime(0), waitTime(0) {}
    inline Timer(unsigned int msTimeDelta) : startTime(SystemClockMillis()), waitTime(msTimeDelta) {}

    inline void Set(unsigned int msTimeDelta) { startTime = SystemClockMillis(); waitTime = msTimeDelta; }

    inline bool IsTimePast() { return waitTime == 0 ? true : (SystemClockMillis() - startTime) >= waitTime; }

    inline unsigned int MillisLeft() {
        unsigned int time = (SystemClockMillis() - startTime);
        return (time >= waitTime) ? 0 : (waitTime - time);
    }

    inline void SetExpired() { waitTime = 0; }
    inline void SetInfinite() { waitTime = std::numeric_limits<unsigned int>::max();}
};

