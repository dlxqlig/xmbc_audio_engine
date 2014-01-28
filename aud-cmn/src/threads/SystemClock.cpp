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
 *  You can also contact with mail(dlxqlig@intel.com)
 */

#include <stdint.h>
#include <time.h>
#include <errno.h>

unsigned int SystemClockMillis() {
    uint64_t now_time;
    static uint64_t start_time = 0;
    static bool start_time_set = false;
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);

    if (!start_time_set) {
        start_time = now_time;
        start_time_set = true;
    }
    return (unsigned int)(now_time - start_time);
}

void Sleep(unsigned int usec) {
    struct timespec time;
    time.tv_sec = usec / 1000000;
    time.tv_nsec = (usec % 1000000)* 1000000 * 1000;
    while (nanosleep(&time, &time) == -1 && errno == EINTR && (time.tv_sec > 0 || time.tv_nsec > 0));

}
