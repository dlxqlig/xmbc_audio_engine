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

#include "Condition.h"
#include "SingleLock.h"
#include "Helpers.h"

class CSharedSection
{
    CCriticalSection sec;
    ConditionVariable actualCv;
    TightConditionVariable<InversePredicate<unsigned int&> > cond;

    unsigned int sharedCount;

public:
    inline CSharedSection() : cond(actualCv, InversePredicate<unsigned int&>(sharedCount)), sharedCount(0) {}

    inline void lock() { CSingleLock l(sec); while (sharedCount) cond.wait(l); sec.lock(); }
    inline bool try_lock() { return (sec.try_lock() ? ((sharedCount == 0) ? true : (sec.unlock(), false)) : false); }
    inline void unlock() { sec.unlock(); }

    inline void lock_shared() { CSingleLock l(sec); sharedCount++; }
    inline bool try_lock_shared() { return (sec.try_lock() ? sharedCount++, sec.unlock(), true : false); }
    inline void unlock_shared() { CSingleLock l(sec); sharedCount--; if (!sharedCount) { cond.notifyAll(); } }
};

class CSharedLock : public SharedLock<CSharedSection>
{
public:
    inline CSharedLock(CSharedSection& cs) : SharedLock<CSharedSection>(cs) {}
    inline CSharedLock(const CSharedSection& cs) : SharedLock<CSharedSection>((CSharedSection&)cs) {}

    inline bool IsOwner() const { return owns_lock(); }
    inline void Enter() { lock(); }
    inline void Leave() { unlock(); }
};

class CExclusiveLock : public UniqueLock<CSharedSection>
{
public:
    inline CExclusiveLock(CSharedSection& cs) : UniqueLock<CSharedSection>(cs) {}
    inline CExclusiveLock(const CSharedSection& cs) : UniqueLock<CSharedSection> ((CSharedSection&)cs) {}

    inline bool IsOwner() const { return owns_lock(); }
    inline void Leave() { unlock(); }
    inline void Enter() { lock(); }
};

