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

#include <string.h>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <climits>

#include "AudSinkProfiler.h"
#include "AudSinkALSA.h"
#include "AudSinkNULL.h"
#include "AudRender.h"
#include "Logger.h"
#include "SingleLock.h"
#include "SystemClock.h"
#include "AudStream.h"
#include "AudUtil.h"

using namespace std;

namespace AudRender {

#define SOFTAUD_IDLE_WAIT_MSEC 50
#define CMN_MSG ((std::string("CAudRender::")+ std::string(__FUNCTION__)).c_str())

CAudRender::CAudRender():
    m_outputStageFn(NULL),
    m_streamStageFn(NULL),
    m_running(false),
    m_reopen(false),
    m_sink_is_suspended(false),
    m_is_suspended(false),
    m_soft_suspend(false),
    m_soft_suspend_timer(0),
    m_volume(1.0),
    m_sink(NULL),
    m_sink_block_time(0),
    m_raw_passthrough(false),
    m_streams_playing(false),
    m_converted(NULL),
    m_converted_size(0),
    m_master_stream(NULL)
{
    EntryLogger entry(__FUNCTION__);
}

CAudRender::~CAudRender()
{
    EntryLogger entry(__FUNCTION__);
    Deinitialize();

    // free streams
    CSingleLock streamLock(m_stream_lock);
    while (!m_streams.empty()) {
        CAudStream *s = m_streams.back();
        m_streams.pop_back();
        delete s;
    }
}

IAudSink *CAudRender::CreateSink(std::string &device, AudFormat &desiredFormat, bool rawPassthrough)
{
    EntryLogger entry(__FUNCTION__);

    AudFormat tmpFormat = desiredFormat;
    IAudSink *sink = NULL;
    std::string tmpDevice;
    std::string driver;

    Logger::LogOut(LOG_LEVEL_INFO, "%s device[%s]", CMN_MSG, device.c_str());

    int pos = device.find_first_of(':');
    if (pos > 0) {
        driver = device.substr(0, pos);
        std::transform(driver.begin(), driver.end(), driver.begin(), ::toupper);

        if (driver == "ALSA" || driver == "PROFILER")
            device = device.substr(pos + 1, device.length() - pos - 1);
        else
            driver.clear();
    }
    else {
        driver.clear();
    }

    if (driver == "PROFILER") {
        sink = new CAudSinkProfiler();
        if (sink && sink->Initialize (tmpFormat,  tmpDevice)) {
            desiredFormat = tmpFormat;
            device = tmpDevice;
            return sink;
        }
    }

    if (driver.empty() || driver == "ALSA") {
        sink = new CAudSinkALSA();
        tmpDevice = device;
        if (sink && sink->Initialize (tmpFormat,  tmpDevice)) {
            desiredFormat = tmpFormat;
            device = tmpDevice;
            return sink;
        }
    }

    sink = new CAudSinkNULL();
    if (sink && sink->Initialize (tmpFormat,  tmpDevice)) {
        desiredFormat = tmpFormat;
        device = tmpDevice;
        return sink;
    }

    sink->Deinitialize();
    delete sink;
    sink = NULL;

    return NULL;
}

IAudSink *CAudRender::GetSink(AudFormat &newFormat, bool passthrough, std::string &device)
{
    EntryLogger entry(__FUNCTION__);

    device = passthrough ? m_passthrough_device : m_device;

    // raw stream need fixed sample rate
    if (AUD_IS_RAW(newFormat.m_data_format)) {
        switch (newFormat.m_data_format) {
        case AUD_FMT_AC3:
        case AUD_FMT_DTS:
            break;
        case AUD_FMT_EAC3:
            newFormat.m_sample_rate = 192000;
            break;
        case AUD_FMT_TRUEHD:
        case AUD_FMT_DTSHD:
            newFormat.m_sample_rate = 192000;
            break;
        default:
            break;
        }
    }

    IAudSink *sink = CreateSink(device, newFormat, passthrough);
    return sink;
}

inline CAudStream *CAudRender::GetMasterStream()
{
    EntryLogger entry(__FUNCTION__);

    // clear destroyed streams
    for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ) {
        CAudStream *stream = *itt;
        if (stream->IsDestroyed()) {
            RemoveStream(m_playing_streams, stream);
            RemoveStream(m_streams, stream);
            delete stream;
            continue;
        }
        ++itt;
    }

