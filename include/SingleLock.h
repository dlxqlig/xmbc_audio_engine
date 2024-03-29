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

#include "CriticalSection.h"
#include "Lockables.h"

class CSingleLock : public UniqueLock<CCriticalSection>
{
public:
    inline CSingleLock(CCriticalSection& cs) : UniqueLock<CCriticalSection>(cs) {}
    inline CSingleLock(const CCriticalSection& cs) : UniqueLock<CCriticalSection> ((CCriticalSection&)cs) {}

    inline void Leave() { unlock(); }
    inline void Enter() { lock(); }
protected:
    inline CSingleLock(CCriticalSection& cs, bool dicrim) : UniqueLock<CCriticalSection>(cs,true) {}
};

