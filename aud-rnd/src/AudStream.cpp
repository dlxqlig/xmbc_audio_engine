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
#include <cmath>

#include "AudRender.h"
#include "AudStream.h"
#include "Logger.h"
#include "AudUtil.h"
#include "AudBuffer.h"
#include "SingleLock.h"

using namespace std;

namespace AudRender {

#define CMN_MSG ((std::string("CAudStream::")+ std::string(__FUNCTION__)).c_str())

CAudStream::CAudStream(CAudRender* ar,
                        enum AudDataFormat dataFormat,
                        unsigned int sampleRate,
                        unsigned int encodedSampleRate,
                        CAudChInfo channelLayout,
                        unsigned int options,
                        CCriticalSection& lock) :
    m_render(ar),
    m_lock(lock),
    m_internal_ratio(1.0),
    m_convert_buffer(NULL),
    m_valid(false),
    m_delete(false),
    m_volume(1.0f ),
    m_refill_buffer(0),
    m_frames_buffered(0),
    m_new_packet(NULL),
    m_packet(NULL),
    m_draining(false)
{
    EntryLogger entry(__FUNCTION__);

    m_init_data_format = dataFormat;
    m_init_sample_rate = sampleRate;
    m_init_encoded_sample_rate = encodedSampleRate;
    m_init_channel_layout = channelLayout;
    m_ch_layout_count = channelLayout.Count();
    m_paused = (options & AUDSTREAM_PAUSED) != 0;
    m_auto_start = (options & AUDSTREAM_AUTOSTART) != 0;

    if (m_auto_start)
        m_paused = true;

    ASSERT(m_init_channel_layout.Count());
}

void CAudStream::InitializeRemap()
{
    CSingleLock lock(m_lock);

    if (!AUD_IS_RAW(m_init_data_format)) {

        m_remap.Initialize(m_init_channel_layout, m_render->GetChannelLayout(),
                false, false, m_render->GetStdChLayout());

        // remap channels again if channle layout changed
        if (m_render->GetChannelLayout() != m_rnd_channel_layout) {
            InternalFlush();
            m_rnd_channel_layout = m_render->GetChannelLayout();
            m_samples_per_frame = m_render->GetChannelLayout().Count();
            m_rnd_bytes_per_frame = AUD_IS_RAW(m_init_data_format) ? m_bytes_per_frame : (m_samples_per_frame * sizeof(float));
        }
    }
}

void CAudStream::Initialize()
{
    EntryLogger entry(__FUNCTION__);
    CSingleLock lock(m_lock);

    if (m_valid) {
        InternalFlush();
        delete m_new_packet;

        if (m_convert)
            free_aligned(m_convert_buffer);

        if (m_resample) {
            free_aligned(m_ssrc_data.data_out);
            m_ssrc_data.data_out = NULL;
        }
    }

    enum AudDataFormat useDataFormat = m_init_data_format;
    if (AUD_IS_RAW(m_init_data_format)) {
        useDataFormat = m_render->GetSinkDataFormat();
        m_init_channel_layout = m_render->GetSinkChLayout();
        m_samples_per_frame = m_init_channel_layout.Count();
    }
    else {
        if (!m_init_channel_layout.Count()) {
            m_valid = false;
            return;
        }
        m_samples_per_frame = m_render->GetChannelLayout().Count();
    }

    m_bytes_per_sample = (CAudUtil::DataFormatToBits(useDataFormat) >> 3);
    m_bytes_per_frame = m_bytes_per_sample * m_init_channel_layout.Count();

    m_rnd_channel_layout = m_render->GetChannelLayout();
    m_rnd_bytes_per_frame = AUD_IS_RAW(m_init_data_format) ? m_bytes_per_frame : (m_samples_per_frame * sizeof(float));

    // set the waterlevel to 75 percent of the number of frames per second.
    m_water_level = m_render->GetSampleRate() - (m_render->GetSampleRate() / 4);
    m_refill_buffer = m_water_level;
    Logger::LogOut(LOG_LEVEL_DEBUG, "%s m_rnd_bytes_per_frame[%d]:m_water_level[%d]:m_refill_buffer[%d].",
            CMN_MSG, m_rnd_bytes_per_frame, m_water_level, m_refill_buffer);

    m_format.m_data_format = useDataFormat;
    m_format.m_sample_rate = m_init_sample_rate;
    m_format.m_encoded_rate = m_init_encoded_sample_rate;
    m_format.m_channel_layout = m_init_channel_layout;
    m_format.m_frames = m_init_sample_rate / 8;
    m_format.m_frame_samples = m_format.m_frames * m_init_channel_layout.Count();
    m_format.m_frame_size = m_bytes_per_frame;

    CAudUtil::AudFormatToString(m_format);
    m_new_packet = new PPacket();

    if (AUD_IS_RAW(m_init_data_format)) {
        m_new_packet->data.Alloc(m_format.m_frames * m_format.m_frame_size);
    }
    else {
        if (!m_remap.Initialize(m_init_channel_layout, m_rnd_channel_layout, false, false, m_render->GetStdChLayout())) {
            m_valid = false;
            return;
        }
        m_new_packet->data.Alloc(m_format.m_frame_samples * sizeof(float));
    }

    m_packet = NULL;
    m_input_buffer.Alloc(m_format.m_frames * m_format.m_frame_size);
    m_resample = (m_init_sample_rate != m_render->GetSampleRate()) && !AUD_IS_RAW(m_init_data_format);
    m_convert = m_init_data_format != AUD_FMT_FLOAT && !AUD_IS_RAW(m_init_data_format);

    if (m_convert) {
        Logger::LogOut(LOG_LEVEL_DEBUG, "%s Converting from %s to AUD_FMT_FLOAT",
                CMN_MSG, CAudUtil::DataFormatToStr(m_init_data_format));

        m_convert_fn = CAudConvert::ToFloat(m_init_data_format);
        if (m_convert_fn) {
            m_convert_buffer = (float*)malloc_aligned(m_format.m_frame_samples * sizeof(float), 16);
        }
        else {
            m_valid = false;
        }
    }
    else {
        m_convert_buffer = (float*)m_input_buffer.Raw(m_format.m_frames * m_format.m_frame_size);
    }

    if (m_resample) {
        int err;
        m_ssrc = src_new(SRC_SINC_MEDIUM_QUALITY, m_init_channel_layout.Count(), &err);
        m_ssrc_data.data_in = m_convert_buffer;
        m_internal_ratio = (double)m_render->GetSampleRate() / (double)m_init_sample_rate;
        m_ssrc_data.src_ratio = m_internal_ratio;
        m_ssrc_data.data_out = (float*)malloc_aligned(m_format.m_frame_samples * (int)std::ceil(m_ssrc_data.src_ratio) * sizeof(float), 16);
        m_ssrc_data.output_frames = m_format.m_frames * (long)std::ceil(m_ssrc_data.src_ratio);
        m_ssrc_data.end_of_input = 0;
        // we must buffer the same amount as before but taking the source sample rate into account
        if (m_internal_ratio < 1) {
            m_water_level *= (1.0 / m_internal_ratio);
            m_refill_buffer = m_water_level;
        }
    }

    m_ch_layout_count = m_format.m_channel_layout.Count();
    m_valid = true;

    Logger::LogOut(LOG_LEVEL_DEBUG, "%s end", CMN_MSG);
}

void CAudStream::Destroy()
{
    CSingleLock lock(m_lock);

    m_valid = false;
    m_delete = true;
}

CAudStream::~CAudStream()
{
    CSingleLock lock(m_lock);

    InternalFlush();
    if (m_convert)
        free_aligned(m_convert_buffer);

    if (m_resample) {
        free_aligned(m_ssrc_data.data_out);
        src_delete(m_ssrc);
        m_ssrc = NULL;
    }

    delete m_new_packet;
    delete m_packet;
}

unsigned int CAudStream::GetSpace()
{
    EntryLogger entry(__FUNCTION__);
    CSingleLock lock(m_lock);

    //Logger::LogOut(LOG_LEVEL_ERROR, "GetSpace valid[%d]:m_draining[%d]:m_frames_buffered[%d]:m_water_level[%d]", m_valid, m_draining,m_frames_buffered,m_water_level);
    if (!m_valid || m_draining) {
        return 0;
    }

    if (m_frames_buffered >= m_water_level)
        return 0;

    //Logger::LogOut(LOG_LEVEL_ERROR, "m_input_buffer.Free[%d],m_frameSize[%d],m_frames_buffered[%d]:m_water_level[%d",
    // m_frames_buffered, m_water_level, m_input_buffer.Free(), m_format.m_frame_size);
    return m_input_buffer.Free() + (std::max(0U, (m_water_level - m_frames_buffered)) * m_format.m_frame_size);
}

unsigned int CAudStream::AddData(void *data, unsigned int size)
{
    EntryLogger entry(__FUNCTION__);
    CSingleLock lock(m_lock);

    if (!m_valid || size == 0 || data == NULL) {
        return 0;
    }

    if (m_draining) {
        // if the stream has finished draining, cork it
        if (m_packet && !m_packet->data.Used() && m_out_buffer.empty()) {
            m_draining = false;
        }
        else {
            return 0;
        }
    }

    size = std::min(size, GetSpace());
    if (size == 0)
        return 0;

    unsigned int taken = 0;
    while(size) {

        unsigned int copy = std::min((unsigned int)m_input_buffer.Free(), size);
        if (copy > 0) {
            m_input_buffer.Push(data, copy);
            size -= copy;
            taken += copy;
            data = (uint8_t*)data + copy;
        }

        if (m_input_buffer.Free() == 0) {
            unsigned int consumed = ProcessFrameBuffer();
            m_input_buffer.Shift(NULL, consumed);
        }
    }

    lock.Leave();

    // dealing with autostart flag
    if (m_auto_start && m_frames_buffered >= m_water_level) {
        Logger::LogOut(LOG_LEVEL_INFO, "%s start stream m_auto_start[%d]:m_frames_buffered[%d]:m_water_level[%d]",
                CMN_MSG, m_auto_start, m_frames_buffered, m_water_level);
        Resume();
    }

    return taken;
}

unsigned int CAudStream::ProcessFrameBuffer()
{
    EntryLogger entry(__FUNCTION__);

    uint8_t *data;
    unsigned int frames, consumed, sampleSize;
    unsigned int samples;

    if (m_convert) {
        data = (uint8_t*)m_convert_buffer;
        samples = m_convert_fn(
                (uint8_t*)m_input_buffer.Raw(m_input_buffer.Used()),
                m_input_buffer.Used() / m_bytes_per_sample,
                m_convert_buffer);
        sampleSize = sizeof(float);
    }
    else {
        data = (uint8_t*)m_input_buffer.Raw(m_input_buffer.Used());
        samples = m_input_buffer.Used() / m_bytes_per_sample;
        sampleSize = m_bytes_per_sample;
    }

    if (samples == 0)
    return 0;

    //Logger::LogOut(LOG_LEVEL_ERROR,"CAudStream::ProcessFrameBuffer samples[%d] sampleSize[%d]", samples, sampleSize);
    if (m_resample) {
        m_ssrc_data.input_frames = samples / m_ch_layout_count;
        if (src_process(m_ssrc, &m_ssrc_data) != 0)
            return 0;
        data = (uint8_t*)m_ssrc_data.data_out;
        frames = m_ssrc_data.output_frames_gen;
        consumed = m_ssrc_data.input_frames_used * m_bytes_per_frame;
        if (!frames)
            return consumed;

        samples = frames * m_ch_layout_count;
    }
    else {
        data = (uint8_t*)m_convert_buffer;
        frames = samples / m_ch_layout_count;
        consumed = frames * m_bytes_per_frame;
    }

    //Logger::LogOut(LOG_LEVEL_ERROR,"CAudStream::ProcessFrameBuffer resamples[%d]", samples);
    //Logger::LogOut(LOG_LEVEL_ERROR,"CAudStream::ProcessFrameBuffer frames[%d] consumed[%d]", frames, consumed);
    //Logger::LogOut(LOG_LEVEL_ERROR, "m_refill_buffer[%d], frames[%d], m_frames_buffered[%d], consumed[%d],m_convert[%d]",
    //m_refill_buffer, frames, m_frames_buffered, consumed, m_convert);
    if (m_refill_buffer) {
        if (frames > m_refill_buffer)
            m_refill_buffer = 0;
        else
            m_refill_buffer -= frames;
    }

    m_frames_buffered += frames;
    const unsigned int inputBlockSize = m_format.m_frames * m_format.m_channel_layout.Count() * sampleSize;

    size_t remaining = samples * sampleSize;
    //Logger::LogOut(LOG_LEVEL_ERROR,"CAudStream::ProcessFrameBuffer inputBlockSize[%d] m_format.m_frame_samples[%d] remaining[%d]",
    //inputBlockSize, m_format.m_frame_samples, remaining);
    while (remaining) {
        size_t copy = std::min(m_new_packet->data.Free(), remaining);
        m_new_packet->data.Push(data, copy);
        data += copy;
        remaining -= copy;

        //Logger::LogOut(LOG_LEVEL_ERROR,"CAudStream::ProcessFrameBuffer remaining[%d] copy[%d] m_new_packet->data.Free() [%d] size[%d]",
        //remaining, copy, m_new_packet->data.Free(), m_new_packet->data.Size());
        // wait till we have a full packet, or no more data before processing the packet
        if ((!m_draining || remaining) && m_new_packet->data.Free() > 0)
            continue;

        // if we have a full block of data
        if (AUD_IS_RAW(m_init_data_format)) {
            m_out_buffer.push_back(m_new_packet);
            m_new_packet = new PPacket();
            m_new_packet->data.Alloc(inputBlockSize);
            continue;
        }

        // make a new packet for downmix/remap
        PPacket *pkt = new PPacket();
        size_t frames = m_new_packet->data.Used() / m_format.m_channel_layout.Count() / sizeof(float);
        size_t used = frames * m_rnd_channel_layout.Count() * sizeof(float);
        pkt->data.Alloc(used);
        m_remap.Remap(
                (float*)m_new_packet->data.Raw(m_new_packet->data.Used()),
                (float*)pkt->data.Take(used),
                frames);

        // add to the output
        m_out_buffer.push_back(pkt);
        m_new_packet->data.Empty();
    }

    return consumed;
}

uint8_t* CAudStream::GetFrame()
{
    CSingleLock lock(m_lock);

    // if we have been deleted or are refilling but not draining
    if (!m_valid || m_delete || (m_refill_buffer && !m_draining))
        return NULL;
    //Logger::LogOut(LOG_LEVEL_ERROR, "CAudStream::GetFrame m_frames_buffered[%d]:m_water_level[%d]", m_frames_buffered, m_water_level);
    // if the packet is empty, advance to the next one
    if (!m_packet || m_packet->data.CursorEnd()) {
        delete m_packet;
        m_packet = NULL;

        // no more packets, return null
        if (m_out_buffer.empty()) {
            if (m_draining) {
                return NULL;
            }
            else {
                // underrun, we need to refill our buffers
                Logger::LogOut(LOG_LEVEL_DEBUG, "%s Underrun", CMN_MSG);
                ASSERT(m_water_level > m_frames_buffered);
                m_refill_buffer = m_water_level - m_frames_buffered;
                return NULL;
            }
        }

        m_packet = m_out_buffer.front();
        m_out_buffer.pop_front();
    }

    // get one frame of data
    uint8_t *ret = (uint8_t*)m_packet->data.CursorRead(m_rnd_bytes_per_frame);
    if (m_frames_buffered > 0)
        --m_frames_buffered;

    return ret;
}

double CAudStream::GetDelay()
{
    CSingleLock lock(m_lock);

    if (m_delete)
        return 0.0;

    double delay = m_render->GetDelay();
    delay += (double)(m_input_buffer.Used() / m_format.m_frame_size) / (double)m_format.m_sample_rate;
    delay += (double)m_frames_buffered / (double)m_render->GetSampleRate();
    return delay;
}

double CAudStream::GetCacheTime()
{
    CSingleLock lock(m_lock);

    if (m_delete)
        return 0.0;

    double time = m_render->GetCacheTime();
    time += (double)(m_input_buffer.Used() / m_format.m_frame_size) / (double)m_format.m_sample_rate;
    time += (double)m_frames_buffered  / (double)m_render->GetSampleRate();
    return time;
}

double CAudStream::GetCacheTotal()
{
    CSingleLock lock(m_lock);

    if (m_delete)
        return 0.0;

    double total = m_render->GetCacheTotal();
    total += (double)(m_input_buffer.Size() / m_format.m_frame_size) / (double)m_format.m_sample_rate;
    total += (double)m_water_level / (double)m_render->GetSampleRate();
    return total;
}

void CAudStream::Pause()
{
    CSingleLock lock(m_lock);

    if (m_paused)
        return;

    m_paused = true;
    m_render->PauseStream(this);
}

void CAudStream::Resume()
{
    CSingleLock lock(m_lock);
    if (!m_paused)
        return;
    m_paused = false;
    m_auto_start = false;
    m_render->ResumeStream(this);
}

void CAudStream::Drain()
{
    CSingleLock lock(m_lock);
    m_draining = true;
}

bool CAudStream::IsDrained()
{
    CSingleLock lock(m_lock);
    return (m_draining && !m_packet && m_out_buffer.empty());
}

void CAudStream::Flush()
{
    CSingleLock lock(m_lock);
    InternalFlush();

    // internal flush does not do this as these samples are still valid if we are re-initializing
    m_input_buffer.Empty();
}

void CAudStream::InternalFlush()
{
    // invalidate any incoming samples
    m_new_packet->data.Empty();

    // clear the current buffered packet, we cant delete the data as it may be
    // in use by the Render thread, so we just seek to the end of the buffer
    if (m_packet)
        m_packet->data.CursorSeek(m_packet->data.Size());

    // clear any other buffered packets
    while (!m_out_buffer.empty()) {
        PPacket *p = m_out_buffer.front();
        m_out_buffer.pop_front();
        delete p;
    }

    m_frames_buffered = 0;
    m_refill_buffer = m_water_level;
    m_draining = false;
}

}
