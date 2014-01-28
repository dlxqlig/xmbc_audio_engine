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

#include <list>
#include <samplerate.h>

#include "AudFormat.h"
#include "AudBuffer.h"
#include "AudConvert.h"
#include "AudRemap.h"
#include "CriticalSection.h"

namespace AudRender {

enum AudStreamOptions {
    AUDSTREAM_FORCE_RESAMPLE = 0x01, // resample
    AUDSTREAM_PAUSED = 0x02, // create stream with paused state
    AUDSTREAM_AUTOSTART = 0x04  // create stream start auto
};

class CAudRender;

class CAudStream {

protected:
    friend class CAudRender;
    CAudStream(CAudRender* ar, enum AudDataFormat format, unsigned int sampleRate,
            unsigned int encodedSamplerate, CAudChInfo channelLayout, unsigned int options, CCriticalSection& lock);
    virtual ~CAudStream();

    bool IsPaused() { return m_paused; }
    bool IsDestroyed() { return m_delete; }
    bool IsValid() { return m_valid; }
    const bool IsRaw() const { return AUD_IS_RAW(m_init_data_format); }

    void InitializeRemap();
    void Initialize();
    void Destroy();
    uint8_t* GetFrame();

public:
    virtual unsigned int GetSpace();
    virtual unsigned int AddData(void *data, unsigned int size);
    virtual double GetDelay();
    virtual bool IsBuffering() { return m_refill_buffer > 0; }
    virtual double GetCacheTime();
    virtual double GetCacheTotal();

    virtual void Pause();
    virtual void Resume();
    virtual void Drain();
    virtual bool IsDraining() { return m_draining; }
    virtual bool IsDrained();
    virtual void Flush();

    virtual float GetVolume() { return m_volume; }
    virtual void SetVolume (float volume) { m_volume = std::max( 0.0f, std::min(1.0f, volume)); }
    virtual const unsigned int GetFrameSize() const  { return m_format.m_frame_size; }
    virtual const unsigned int GetChannelCount() const { return m_init_channel_layout.Count(); }

    virtual const unsigned int GetSampleRate() const { return m_init_sample_rate; }
    virtual const unsigned int GetEncodedSampleRate() const { return m_init_encoded_sample_rate; }
    virtual const enum AudDataFormat GetDataFormat() const { return m_init_data_format; }
    virtual const CAudChInfo GetChannelLayout() const { return m_init_channel_layout; }

private:
    void InternalFlush();
    void CheckResampleBuffers();
    unsigned int ProcessFrameBuffer();

    CAudRender* m_render;

    CCriticalSection& m_lock;
    enum AudDataFormat m_init_data_format;
    unsigned int m_init_sample_rate;
    unsigned int m_init_encoded_sample_rate;
    CAudChInfo m_init_channel_layout;
    unsigned int m_ch_layout_count;
  
    typedef struct {
        CAudBuffer data;
    } PPacket;

    AudFormat m_format;

    double m_internal_ratio;
    bool m_resample;
    CAudRemap m_remap;
    bool m_convert;
    float *m_convert_buffer;
    bool m_valid;
    bool m_delete;
    float m_volume;
    unsigned int m_water_level;
    unsigned int m_refill_buffer;

    CAudConvert::AudConvertToFn m_convert_fn;

    CAudBuffer m_input_buffer;
    unsigned int m_bytes_per_sample;
    unsigned int m_bytes_per_frame;
    unsigned int m_samples_per_frame;
    SRC_STATE *m_ssrc;
    SRC_DATA m_ssrc_data;
    CAudChInfo m_rnd_channel_layout;
    unsigned int m_rnd_bytes_per_frame;
    unsigned int m_frames_buffered;

    std::list<PPacket*> m_out_buffer;
    PPacket *m_new_packet;
    PPacket *m_packet;
    uint8_t *m_packet_pos;

    bool m_paused;
    bool m_auto_start;
    bool m_draining;
};

}