    if (!m_new_streams.empty())
        return m_new_streams.back();

    if (!m_streams.empty())
        return m_streams.back();

    return NULL;
}

void CAudRender::OpenSink()
{
    EntryLogger entry(__FUNCTION__);

    m_reopen_event.Reset();
    m_reopen = true;
    // wait InternalOpenSink() or Suspend() event signaled.
    m_reopen_event.Wait();

    // signaled for ProcessSuspend() to go on process.
    m_wake.Set();
}

void CAudRender::InternalCloseSink()
{
    EntryLogger entry(__FUNCTION__);

    if (m_sink) {
        CExclusiveLock sinkLock(m_sink_lock);
        m_sink->Drain();
        m_sink->Deinitialize();
        delete m_sink;
        m_sink = NULL;
    }
}

// this must NEVER be called from outside the main thread or Initialization
void CAudRender::InternalOpenSink()
{
    EntryLogger entry(__FUNCTION__);

    bool wasRawPassthrough = m_raw_passthrough;
    bool reInit = false;

    // TODO:
    m_device = "default";
    m_passthrough_device = "default";

    m_std_ch_layout = CH_LAYOUT_2_0;

    // initialize for pcm output
    m_raw_passthrough = false;
    m_streamStageFn = &CAudRender::RunStreamStage;
    m_outputStageFn = &CAudRender::RunOutputStage;

    AudFormat newFormat;
    newFormat.m_data_format = AUD_FMT_FLOAT;
    newFormat.m_sample_rate = 44100;
    newFormat.m_encoded_rate = 0;
    newFormat.m_channel_layout = CH_LAYOUT_2_0;
    newFormat.m_frames = 0;
    newFormat.m_frame_samples = 0;
    newFormat.m_frame_size = 0;

    CSingleLock streamLock(m_stream_lock);
    m_master_stream = GetMasterStream();
    if (m_master_stream) {
        // choose the sample rate & channel layout based on the master stream
        newFormat.m_sample_rate = m_master_stream->GetSampleRate();
        newFormat.m_channel_layout = m_master_stream->GetChannelLayout();

        if (m_master_stream->IsRaw()) {
            newFormat.m_sample_rate = m_master_stream->GetSampleRate();
            newFormat.m_encoded_rate = m_master_stream->GetEncodedSampleRate();
            newFormat.m_data_format = m_master_stream->GetDataFormat();
            newFormat.m_channel_layout = m_master_stream->GetChannelLayout();
            m_raw_passthrough = true;
            m_streamStageFn = &CAudRender::RunRawStreamStage;
            m_outputStageFn = &CAudRender::RunRawOutputStage;
        }
        else {
            newFormat.m_channel_layout.ResolveChannels(m_std_ch_layout);
        }

        // paused stream cant be used as master stream.
        if (m_master_stream->m_paused)
            m_master_stream = NULL;
    }
    streamLock.Leave();

    Logger::LogOut(LOG_LEVEL_INFO, "%s device[%s], passdev[%s]", CMN_MSG, m_passthrough_device.c_str(), m_device.c_str());

    std::string device, driver;
    driver = "ALSA";

    if (m_raw_passthrough) {
        device = m_passthrough_device;
    }
    else {
        device = m_device;
    }

    if (m_raw_passthrough)
        Logger::LogOut(LOG_LEVEL_INFO, "%s RAW passthrough enabled", CMN_MSG);

    // re-open sink when sink has some changed
    std::string sinkName;
    if (m_sink) {
        sinkName = m_sink->GetName();
        std::transform(sinkName.begin(), sinkName.end(), sinkName.begin(), ::toupper);
    }

    if (!m_sink || sinkName != driver || !m_sink->IsCompatible(newFormat, device)) {
        Logger::LogOut(LOG_LEVEL_INFO, "%s sink incompatible, re-starting", CMN_MSG);

        CExclusiveLock sinkLock(m_sink_lock);
        reInit = true;
        InternalCloseSink();

        // if we already have a driver, prepend it to the device string
        if (!driver.empty())
            device = driver + ":" + device;

        m_sink = GetSink(newFormat, m_raw_passthrough, device);

        Logger::LogOut(LOG_LEVEL_DEBUG, "%s %s Initialized:", CMN_MSG, m_sink->GetName());
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Output Device : %s", m_device_friendly_name.c_str());
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Sample Rate   : %d", newFormat.m_sample_rate);
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Sample Format : %s", CAudUtil::DataFormatToStr(newFormat.m_data_format));
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Channel Count : %d", newFormat.m_channel_layout.Count());
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Channel Layout: %s", newFormat.m_channel_layout.ToChString().c_str());
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Frames        : %d", newFormat.m_frames);
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Frame Samples : %d", newFormat.m_frame_samples);
        Logger::LogOut(LOG_LEVEL_DEBUG, "  Frame Size    : %d", newFormat.m_frame_size);

        m_sink_format = newFormat;
        m_sink_format_sample_rate_mul = 1.0 / (double)newFormat.m_sample_rate;
        m_sink_format_sample_size_mul = 1.0 / (double)newFormat.m_frame_size;
        m_sink_block_size = newFormat.m_frames * newFormat.m_frame_size;
        m_sink_block_time = 1000 * newFormat.m_frames / newFormat.m_sample_rate;

        // check sink if control volume
        m_sink_handles_volume = m_sink->HasVolume();
        if (m_sink_handles_volume)
            m_sink->SetVolume(m_volume);

        m_buffer.Empty();
    }
    else {
        Logger::LogOut(LOG_LEVEL_INFO, "%s keeping old sink with : %s, %s, %dhz",
                            CMN_MSG,
                            CAudUtil::DataFormatToStr(newFormat.m_data_format),
                            newFormat.m_channel_layout.ToChString().c_str(),
                            newFormat.m_sample_rate);
    }

    reInit = (reInit || m_ch_layout != m_sink_format.m_channel_layout);
    m_ch_layout = m_sink_format.m_channel_layout;

    size_t neededBufferSize = 0;
    // we mix streams with data format float,
    // so before write to sink need to convert data format to sink format.
    // but raw data format need not convert.
    if (m_raw_passthrough) {
        if (!wasRawPassthrough)
            m_buffer.Empty();

        m_convert_fn = NULL;
        m_bytes_per_sample = CAudUtil::DataFormatToBits(m_sink_format.m_data_format) >> 3;
        m_frame_size = m_sink_format.m_frame_size;
        neededBufferSize = m_sink_format.m_frames * m_sink_format.m_frame_size;
    }
    else {
        m_convert_fn = CAudConvert::FrFloat(m_sink_format.m_data_format);
        m_bytes_per_sample = CAudUtil::DataFormatToBits(AUD_FMT_FLOAT) >> 3;
        m_frame_size = m_bytes_per_sample * m_ch_layout.Count();
        neededBufferSize = m_sink_format.m_frames * sizeof(float) * m_ch_layout.Count();
        Logger::LogOut(LOG_LEVEL_DEBUG, "%s Using speaker layout: %s", CMN_MSG, CAudUtil::GetStdChLayoutName(m_std_ch_layout));
    }

    Logger::LogOut(LOG_LEVEL_DEBUG, "%s Internal Buffer Size: %d", CMN_MSG, (int)neededBufferSize);
    if (m_buffer.Size() < neededBufferSize)
        m_buffer.Alloc(neededBufferSize);

    if (reInit) {
        streamLock.Enter();
        for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
            (*itt)->Initialize();
        streamLock.Leave();
    }

    for (StreamList::iterator itt = m_new_streams.begin(); itt != m_new_streams.end(); ++itt) {
        (*itt)->Initialize();
        m_streams.push_back(*itt);
        if (!(*itt)->m_paused)
            m_playing_streams.push_back(*itt);
    }
    m_new_streams.clear();
    m_streams_playing = !m_playing_streams.empty();

    m_soft_suspend = false;
    m_reopen = false;
    // notify to open sink ok
    m_reopen_event.Set();
    // notify to go on processing
    m_wake.Set();
}

