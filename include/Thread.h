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
#ifndef _ATHREAD_H_
#define _ATHREAD_H_

#include <pthread.h>

enum ThreadPriority {
    THREAD_PRIORITY_IDLE = -1,
    THREAD_PRIORITY_NORMAL = 0,
    THREAD_PRIORITY_ABOVE_NORMAL = 1,
    THREAD_PRIORITY_HIGH = 2,
};

class AThread 
{
protected:
    pthread_mutex_t m_lock;
    pthread_t m_thread;
    volatile bool m_running;
    volatile bool m_bStop;

private:
    static void *Run(void *arg);

public:
    AThread();
    ~AThread();

    bool Create(ThreadPriority priority);
    virtual void Process() = 0;
    bool Running();
    pthread_t ThreadHandle();
    bool StopThread();
};

#endif
