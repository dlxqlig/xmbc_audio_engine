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
#include <errno.h>

#include "Lockables.h"
#include "Helpers.h"


class ConditionVariable;

namespace pthreads
{
    class RecursiveMutex
    {
        pthread_mutexattr_t* getRecursiveAttr();
    public:
        pthread_mutex_t mutex;

        friend class ConditionVariable;
    public:
        inline RecursiveMutex() { pthread_mutex_init(&mutex, getRecursiveAttr()); }
        inline ~RecursiveMutex() { pthread_mutex_destroy(&mutex); }

        inline void lock() { pthread_mutex_lock(&mutex); }
        inline void unlock() { pthread_mutex_unlock(&mutex); }
        inline bool try_lock() { return (pthread_mutex_trylock(&mutex) == 0); }
    };
};


// A CCriticalSection is a CountingLockable whose implementation is a
// native recursive mutex.
class CCriticalSection : public CountingLockable<pthreads::RecursiveMutex> {};

