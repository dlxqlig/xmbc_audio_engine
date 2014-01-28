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
 *  You can also contact with mail(dlxqlig@gmail.com)
 */

#include "Lockables.h"
#include "CriticalSection.h"
#include "Helpers.h"

#include <pthread.h>

namespace pthreads
{
    static pthread_mutexattr_t recursiveAttr;

    static bool setRecursiveAttr()
    {
        static bool alreadyCalled = false;
        if (!alreadyCalled) {
            pthread_mutexattr_init(&recursiveAttr);
            pthread_mutexattr_settype(&recursiveAttr,PTHREAD_MUTEX_RECURSIVE);
            alreadyCalled = true;
        }
        return true;
    }

    static bool recursiveAttrSet = setRecursiveAttr();

    pthread_mutexattr_t* RecursiveMutex::getRecursiveAttr()
    {
        if (!recursiveAttrSet)
            recursiveAttrSet = setRecursiveAttr();
        return &recursiveAttr;
    }
}
