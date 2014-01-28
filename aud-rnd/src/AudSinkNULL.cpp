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

#include "AudSinkNULL.h"
#include "AudUtil.h"
#include "Logger.h"

namespace AudRender {

#define CMN_MSG ((std::string("CAudSinkNULL::")+ std::string(__FUNCTION__)).c_str())

CAudSinkNULL::CAudSinkNULL()
  : AThread(),
    m_draining(false),
    m_sink_frame_size(0),
    m_sinkbuffer_size(0),
    m_sinkbuffer_level(0),
    m_sinkbuffer_sec_per_byte(0)
{
}

CAudSinkNULL::~CAudSinkNULL()
{
}

bool CAudSinkNULL::Initialize(AudFormat &format, std::string &device)
{

    format.m_data_format = AUD_IS_RAW(format.m_data_format) ? AUD_FMT_S16NE : AUD_FMT_FLOAT;
    format.m_frames = format.m_sample_rate / 1000 * 250; // 250ms buffer
    format.m_frame_samples = format.m_channel_layout.Count();
    format.m_frame_size = format.m_frame_samples * (CAudUtil::DataFormatToBits(format.m_data_format) >> 3);
    m_format = format;

    m_sink_frame_size = format.m_channel_layout.Count() * CAudUtil::DataFormatToBits(format.m_data_format) >> 3;
    m_sinkbuffer_size = m_sink_frame_size * format.m_sample_rate / 2; // 500ms buffer
    m_sinkbuffer_sec_per_byte = 1.0 / (double)(m_sink_frame_size * format.m_sample_rate);

    m_draining = false;
    m_wake.Reset();
    m_inited.Reset();
    Create(THREAD_PRIORITY_NORMAL);
    if (!m_inited.WaitMSec(100)) {
        while(!m_inited.WaitMSec(1))
            Sleep(100);
    }

    return true;
}

void CAudSinkNULL::Deinitialize()
{
    m_bStop = true;
    StopThread();
}

bool CAudSinkNULL::IsCompatible(const AudFormat format, const std::string &device)
{
    return ((m_format.m_sample_rate    == format.m_sample_rate) &&
          (m_format.m_data_format    == format.m_data_format) &&
          (m_format.m_channel_layout == format.m_channel_layout));
}

double CAudSinkNULL::GetDelay()
{
    double sinkbuffer_seconds_to_empty = m_sinkbuffer_sec_per_byte * (double)m_sinkbuffer_level;
    return sinkbuffer_seconds_to_empty;
}

double CAudSinkNULL::GetCacheTime()
{
    double sinkbuffer_seconds_to_empty = m_sinkbuffer_sec_per_byte * (double)m_sinkbuffer_level;
    return sinkbuffer_seconds_to_empty;
}

double CAudSinkNULL::GetCacheTotal()
{
    return m_sinkbuffer_sec_per_byte * (double)m_sinkbuffer_size;
}

unsigned int CAudSinkNULL::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio)
{
    unsigned int max_frames = (m_sinkbuffer_size - m_sinkbuffer_level) / m_sink_frame_size;
    if (frames > max_frames)
        frames = max_frames;

    if (hasAudio && frames) {
        m_sinkbuffer_level += frames * m_sink_frame_size;
        m_wake.Set();
    }

    return frames;
}

void CAudSinkNULL::Drain()
{
    m_draining = true;
    m_wake.Set();
}

void CAudSinkNULL::Process()
{
    Logger::LogOut(LOG_LEVEL_DEBUG, "%s",  CMN_MSG);

    m_inited.Set();

    while (!m_bStop) {
        if (m_draining) {
            m_sinkbuffer_level = 0;
            m_draining = false;
        }

        // pretend we have a 64k audio buffer
        unsigned int min_buffer_size = 64 * 1024;
        unsigned int read_bytes = m_sinkbuffer_level;
        if (read_bytes > min_buffer_size)
            read_bytes = min_buffer_size;

        if (read_bytes > 0) {
            m_sinkbuffer_level -= read_bytes;

            // we MUST drain at the correct audio sample rate
            // or the NULL sink will not work right. So calc
            // an approximate sleep time.
            int frames_written = read_bytes / m_sink_frame_size;
            double empty_ms = 1000.0 * (double)frames_written / m_format.m_sample_rate;
            Sleep(empty_ms * 1000.0);
        }

        if (m_sinkbuffer_level == 0) {
            // sleep this audio thread, we will get woken when we have audio data.
            m_wake.WaitMSec(250);
        }
    }
}

}
