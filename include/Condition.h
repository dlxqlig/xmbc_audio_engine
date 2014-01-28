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

#include <pthread.h>
#include <time.h>

#include "SingleLock.h"
#include "Helpers.h"
#include "SystemClock.h"

class ConditionVariable : public NonCopyable
{

private:
    pthread_cond_t cond;

public:
    inline ConditionVariable() {
        pthread_cond_init(&cond, NULL);
    }

    inline ~ConditionVariable() {
        pthread_cond_destroy(&cond);
    }

    inline void wait(CCriticalSection& lock) {
        int count = lock.count;
        lock.count = 0;
        pthread_cond_wait(&cond, &lock.get_underlying().mutex);
        lock.count = count;
    }

    inline bool wait(CCriticalSection& lock, unsigned long milliseconds) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        ts.tv_nsec += milliseconds % 1000 * 1000000;
        ts.tv_sec += milliseconds / 1000 + ts.tv_nsec / 1000000000;
        ts.tv_nsec %= 1000000000;
        int count = lock.count;
        lock.count = 0;
        int res = pthread_cond_timedwait(&cond, &lock.get_underlying().mutex, &ts);
        lock.count = count;
        return res == 0;
    }

    inline void wait(CSingleLock& lock) {
        wait(lock.get_underlying());
    }

    inline bool wait(CSingleLock& lock, unsigned long milliseconds) {
        return wait(lock.get_underlying(), milliseconds);
    }

    inline void notifyAll() {
        pthread_cond_broadcast(&cond);
    }

    inline void notify()
    {
        pthread_cond_signal(&cond);
    }
};

template <typename P> class TightConditionVariable
{
    ConditionVariable& cond;
    P predicate;

public:
    inline TightConditionVariable(ConditionVariable& cv, P predicate_) : cond(cv), predicate(predicate_) {}

    template <typename L> inline void wait(L& lock) {
        while(!predicate) cond.wait(lock);
    }

    template <typename L> inline bool wait(L& lock, unsigned long milliseconds) {
        bool ret = true;
        if (!predicate) {
            if (!milliseconds) {
                cond.wait(lock, milliseconds);
                return !(!predicate);
            }
            else {
                Timer endTime((unsigned int)milliseconds);
                for (bool notdone = true; notdone && ret == true;
                        ret = (notdone = (!predicate)) ? ((milliseconds = endTime.MillisLeft()) != 0) : true)
                    cond.wait(lock, milliseconds);
            }
        }
        return ret;
    }

    inline void notifyAll() { cond.notifyAll(); }
    inline void notify() { cond.notify(); }
};

