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

#include "Helpers.h"

template<class L> class CountingLockable : public NonCopyable
{
    friend class ConditionVariable;

protected:
    L mutex;
    unsigned int count;

public:
    inline CountingLockable() : count(0) {}

    inline void lock() { mutex.lock(); count++; }
    inline bool try_lock() { return mutex.try_lock() ? count++, true : false; }
    inline void unlock() { count--; mutex.unlock(); }

    inline unsigned int exit() {
        // it's possibe we don't actually own the lock
        // so we will try it.
        unsigned int ret = 0;
        if (try_lock()) {
            ret = count - 1;  // The -1 is because we don't want
            // to count the try_lock increment.
            // We must NOT compare "count" in this loop since
            // as soon as the last unlock is called another thread
            // can modify it.
            for (unsigned int i = 0; i <= ret; i++) // This will also unlock the try_lock.
                unlock();
        }

        return ret;
    }

     // Restore a previous exit to the provided level.
    inline void restore(unsigned int restoreCount) {
        for (unsigned int i = 0; i < restoreCount; i++)
            lock();
    }

    inline L& get_underlying() { return mutex; }
};

template<typename L> class UniqueLock : public NonCopyable
{
protected:
    L& mutex;
    bool owns;
    inline UniqueLock(L& lockable) : mutex(lockable), owns(true) { mutex.lock(); }
    inline UniqueLock(L& lockable, bool try_to_lock_discrim ) : mutex(lockable) { owns = mutex.try_lock(); }
    inline ~UniqueLock() { if (owns) mutex.unlock(); }

public:

    inline bool owns_lock() const { return owns; }

    inline void lock() { mutex.lock(); owns=true; }
    inline bool try_lock() { return (owns = mutex.try_lock()); }
    inline void unlock() { if (owns) { mutex.unlock(); owns = false; } }

    inline L& get_underlying() { return mutex; }
};

template<typename L> class SharedLock : public NonCopyable
{
protected:
    L& mutex;
    bool owns;
    inline SharedLock(L& lockable) : mutex(lockable), owns(true) { mutex.lock_shared(); }
    inline ~SharedLock() { if (owns) mutex.unlock_shared(); }

    inline bool owns_lock() const { return owns; }
    inline void lock() { mutex.lock_shared(); owns = true; }
    inline bool try_lock() { return (owns = mutex.try_lock_shared()); }
    inline void unlock() { if (owns) mutex.unlock_shared(); owns = false; }

    inline L& get_underlying() { return mutex; }
};