void CAudRender::Shutdown()
{
    EntryLogger entry(__FUNCTION__);
    Deinitialize();
}

bool CAudRender::Initialize()
{
    EntryLogger entry(__FUNCTION__);

    CSingleLock lock(m_thread_lock);
    InternalOpenSink();
    m_running = true;

    // create thread for run output and stream stage.
    Create(THREAD_PRIORITY_ABOVE_NORMAL);

    return true;
}

void CAudRender::Deinitialize()
{
    EntryLogger entry(__FUNCTION__);

    CSingleLock lock(m_thread_lock);
    Stop();
    StopThread();
    lock.Leave();

    CExclusiveLock sinkLock(m_sink_lock);
    if (m_sink) {
        m_sink->Deinitialize();
        delete m_sink;
        m_sink = NULL;
    }

    m_buffer.DeAlloc();

    free_aligned(m_converted);
    m_converted = NULL;
    m_converted_size = 0;

}

bool CAudRender::SupportsRaw()
{
    return true;
}

void CAudRender::PauseStream(CAudStream *stream)
{
    EntryLogger entry(__FUNCTION__);

    CSingleLock streamLock(m_stream_lock);

    RemoveStream(m_playing_streams, stream);
    stream->m_paused = true;

    m_reopen = true;
    m_wake.Set();
}

