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
#include <stdint.h>
#include <limits.h>

#include "AudSinkProfiler.h"
#include "AudUtil.h"
#include "Logger.h"

namespace AudRender {

#define CMN_MSG ((std::string("CAudSinkProfiler::")+ std::string(__FUNCTION__)).c_str())

CAudSinkProfiler::CAudSinkProfiler()
{
}

CAudSinkProfiler::~CAudSinkProfiler()
{
}

bool CAudSinkProfiler::Initialize(AudFormat &format, std::string &device)
{
    if (AUD_IS_RAW(format.m_data_format))
        return false;

    format.m_sample_rate = 192000;
    format.m_channel_layout = CH_LAYOUT_7_1;
    format.m_data_format = AUD_FMT_S32LE;
    format.m_frames = 30720;
    format.m_frame_samples = format.m_channel_layout.Count();
    format.m_frame_size = format.m_frame_samples * sizeof(float);

    return true;
}

void CAudSinkProfiler::Deinitialize()
{
}

bool CAudSinkProfiler::IsCompatible(const AudFormat format, const std::string &device)
{
    if (AUD_IS_RAW(format.m_data_format))
        return false;

    if (format.m_data_format != AUD_FMT_FLOAT)
        return false;

    return true;
}

double CAudSinkProfiler::GetDelay()
{
    return 0.0f;
}

unsigned int CAudSinkProfiler::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio)
{
    int64_t ts = CAudUtil::CurrentHostCounter();
    Logger::LogOut(LOG_LEVEL_DEBUG, "%s - latency %f ms", (float)(ts - m_ts) / 1000000.0f, CMN_MSG);
    m_ts = ts;
    return frames;
}

void CAudSinkProfiler::Drain()
{
}

}
