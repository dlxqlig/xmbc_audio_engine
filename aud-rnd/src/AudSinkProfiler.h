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

#include <stdint.h>
#include "AudSink.h"

namespace AudRender{
class CAudSinkProfiler : public IAudSink
{
public:
    virtual const char *GetName() { return "Profiler"; }

    CAudSinkProfiler();
    virtual ~CAudSinkProfiler();

    virtual bool Initialize(AudFormat &format, std::string &device);
    virtual void Deinitialize();
    virtual bool IsCompatible(const AudFormat format, const std::string &device);

    virtual double GetDelay();
    virtual double GetCacheTime() { return 0.0; }
    virtual double GetCacheTotal() { return 0.0; }
    virtual unsigned int AddPackets(uint8_t *data, unsigned int frames, bool hasAudio);
    virtual void Drain();

private:
    int64_t m_ts;
};
}