void CAudRender::ResumeStream(CAudStream *stream)
{
    EntryLogger entry(__FUNCTION__);

    CSingleLock streamLock(m_stream_lock);
    m_playing_streams.push_back(stream);
    stream->m_paused = false;
    streamLock.Leave();

    m_streams_playing = true;
    m_reopen = true;
    m_wake.Set();
}

void CAudRender::Stop()
{
    EntryLogger entry(__FUNCTION__);

    m_running = false;
    m_is_suspended = false;
    m_wake.Set();

    // wait for the thread to stop
    CSingleLock lock(m_running_lock);
}

CAudStream *CAudRender::MakeStream(enum AudDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAudChInfo channelLayout, unsigned int options/* = 0 */)
{
    EntryLogger entry(__FUNCTION__);

    CAudChInfo channelInfo(channelLayout);
    Logger::LogOut(LOG_LEVEL_INFO, "%s %s, %u, %s", CMN_MSG,
            CAudUtil::DataFormatToStr(dataFormat),
            sampleRate, channelInfo.ToChString().c_str());

    // ensure we have the encoded sample rate if the format is RAW
    if (AUD_IS_RAW(dataFormat) && encodedSampleRate <= 0) {
        Logger::LogOut(LOG_LEVEL_ERROR, "%s - Raw stream need encoded sample rate.", CMN_MSG);
        return NULL;
    }

    CSingleLock streamLock(m_stream_lock);
    CAudStream *stream = new CAudStream(this, dataFormat, sampleRate, encodedSampleRate,
                channelLayout, options, m_stream_lock);
    m_new_streams.push_back(stream);
    streamLock.Leave();

    OpenSink();

    return stream;
}

unsigned int CAudRender::GetSampleRate()
{
    return m_sink_format.m_sample_rate;
}

CAudStream *CAudRender::FreeStream(CAudStream *stream)
{
    EntryLogger entry(__FUNCTION__);

    CSingleLock lock(m_stream_lock);
    RemoveStream(m_playing_streams, stream);
    RemoveStream(m_streams, stream);

    if(!m_is_suspended && (m_master_stream == stream)) {
        m_reopen = true;
        m_master_stream = NULL;
    }

    delete (CAudStream*)stream;
    return NULL;
}

