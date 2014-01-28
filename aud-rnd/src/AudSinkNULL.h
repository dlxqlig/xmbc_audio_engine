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

#include "AudSink.h"
#include "Thread.h"
#include "Event.h"

namespace AudRender {
class CAudSinkNULL : public AThread, public IAudSink
{
public:
    virtual const char *GetName() { return "NULL"; }

    CAudSinkNULL();
    virtual ~CAudSinkNULL();

    virtual bool Initialize(AudFormat &format, std::string &device);
    virtual void Deinitialize();
    virtual bool IsCompatible(const AudFormat format, const std::string &device);

    virtual double GetDelay();
    virtual double GetCacheTime();
    virtual double GetCacheTotal();
    virtual unsigned int AddPackets(uint8_t *data, unsigned int frames, bool hasAudio);
    virtual void Drain();

private:
    virtual void Process();

    CEvent m_wake;
    CEvent m_inited;
    volatile bool m_draining;
    AudFormat m_format;
    unsigned int m_sink_frame_size;
    unsigned int m_sinkbuffer_size;  ///< total size of the buffer
    unsigned int m_sinkbuffer_level; ///< current level in the buffer
    double m_sinkbuffer_sec_per_byte;
};
}
