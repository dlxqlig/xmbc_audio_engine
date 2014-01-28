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
#include <list>
#include <vector>
#include <map>

#include "AudFormat.h"
#include "AudStream.h"
#include "AudBuffer.h"
#include "AudConvert.h"
#include "Thread.h"
#include "Event.h"
#include "SharedSection.h"

class IAudSink;

namespace AudRender{

typedef std::vector<CAudStream*> StreamList;

class CAudRender : public AThread
{

public:
    CAudRender();
    virtual ~CAudRender();

    virtual void Shutdown();
    virtual bool Initialize();

    virtual void Stop();
    virtual bool Suspend();
    virtual bool Resume();
    virtual bool IsSuspended();
    virtual double GetDelay();

    virtual float GetVolume();
    virtual void SetVolume(const float volume);
    virtual void SetMute(const bool enabled) { m_muted = enabled; }
    virtual bool IsMuted() { return m_muted; }

    virtual void Process();

    virtual CAudStream *MakeStream(enum AudDataFormat dataFormat, unsigned int sampleRate,
        unsigned int encodedSampleRate, CAudChInfo channelLayout, unsigned int options = 0);
    virtual CAudStream *FreeStream(CAudStream *stream);

    unsigned int GetSampleRate();
    unsigned int GetChannelCount() {return m_ch_layout.Count();}
    CAudChInfo& GetChannelLayout() {return m_ch_layout;}
    enum AudStdChLayout GetStdChLayout() {return m_std_ch_layout;}
    unsigned int GetFrames() {return m_sink_format.m_frames;}
    unsigned int GetFrameSize() {return m_frame_size;}

    const AudFormat* GetSinkAudioFormat() {return &m_sink_format;}
    enum AudDataFormat GetSinkDataFormat () {return m_sink_format.m_data_format;}
    CAudChInfo& GetSinkChLayout() {return m_sink_format.m_channel_layout;}
    unsigned int GetSinkFrameSize() {return m_sink_format.m_frame_size;}

    double GetCacheTime();
    double GetCacheTotal();

    virtual bool SupportsRaw();

    void PauseStream (CAudStream *stream);
    void ResumeStream(CAudStream *stream);

private:
    CAudStream *GetMasterStream();
    void OpenSink();
    void InternalOpenSink();
    void InternalCloseSink();
    void Deinitialize();

    inline void ProcessSuspend();
    IAudSink *CreateSink(std::string &device, AudFormat &desiredFormat, bool rawPassthrough);
    IAudSink *GetSink(AudFormat &desiredFormat, bool passthrough, std::string &device);

    // run output stage, write data to audio sink.
    int (CAudRender::*m_outputStageFn)(bool);
    int RunOutputStage(bool hasAudio);
    int RunRawOutputStage(bool hasAudio);
    unsigned int (CAudRender::*m_streamStageFn)(unsigned int channelCount, void *out, bool &restart);
    unsigned int RunRawStreamStage(unsigned int channelCount, void *out, bool &restart);
    unsigned int RunStreamStage(unsigned int channelCount, void *out, bool &restart);
    void RunNormalizeStage(unsigned int channelCount, void *out, unsigned int mixed);
    void RemoveStream(StreamList &streams, CAudStream *stream);
    unsigned int WriteSink(CAudBuffer& src, unsigned int src_len, uint8_t *data, bool hasAudio);

    void AllocateConvIfNeeded(size_t convertedSize, bool prezero = false);
    bool FinalizeSamples(float *buffer, unsigned int samples, bool hasAudio);

    enum AudStdChLayout m_std_ch_layout;
    std::string m_device;
    std::string m_passthrough_device;
    std::string m_device_friendly_name;

    bool m_running, m_reopen;
    bool m_sink_is_suspended; // sink is suspended
    bool m_is_suspended; // render is suspended by user
    bool m_soft_suspend; // after last stream for timer below for idle
    unsigned int m_soft_suspend_timer; // time hold sink open before soft suspend for idle
    CEvent m_reopen_event;
    CEvent m_wake;
    CEvent m_save_suspend;

    CCriticalSection m_running_lock;
    CCriticalSection m_stream_lock;
    CSharedSection m_sink_lock;
    CCriticalSection m_thread_lock;

    float m_volume;
    bool m_muted;
    CAudChInfo m_ch_layout;
    unsigned int m_frame_size;

    IAudSink *m_sink;
    AudFormat m_sink_format;
    double m_sink_format_sample_rate_mul;
    double m_sink_format_sample_size_mul;
    unsigned int m_sink_block_size;
    unsigned int m_sink_block_time;
    bool m_sink_handles_volume;

    unsigned int m_bytes_per_sample;
    CAudConvert::AudConvertFrFn m_convert_fn;

    bool m_raw_passthrough;
    StreamList m_new_streams, m_streams, m_playing_streams;
    bool m_streams_playing;

    CAudBuffer m_buffer;

    uint8_t *m_converted;
    size_t m_converted_size;

    CAudStream *m_master_stream;

};

}