double CAudRender::GetDelay()
{
    EntryLogger entry(__FUNCTION__);
    double delayBuffer = 0.0, delaySink = 0.0, delayTranscoder = 0.0;

    CSharedLock sinkLock(m_sink_lock);
    if (m_sink)
        delaySink = m_sink->GetDelay();
 
    delayBuffer = (double)m_buffer.Used() * m_sink_format_sample_size_mul *m_sink_format_sample_rate_mul;

    return delayBuffer + delaySink + delayTranscoder;
}

double CAudRender::GetCacheTime()
{
    double timeBuffer = 0.0, timeSink = 0.0, timeTranscoder = 0.0;

    CSharedLock sinkLock(m_sink_lock);
    if (m_sink)
        timeSink = m_sink->GetCacheTime();

    timeBuffer = (double)m_buffer.Used() * m_sink_format_sample_size_mul *m_sink_format_sample_rate_mul;

    return timeBuffer + timeSink + timeTranscoder;
}

double CAudRender::GetCacheTotal()
{
    double timeBuffer = 0.0, timeSink = 0.0, timeTranscoder = 0.0;

    CSharedLock sinkLock(m_sink_lock);
    if (m_sink)
        timeSink = m_sink->GetCacheTotal();

    timeBuffer = (double)m_buffer.Size() * m_sink_format_sample_size_mul *m_sink_format_sample_rate_mul;

    return timeBuffer + timeSink + timeTranscoder;
}

bool CAudRender::IsSuspended()
{
    return m_is_suspended;
}

float CAudRender::GetVolume()
{
    return m_volume;
}

void CAudRender::SetVolume(float volume)
{
    m_volume = volume;
    if (!m_sink_handles_volume)
        return;

    CSharedLock sinkLock(m_sink_lock);
    if (m_sink)
        m_sink->SetVolume(m_volume);
}

bool CAudRender::Suspend()
{
    EntryLogger entry(__FUNCTION__);
    Logger::LogOut(LOG_LEVEL_DEBUG, "CAudRender::Suspend - Suspending AE processing");

    m_is_suspended = true;

    CSingleLock streamLock(m_stream_lock);
    for (StreamList::iterator itt = m_playing_streams.begin(); itt != m_playing_streams.end(); ++itt) {
        CAudStream *stream = *itt;
        stream->Flush();
    }
    streamLock.Leave();

    bool ret = true;
    if (m_sink) {
        m_save_suspend.Reset();
        // wait ProcessSuspend() event signaled
        ret = m_save_suspend.WaitMSec(500);
        if(ret) {
            Logger::LogOut(LOG_LEVEL_DEBUG, "%s - Save Suspend After Event", CMN_MSG);
            CExclusiveLock sinkLock(m_sink_lock);
            InternalCloseSink();
        }
        else {
            Logger::LogOut(LOG_LEVEL_DEBUG, "%s - Unload failed will continue", CMN_MSG);
            m_save_suspend.Reset();
        }
    }

    if(m_reopen)
        m_reopen_event.Set();

    return true;
}

bool CAudRender::Resume()
{
    EntryLogger entry(__FUNCTION__);
    Logger::LogOut(LOG_LEVEL_DEBUG, "%s - Resuming Render processing", CMN_MSG);

    m_is_suspended = false;
    m_reopen = true;

    return true;
}

void CAudRender::Process()
{
    Logger::LogOut(LOG_LEVEL_INFO, "%s - Thread Started", CMN_MSG);

    CSingleLock runningLock(m_running_lock);
    bool hasAudio = false;
    while (m_running) {
        bool restart = false;

        // run output stage
        if ((this->*m_outputStageFn)(hasAudio) > 0)
            hasAudio = false; // taken some audio - reset our silence flag

        if (m_buffer.Free() >= m_frame_size) {
            uint8_t *out = (uint8_t*)m_buffer.Take(m_frame_size);
            memset(out, 0, m_frame_size);

            // run the stream stage
            CAudStream *oldMaster = m_master_stream;
            if ((this->*m_streamStageFn)(m_ch_layout.Count(), out, restart) > 0)
                hasAudio = true;

            if (oldMaster != m_master_stream)
                restart = true;
        }

        // Handle idle or forced suspend
        ProcessSuspend();

        if (m_reopen || restart || !m_sink) {
            if(m_sink_is_suspended && m_sink) {
                m_reopen = !m_sink->SoftResume() || m_reopen;
                m_sink_is_suspended = false;
                Logger::LogOut(LOG_LEVEL_DEBUG, "%s - Sink was forgotten", CMN_MSG);
            }

            Logger::LogOut(LOG_LEVEL_DEBUG, "%s - Sink restart flagged, m_reopen[%d], restart[%d]", CMN_MSG, m_reopen, restart);
        InternalOpenSink();
        }

    }
}

void CAudRender::AllocateConvIfNeeded(size_t convertedSize, bool prezero)
{
    if (m_converted_size < convertedSize) {
        free_aligned(m_converted);
        m_converted = (uint8_t *)malloc_aligned(convertedSize, 16);
        m_converted_size = convertedSize;
    }

    // zero memory means silence after write to sink
    if (prezero)
        memset(m_converted, 0x00, convertedSize);
}

bool CAudRender::FinalizeSamples(float *buffer, unsigned int samples, bool hasAudio)
{

    EntryLogger entry(__FUNCTION__);

    // no need to process if we don't have audio (buffer is memset to 0)
    if (!hasAudio)
        return false;
#if 0
    if (m_muted) {
        memset(buffer, 0, samples * sizeof(float));
        return false;
    }
#endif

    // adjust volume
    if (!m_sink_handles_volume && m_volume < 1.0) {
        float *fbuffer = buffer;
        for (unsigned int i = 0; i < samples; i++)
            *fbuffer++ *= m_volume;
    }

    // check if we need to clamp
    bool clamp = false;
    float *fbuffer = buffer;
    for (unsigned int i = 0; i < samples; i++, fbuffer++) {
        if (*fbuffer < -1.0f || *fbuffer > 1.0f) {
            clamp = true;
            break;
        }
    }

    // if there were no samples outside of the range, dont clamp the buffer
    if (!clamp)
        return true;

    Logger::LogOut(LOG_LEVEL_DEBUG, "%s - Clamping buffer of %d samples", CMN_MSG, samples);

    // TODO:
    //CAudUtil::ClampArray(buffer, samples);
    return true;
}

unsigned int CAudRender::WriteSink(CAudBuffer& src, unsigned int src_len, uint8_t *data, bool hasAudio)
{
    EntryLogger entry(__FUNCTION__);

    CExclusiveLock lock(m_sink_lock);
    Timer timeout(m_sink_block_time * 2);
    while(m_sink && src.Used() >= src_len) {
        int frames = m_sink->AddPackets(data, m_sink_format.m_frames, hasAudio);

        if (frames == INT_MAX) {
            Logger::LogOut(LOG_LEVEL_ERROR, "%s- sink error - reinit flagged", CMN_MSG);
            m_reopen = true;
            break;
        }

        if (frames) {
            src.Shift(NULL, src_len);
            return frames;
        }

        if(timeout.IsTimePast()) {
            Logger::LogOut(LOG_LEVEL_ERROR, "%s - sink blocked- reinit flagged", CMN_MSG);
            m_reopen = true;
            break;
        }

        lock.Leave();
        Sleep(m_sink_block_time / 4);
        lock.Enter();
    }

    return 0;
}

int CAudRender::RunOutputStage(bool hasAudio)
{
    const unsigned int needSamples = m_sink_format.m_frames * m_sink_format.m_channel_layout.Count();
    const size_t needBytes = needSamples * sizeof(float);
    if (m_buffer.Used() < needBytes)
        return 0;

    void *data = m_buffer.Raw(needBytes);
    hasAudio = FinalizeSamples((float*)data, needSamples, hasAudio);

    //Logger::LogOut(LOG_LEVEL_ERROR, "%s hasAudio[%d]", CMN_MSG, hasAudio);
    if (m_convert_fn) {
        const unsigned int convertedBytes = m_sink_format.m_frames * m_sink_format.m_frame_size;
        AllocateConvIfNeeded(convertedBytes, !hasAudio);
        if (hasAudio)
            m_convert_fn((float*)data, needSamples, m_converted);

        data = m_converted;
    }

    return WriteSink(m_buffer, needBytes, (uint8_t*)data, hasAudio);
}

int CAudRender::RunRawOutputStage(bool hasAudio)
{
    return 0;
    // TODO:
}

unsigned int CAudRender::RunRawStreamStage(unsigned int channelCount, void *out, bool &restart)
{
    // TODO:
    return 0;
}

unsigned int CAudRender::RunStreamStage(unsigned int channelCount, void *out, bool &restart)
{
    // no point doing anything if we have no streams,
    // we do not have to take a lock just to check empty
    if (m_playing_streams.empty())
        return 0;

    float *dst = (float*)out;
    unsigned int mixed = 0;

    // identify the master stream
    CSingleLock streamLock(m_stream_lock);

    // mix in any running streams
    StreamList resumeStreams;
    for (StreamList::iterator itt = m_playing_streams.begin(); itt != m_playing_streams.end(); ++itt) {
        CAudStream *stream = *itt;

        float *frame = (float*)stream->GetFrame();

        if (!frame)
            continue;
// TODO: stream volume adjust
        {
            for (unsigned int i = 0; i < channelCount; ++i)
                *dst++ += *frame++;
        }
        ++mixed;
    }

    return mixed;
}

inline void CAudRender::RemoveStream(StreamList &streams, CAudStream *stream)
{
    EntryLogger entry(__FUNCTION__);
    StreamList::iterator f = std::find(streams.begin(), streams.end(), stream);
    if (f != streams.end())
        streams.erase(f);

    if (streams == m_playing_streams)
        m_streams_playing = !m_playing_streams.empty();
}

inline void CAudRender::ProcessSuspend()
{
    unsigned int curSystemClock = 0;
    if (!m_soft_suspend && m_playing_streams.empty()) {
        m_soft_suspend = true;
        m_soft_suspend_timer = SystemClockMillis() + 10000; //10.0 second delay for softSuspend
        Sleep(10 *1000);
    }

    if (m_soft_suspend)
        curSystemClock = SystemClockMillis();

    // idle Suspend->Resume by user
    // idle nothing to play
    while (m_is_suspended ||
            ((m_soft_suspend && (curSystemClock > m_soft_suspend_timer)) && m_running && !m_reopen)) {

        if (!m_is_suspended && m_sink && !m_sink_is_suspended) {
            // put the sink in Suspend mode
            CExclusiveLock sinkLock(m_sink_lock);
            if (m_sink && !m_sink->SoftSuspend()) {
                m_sink_is_suspended = false; // sink cannot be suspended
                m_soft_suspend = false; // break suspend loop
                break;
            }
            else {
                Logger::LogOut(LOG_LEVEL_DEBUG, "Suspended the Sink");
                m_sink_is_suspended = true; // sink has suspended processing
            }
            sinkLock.Leave();
        }

        // signal that the Suspend can go on now.
        if(m_is_suspended)
            m_save_suspend.Set();

        m_wake.WaitMSec(SOFTAUD_IDLE_WAIT_MSEC);

        // we check reusme or has streams
        if (!m_is_suspended && (!m_playing_streams.empty())) {

            m_reopen = m_sink && (!m_sink->SoftResume() || m_reopen);
            m_sink_is_suspended = false;
            m_soft_suspend = false;
            Logger::LogOut(LOG_LEVEL_DEBUG, "%s Resumed the Sink", CMN_MSG);
            break;
        }
    }
}

}
