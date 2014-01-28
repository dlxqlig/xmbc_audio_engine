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
#include <set>
#include <cmath>
#include <sstream>

#include "AudSinkALSA.h"
#include "AudUtil.h"
#include "Logger.h"
#include "SystemClock.h"

namespace AudRender {

#define CMN_MSG ((std::string("CAudSinkALSA::")+ std::string(__FUNCTION__)).c_str())

#define ALSA_OPTIONS (SND_PCM_NONBLOCK | SND_PCM_NO_AUTO_FORMAT | SND_PCM_NO_AUTO_CHANNELS | SND_PCM_NO_AUTO_RESAMPLE)
#define ALSA_MAX_CHANNELS 16
static enum AudChannel ALSAChannelMap[ALSA_MAX_CHANNELS + 1] = {
    CH_FL,
    CH_FR,
    CH_BL,
    CH_BR,
    CH_FC,
    CH_LFE,
    CH_SL,
    CH_SR,
    CH_NULL
};

static unsigned int ALSASampleRateList[] =
{
    5512,
    8000,
    11025,
    16000,
    22050,
    32000,
    44100,
    48000,
    64000,
    88200,
    96000,
    176400,
    192000,
    384000,
    0
};

CAudSinkALSA::CAudSinkALSA() :
    m_buffer_size(0),
    m_format_sample_rate_mul(0.0),
    m_passthrough(false),
    m_pcm(NULL),
    m_timeout(0)
{
    if (!snd_config) {
        snd_config_update();
    }
}

CAudSinkALSA::~CAudSinkALSA()
{
    Deinitialize();
}

inline CAudChInfo CAudSinkALSA::GetChannelLayout(AudFormat format)
{
    unsigned int count = 0;

    if (format.m_data_format == AUD_FMT_AC3 ||
            format.m_data_format == AUD_FMT_DTS ||
            format.m_data_format == AUD_FMT_EAC3) {

        count = 2;
    }
    else if (format.m_data_format == AUD_FMT_TRUEHD ||
            format.m_data_format == AUD_FMT_DTSHD) {

        count = 8;
    }
    else {

        for (unsigned int c = 0; c < 8; ++c) {

            for (unsigned int i = 0; i < format.m_channel_layout.Count(); ++i) {

                if (format.m_channel_layout[i] == ALSAChannelMap[c]) {
                    count = c + 1;
                    break;
                }
            }
        }
    }

    CAudChInfo info;
    for (unsigned int i = 0; i < count; ++i) {
        info += ALSAChannelMap[i];
    }

    return info;
}

void CAudSinkALSA::GetAudSParams(AudFormat format, std::string& params)
{

    if (m_passthrough) {
        params = "AudS0=0x06";
    }
    else {
        params = "AudS0=0x04";
    }
    params += ",AudS1=0x82,AudS2=0x00";

        if (format.m_sample_rate == 192000) params += ",AudS3=0x0e";
    else if (format.m_sample_rate == 176400) params += ",AudS3=0x0c";
    else if (format.m_sample_rate ==  96000) params += ",AudS3=0x0a";
    else if (format.m_sample_rate ==  88200) params += ",AudS3=0x08";
    else if (format.m_sample_rate ==  48000) params += ",AudS3=0x02";
    else if (format.m_sample_rate ==  44100) params += ",AudS3=0x00";
    else if (format.m_sample_rate ==  32000) params += ",AudS3=0x03";
    else params += ",AudS3=0x01";

}

bool CAudSinkALSA::Initialize(AudFormat &format, std::string &device)
{

    EntryLogger entry(__FUNCTION__);

    m_init_device = device;
    m_init_format = format;

    if (AUD_IS_RAW(format.m_data_format)) {
        m_channel_layout = GetChannelLayout(format);
        format.m_data_format = AUD_FMT_S16NE;
        m_passthrough = true;
    }
    else {
        m_channel_layout = GetChannelLayout(format);
        m_passthrough = false;
    }

    if (m_channel_layout.Count() == 0) {
        Logger::LogOut(LOG_LEVEL_ERROR, "%s failed to open the requested channel layout", CMN_MSG);
        return false;
    }

    format.m_channel_layout = m_channel_layout;
    AudDeviceType devType = AudDeviceTypeFromName(device);
    std::string AudSParams;
    // TODO:
    // digital interfaces should have AudSx set, though in practice most
    // receivers don't care
    if (m_passthrough || devType == AUD_DEVTYPE_HDMI || devType == AUD_DEVTYPE_IEC958)
        GetAudSParams(format, AudSParams);

    Logger::LogOut(LOG_LEVEL_INFO, "[%s] try to open device[%s]", CMN_MSG, device.c_str());

    snd_config_t *config;
    snd_config_copy(&config, snd_config);
    if (!OpenPCMDevice(device, AudSParams, m_channel_layout.Count(), &m_pcm, config)) {
        Logger::LogOut(LOG_LEVEL_ERROR, "[%s] failed to initialize device %s", CMN_MSG, device.c_str());
        snd_config_delete(config);
        return false;
    }

    device = snd_pcm_name(m_pcm);
    m_device = device;
    snd_config_delete(config);

    if (!InitializeHW(format) || !InitializeSW(format))
        return false;

    snd_pcm_nonblock(m_pcm, 1);
    snd_pcm_prepare (m_pcm);

    Logger::LogOut(LOG_LEVEL_ERROR, "%s Opened device %s", CMN_MSG, device.c_str());

    m_format = format;
    m_format_sample_rate_mul = 1.0 / (double)m_format.m_sample_rate;

    return true;
}

bool CAudSinkALSA::IsCompatible(const AudFormat format, const std::string &device)
{
    return
        ((m_init_format.m_sample_rate == format.m_sample_rate || m_format.m_sample_rate == format.m_sample_rate) &&
        (m_init_format.m_data_format == format.m_data_format || m_format.m_data_format == format.m_data_format) &&
        (m_init_format.m_channel_layout == format.m_channel_layout || m_format.m_channel_layout == format.m_channel_layout) &&
        (m_init_device == device));
}

snd_pcm_format_t CAudSinkALSA::AudFormatToALSAFormat(const enum AudDataFormat format)
{
    if (AUD_IS_RAW(format))
        return SND_PCM_FORMAT_S16;

    switch (format) {
        case AUD_FMT_S8:
            return SND_PCM_FORMAT_S8;
        case AUD_FMT_U8:
            return SND_PCM_FORMAT_U8;
        case AUD_FMT_S16NE:
            return SND_PCM_FORMAT_S16;
        case AUD_FMT_S16LE:
            return SND_PCM_FORMAT_S16_LE;
        case AUD_FMT_S16BE:
            return SND_PCM_FORMAT_S16_BE;
        case AUD_FMT_S24NE4:
            return SND_PCM_FORMAT_S24;
#ifdef __BIG_ENDIAN__
        case AUD_FMT_S24NE3:
            return SND_PCM_FORMAT_S24_3BE;
#else
        case AUD_FMT_S24NE3:
            return SND_PCM_FORMAT_S24_3LE;
#endif
        case AUD_FMT_S32NE:
            return SND_PCM_FORMAT_S32;
        case AUD_FMT_FLOAT:
            return SND_PCM_FORMAT_FLOAT;

        default:
            return SND_PCM_FORMAT_UNKNOWN;
    }

}

bool CAudSinkALSA::InitializeHW(AudFormat &format)
{
    snd_pcm_hw_params_t *hw_params;

    snd_pcm_hw_params_alloca(&hw_params);
    memset(hw_params, 0, snd_pcm_hw_params_sizeof());

    snd_pcm_hw_params_any(m_pcm, hw_params);
    snd_pcm_hw_params_set_access(m_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);

    unsigned int sampleRate   = format.m_sample_rate;
    unsigned int channelCount = format.m_channel_layout.Count();
    snd_pcm_hw_params_set_rate_near    (m_pcm, hw_params, &sampleRate, NULL);
    snd_pcm_hw_params_set_channels_near(m_pcm, hw_params, &channelCount);

    if (format.m_channel_layout.Count() > channelCount) {
        Logger::LogOut(LOG_LEVEL_INFO, "%s Unable to open the required number of channels", CMN_MSG);
    }

    // update the channelLayout to what we managed to open
    format.m_channel_layout.Reset();
    for (unsigned int i = 0; i < channelCount; ++i) {
        format.m_channel_layout += ALSAChannelMap[i];
    }

    snd_pcm_format_t fmt = AudFormatToALSAFormat(format.m_data_format);
    if (fmt == SND_PCM_FORMAT_UNKNOWN) {
        format.m_data_format = AUD_FMT_FLOAT;
        fmt = SND_PCM_FORMAT_FLOAT;
    }

    // try the data format
    if (snd_pcm_hw_params_set_format(m_pcm, hw_params, fmt) < 0) {
        // if the chosen format is not supported, try each one in decending order
        Logger::LogOut(LOG_LEVEL_INFO, "%s Your hardware does not support %s, trying other formats",
                CMN_MSG, CAudUtil::DataFormatToStr(format.m_data_format));

        for (enum AudDataFormat i = AUD_FMT_MAX; i > AUD_FMT_INVALID; i = (enum AudDataFormat)((int)i - 1)) {
            if (AUD_IS_RAW(i) || i == AUD_FMT_MAX) {
                continue;
            }

            if (m_passthrough && i != AUD_FMT_S16BE && i != AUD_FMT_S16LE) {
                continue;
            }

            fmt = AudFormatToALSAFormat(i);
            if (fmt == SND_PCM_FORMAT_UNKNOWN || snd_pcm_hw_params_set_format(m_pcm, hw_params, fmt) < 0) {
                fmt = SND_PCM_FORMAT_UNKNOWN;
                continue;
            }

            int fmtBits = CAudUtil::DataFormatToBits(i);
            int bits = snd_pcm_hw_params_get_sbits(hw_params);
            if (bits != fmtBits) {
                // if we opened in 32bit and only have 24bits, pack into 24
                if (fmtBits == 32 && bits == 24) {
                    i = AUD_FMT_S24NE4;
                }
                else {
                    continue;
                }
            }

            //record that the format fell back to X
            format.m_data_format = i;
            Logger::LogOut(LOG_LEVEL_INFO, "%s Using data format %s", CMN_MSG, CAudUtil::DataFormatToStr(format.m_data_format));
            break;
        }

        if (fmt == SND_PCM_FORMAT_UNKNOWN) {
            Logger::LogOut(LOG_LEVEL_ERROR, "%s Unable to find a suitable output format", CMN_MSG);
            return false;
        }
    }

    snd_pcm_uframes_t periodSize, bufferSize;
    snd_pcm_hw_params_get_buffer_size_max(hw_params, &bufferSize);
    snd_pcm_hw_params_get_period_size_max(hw_params, &periodSize, NULL);

    // 200 ms buffer size and 50ms period size.
    periodSize = std::min(periodSize, (snd_pcm_uframes_t) sampleRate / 20);
    bufferSize = std::min(bufferSize, (snd_pcm_uframes_t) sampleRate / 5);
    periodSize = std::min(periodSize, bufferSize / 4);

    Logger::LogOut(LOG_LEVEL_DEBUG, "%s Request: periodSize %lu, bufferSize %lu", CMN_MSG, periodSize, bufferSize);

    snd_pcm_hw_params_t *hw_params_copy;
    snd_pcm_hw_params_alloca(&hw_params_copy);
    snd_pcm_hw_params_copy(hw_params_copy, hw_params); // copy what we have and is already working

    // backup periodSize and bufferSize first, and restore them after every failed try
    snd_pcm_uframes_t periodSizeTemp, bufferSizeTemp;
    periodSizeTemp = periodSize;
    bufferSizeTemp = bufferSize;
    if (snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
            || snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0
            || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0) {

        bufferSize = bufferSizeTemp;
        periodSize = periodSizeTemp;
        // retry with PeriodSize, bufferSize
        snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restore working copy
        if (snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0
                || snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
                || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0) {

            // try only periodSize
            periodSize = periodSizeTemp;
            snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restore working copy
            if(snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0
                    || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0) {

                // try only BufferSize
                bufferSize = bufferSizeTemp;
                snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restory working copy
                if (snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
                        || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0) {

                    // set default that Alsa would choose
                    Logger::LogOut(LOG_LEVEL_ERROR, "%s using default alsa values - set failed", CMN_MSG);
                    if (snd_pcm_hw_params(m_pcm, hw_params) != 0) {
                        Logger::LogOut(LOG_LEVEL_DEBUG, "%s could not init a valid sink", CMN_MSG);
                        return false;
                    }
                }
            }

            // reread values when alsa default was kept
            snd_pcm_get_params(m_pcm, &bufferSize, &periodSize);
        }
    }

    Logger::LogOut(LOG_LEVEL_DEBUG, "%s Got: periodSize %lu, bufferSize %lu", CMN_MSG, periodSize, bufferSize);

    format.m_sample_rate = sampleRate;
    format.m_frames = periodSize;
    format.m_frame_samples = periodSize * format.m_channel_layout.Count();
    format.m_frame_size = snd_pcm_frames_to_bytes(m_pcm, 1);

    m_buffer_size = (unsigned int)bufferSize;
    m_timeout = std::ceil((double)(bufferSize * 1000) / (double)sampleRate);

    Logger::LogOut(LOG_LEVEL_DEBUG, "%s Setting timeout to %d ms", CMN_MSG, m_timeout);

    return true;
}

bool CAudSinkALSA::InitializeSW(AudFormat &format)
{
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_uframes_t boundary;

    snd_pcm_sw_params_alloca(&sw_params);
    memset(sw_params, 0, snd_pcm_sw_params_sizeof());

    snd_pcm_sw_params_current(m_pcm, sw_params);
    snd_pcm_sw_params_set_start_threshold(m_pcm, sw_params, INT_MAX);
    snd_pcm_sw_params_set_silence_threshold(m_pcm, sw_params, 0);
    snd_pcm_sw_params_get_boundary(sw_params, &boundary);
    snd_pcm_sw_params_set_silence_size(m_pcm, sw_params, boundary);
    snd_pcm_sw_params_set_avail_min(m_pcm, sw_params, format.m_frames);

    if (snd_pcm_sw_params(m_pcm, sw_params) < 0) {
        Logger::LogOut(LOG_LEVEL_ERROR, "%s failed to set the parameters", CMN_MSG);
        return false;
    }

    return true;
}

void CAudSinkALSA::Deinitialize()
{
    Stop();

    if (m_pcm) {
        snd_pcm_close(m_pcm);
        m_pcm = NULL;
    }
}

void CAudSinkALSA::Stop()
{
    if (!m_pcm)
        return;
    snd_pcm_drop(m_pcm);
}

double CAudSinkALSA::GetDelay()
{

    if (!m_pcm) {
        return 0;
    }

    snd_pcm_sframes_t frames = 0;
    snd_pcm_delay(m_pcm, &frames);

    if (frames < 0) {
        frames = 0;
    }

    return (double)frames * m_format_sample_rate_mul;
}

double CAudSinkALSA::GetCacheTime()
{
    if (!m_pcm) {
        return 0.0;
    }

    int space = snd_pcm_avail_update(m_pcm);
    if (space == 0) {
        snd_pcm_state_t state = snd_pcm_state(m_pcm);
        if (state < 0) {
            HandleError("snd_pcm_state", state);
            space = m_buffer_size;
        }

    if (state != SND_PCM_STATE_RUNNING && state != SND_PCM_STATE_PREPARED)
            space = m_buffer_size;
    }

    return (double)(m_buffer_size - space) * m_format_sample_rate_mul;

}

double CAudSinkALSA::GetCacheTotal()
{
    return (double)m_buffer_size * m_format_sample_rate_mul;
}

unsigned int CAudSinkALSA::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio)
{
    EntryLogger entry(__FUNCTION__);

    if (!m_pcm) {
        SoftResume();
        if(!m_pcm) {
            return 0;
        }
    }

    int ret;
    ret = snd_pcm_avail(m_pcm);
    if (ret < 0) {
        HandleError("snd_pcm_avail", ret);
        ret = 0;
    }

    if ((unsigned int)ret < frames) {
        return 0;
    }

    ret = snd_pcm_writei(m_pcm, (void*)data, frames);
    if (ret < 0) {
        HandleError("snd_pcm_writei(1)", ret);
        ret = snd_pcm_writei(m_pcm, (void*)data, frames);
        if (ret < 0) {
            HandleError("snd_pcm_writei(2)", ret);
            ret = 0;
        }
    }
    //Logger::LogOut(LOG_LEVEL_DEBUG, "CAudSinkALSA::AddPackets frames[%d] snd_pcm_writei[%d].", frames, ret);

    if ( ret > 0 && snd_pcm_state(m_pcm) == SND_PCM_STATE_PREPARED) {
        snd_pcm_start(m_pcm);
    }

    return ret;
}

void CAudSinkALSA::HandleError(const char* name, int err)
{

    switch(err) {
    case -EPIPE:
        Logger::LogOut(LOG_LEVEL_ERROR, "%s(%s) - underrun", CMN_MSG, name);
        if ((err = snd_pcm_prepare(m_pcm)) < 0)
            Logger::LogOut(LOG_LEVEL_ERROR, "%s(%s) - snd_pcm_prepare returned %d (%s)", CMN_MSG, name, err, snd_strerror(err));
        break;

    case -ESTRPIPE:
        Logger::LogOut(LOG_LEVEL_INFO, "%s(%s) - Resuming after suspend", CMN_MSG, name);

        // resume the stream again
        while((err = snd_pcm_resume(m_pcm)) == -EAGAIN)
            Sleep(10*1000);

        // if the hardware doesnt support resume, prepare the stream */
        if (err == -ENOSYS)
            if ((err = snd_pcm_prepare(m_pcm)) < 0)
                Logger::LogOut(LOG_LEVEL_ERROR, "%s(%s) - snd_pcm_prepare returned %d (%s)", CMN_MSG, name, err, snd_strerror(err));
        break;

    default:
        Logger::LogOut(LOG_LEVEL_ERROR, "%s(%s) - snd_pcm_writei returned %d (%s)",  CMN_MSG, name, err, snd_strerror(err));
        break;
  }

}

void CAudSinkALSA::Drain()
{

    if (!m_pcm)
        return;

    snd_pcm_nonblock(m_pcm, 0);
    snd_pcm_drain(m_pcm);
    snd_pcm_nonblock(m_pcm, 1);

}

void CAudSinkALSA::AppendParams(std::string &device, const std::string &params)
{
    device += (device.find(':') == std::string::npos) ? ':' : ',';
    device += params;
}

bool CAudSinkALSA::TryDevice(const std::string &name, snd_pcm_t **pcmp, snd_config_t *lconf)
{
    if (*pcmp) {
        if (name == snd_pcm_name(*pcmp))
            return true;

        snd_pcm_close(*pcmp);
        *pcmp = NULL;
    }

    int err = snd_pcm_open_lconf(pcmp, name.c_str(), SND_PCM_STREAM_PLAYBACK, ALSA_OPTIONS, lconf);
    if (err < 0) {
        Logger::LogOut(LOG_LEVEL_INFO, "%s Unable to open device %s for playback", CMN_MSG, name.c_str());
    }

    return (err == 0);

}

bool CAudSinkALSA::TryDeviceWithParams(const std::string &name, const std::string &params,
        snd_pcm_t **pcmp, snd_config_t *lconf)
{

    if (!params.empty()) {
        std::string nameWithParams = name;
        AppendParams(nameWithParams, params);
        if (TryDevice(nameWithParams, pcmp, lconf)) {
            return true;
        }
    }

    return TryDevice(name, pcmp, lconf);
}

bool CAudSinkALSA::OpenPCMDevice(const std::string &name, const std::string &params,
        int channels, snd_pcm_t **pcmp, snd_config_t *lconf)
{

    Logger::LogOut(LOG_LEVEL_INFO, "%s name:%s params:%s channels:%d", CMN_MSG, name.c_str(), params.c_str(), channels);

    // Special name denoting surroundXX mangling. This is needed for some
    // devices for multichannel to work.
    if (name == "@" || name.substr(0, 2) == "@:") {
        std::string openName = name.substr(1);

        // These device names allow alsa-lib to perform special routing if needed
        // for multichannel to work with the audio hardware.
        // Fall through in switch() so that devices with more channels are
        // added as fallback.
        switch (channels)
        {
        case 3:
        case 4:
            if (TryDeviceWithParams("surround40" + openName, params, pcmp, lconf))
                return true;
        case 5:
        case 6:
            if (TryDeviceWithParams("surround51" + openName, params, pcmp, lconf))
                return true;
        case 7:
        case 8:
            if (TryDeviceWithParams("surround71" + openName, params, pcmp, lconf))
                return true;
        }

        // Try "sysdefault" and "default" (they provide dmix if needed, and route
        // audio to all extra channels on subdeviced cards),
        // unless the selected devices is not DEV=0 of the card, in which case
        // "sysdefault" and "default" would point to another device.
        // "sysdefault" is a newish device name that won't be overwritten in case
        // system configuration redefines "default". "default" is still tried
        // because "sysdefault" is rather new.
        size_t devPos = openName.find(",DEV=");
        if (devPos == std::string::npos || (devPos + 5 < openName.size() && openName[devPos+5] == '0')) {
            // "sysdefault" and "default" do not have "DEV=0", drop it
            std::string nameWithoutDev = openName;
            if (devPos != std::string::npos) {
                nameWithoutDev.erase(nameWithoutDev.begin() + devPos, nameWithoutDev.begin() + devPos + 6);
            }

            if (TryDeviceWithParams("sysdefault" + nameWithoutDev, params, pcmp, lconf)
                    || TryDeviceWithParams("default" + nameWithoutDev, params, pcmp, lconf)) {
                return true;
            }
        }

        // Try "front" (no dmix, no audio in other channels on subdeviced cards)
        if (TryDeviceWithParams("front" + openName, params, pcmp, lconf)) {
            return true;
        }

    }
   else {
        // Non-surroundXX device, just add it
        if (TryDeviceWithParams(name, params, pcmp, lconf)) {
         return true;
        }
   }

   return false;

}

void CAudSinkALSA::EnumerateDevicesEx(AudDevInfoList &list, bool force)
{

   // ensure ALSA lib has been initialized
   snd_lib_error_set_handler(sndLibErrorHandler);
   if(!snd_config || force) {
      if(force) {
         snd_config_update_free_global();
      }

      snd_config_update();
   }

   snd_config_t *config;
   snd_config_copy(&config, snd_config);

   // Always enumerate the default device.
   // Note: If "default" is a stereo device, EnumerateDevice()
   // will automatically add "@" instead to enable surroundXX mangling.
   // We don't want to do that if "default" can handle multichannel
   // itself (e.g. in case of a pulseaudio server).
   EnumerateDevice(list, "default", "", config);

   void **hints;

   if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
      Logger::LogOut(LOG_LEVEL_INFO, "%s Unable to get a list of devices", CMN_MSG);
      return;
   }

   std::string defaultDescription;
   for (void** hint = hints; *hint != NULL; ++hint) {

      char *io = snd_device_name_get_hint(*hint, "IOID");
      char *name = snd_device_name_get_hint(*hint, "NAME");
      char *desc = snd_device_name_get_hint(*hint, "DESC");

      if ((!io || strcmp(io, "Output") == 0) && name
            && strcmp(name, "null") != 0) {

         std::string baseName = std::string(name);
         baseName = baseName.substr(0, baseName.find(':'));

            if (strcmp(name, "default") == 0) {
                // added already, but lets get the description if we have one
                if (desc)
                    defaultDescription = desc;
            }
            else if (baseName == "front") {
                // Enumerate using the surroundXX mangling
                // do not enumerate basic "front", it is already handled
                //  by the default "@" entry added in the very beginning
                if (strcmp(name, "front") != 0)
                    EnumerateDevice(list, std::string("@") + (name+5), desc ? desc : name, config);
            }

            /* Do not enumerate "default", it is already enumerated above. */

            /* Do not enumerate the sysdefault or surroundXX devices, those are
            * always accompanied with a "front" device and it is handled above
            * as "@". The below devices will be automatically used if available
            * for a "@" device. */

            /* Ubuntu has patched their alsa-lib so that "defaults.namehint.extended"
            * defaults to "on" instead of upstream "off", causing lots of unwanted
            * extra devices (many of which are not actually routed properly) to be
            * found by the enumeration process. Skip them as well ("hw", "dmix",
            * "plughw", "dsnoop"). */

            else if (baseName != "default"
                && baseName != "sysdefault"
                && baseName != "surround40"
                && baseName != "surround41"
                && baseName != "surround50"
                && baseName != "surround51"
                && baseName != "surround71"
                && baseName != "hw"
                && baseName != "dmix"
                && baseName != "plughw"
                && baseName != "dsnoop") {

                EnumerateDevice(list, name, desc ? desc : name, config);
            }
        }
        free(io);
        free(name);
        free(desc);
    }
    snd_device_name_free_hint(hints);

    // set the displayname for default device
    if (!list.empty() && list[0].m_device_name == "default") {

        // If we have one from a hint (DESC), use it
        if (!defaultDescription.empty()) {
            list[0].m_display_name = defaultDescription;
        // Otherwise use the discovered name or (unlikely) "Default"
        }
        else if (list[0].m_display_name.empty()) {
            list[0].m_display_name = "Default";
        }
    }

    /* lets check uniqueness, we may need to append DEV or CARD to DisplayName */
    /* If even a single device of card/dev X clashes with Y, add suffixes to
     * all devices of both them, for clarity. */

    /* clashing card names, e.g. "NVidia", "NVidia_2" */
    std::set<std::string> cardsToAppend;

    /* clashing basename + cardname combinations, e.g. ("hdmi","Nvidia") */
    std::set<std::pair<std::string, std::string> > devsToAppend;

    for (AudDevInfoList::iterator it1 = list.begin(); it1 != list.end(); ++it1) {

        for (AudDevInfoList::iterator it2 = it1+1; it2 != list.end(); ++it2) {

            if (it1->m_display_name == it2->m_display_name
                    && it1->m_display_name_extra == it2->m_display_name_extra) {
                /* something needs to be done */
                std::string cardString1 = GetParamFromName(it1->m_device_name, "CARD");
                std::string cardString2 = GetParamFromName(it2->m_device_name, "CARD");

                if (cardString1 != cardString2) {
                    /* card name differs, add identifiers to all devices */
                    cardsToAppend.insert(cardString1);
                    cardsToAppend.insert(cardString2);
                    continue;
                }

                std::string devString1 = GetParamFromName(it1->m_device_name, "DEV");
                std::string devString2 = GetParamFromName(it2->m_device_name, "DEV");

                if (devString1 != devString2) {
                    /* device number differs, add identifiers to all such devices */
                    devsToAppend.insert(std::make_pair(it1->m_device_name.substr(0, it1->m_device_name.find(':')), cardString1));
                    devsToAppend.insert(std::make_pair(it2->m_device_name.substr(0, it2->m_device_name.find(':')), cardString2));
                    continue;
                }

                /* if we got here, the configuration is really weird, just append the whole device string */
                it1->m_display_name += " (" + it1->m_device_name + ")";
                it2->m_display_name += " (" + it2->m_device_name + ")";
            }
        }
    }

    for (std::set<std::string>::iterator it = cardsToAppend.begin();
            it != cardsToAppend.end(); ++it) {

        for (AudDevInfoList::iterator itl = list.begin(); itl != list.end(); ++itl) {
            std::string cardString = GetParamFromName(itl->m_device_name, "CARD");
            if (cardString == *it)
                /* "HDA NVidia (NVidia)", "HDA NVidia (NVidia_2)", ... */
                itl->m_display_name += " (" + cardString + ")";
        }
    }

    for (std::set<std::pair<std::string, std::string> >::iterator it = devsToAppend.begin();
            it != devsToAppend.end(); ++it) {

        for (AudDevInfoList::iterator itl = list.begin(); itl != list.end(); ++itl) {

            std::string baseName = itl->m_device_name.substr(0, itl->m_device_name.find(':'));
            std::string cardString = GetParamFromName(itl->m_device_name, "CARD");
            if (baseName == it->first && cardString == it->second) {

                std::string devString = GetParamFromName(itl->m_device_name, "DEV");
                /* "HDMI #0", "HDMI #1" ... */
                itl->m_display_name_extra += " #" + devString;
            }
        }
    }
}

AudDeviceType CAudSinkALSA::AudDeviceTypeFromName(const std::string &name)
{
    if (name.substr(0, 4) == "hdmi")
        return AUD_DEVTYPE_HDMI;
    else if (name.substr(0, 6) == "iec958" || name.substr(0, 5) == "spdif")
        return AUD_DEVTYPE_IEC958;

    return AUD_DEVTYPE_PCM;
}

std::string CAudSinkALSA::GetParamFromName(const std::string &name, const std::string &param)
{
    // name = "hdmi:CARD=x,DEV=y" param = "CARD" => return "x"
    size_t parPos = name.find(param + '=');
    if (parPos != std::string::npos) {
        parPos += param.size() + 1;
        return name.substr(parPos, name.find_first_of(",'\"", parPos)-parPos);
    }

    return "";
}

void CAudSinkALSA::EnumerateDevice(AudDevInfoList &list, const std::string &device, const std::string &description, snd_config_t *config)
{
    snd_pcm_t *pcmhandle = NULL;
    if (!OpenPCMDevice(device, "", ALSA_MAX_CHANNELS, &pcmhandle, config))
        return;

    snd_pcm_info_t *pcminfo;
    snd_pcm_info_alloca(&pcminfo);
    memset(pcminfo, 0, snd_pcm_info_sizeof());

    int err = snd_pcm_info(pcmhandle, pcminfo);
    if (err < 0) {
        Logger::LogOut(LOG_LEVEL_INFO, "%s Unable to get pcm_info for \"%s\"", CMN_MSG, device.c_str());
        snd_pcm_close(pcmhandle);
    }

    int cardNr = snd_pcm_info_get_card(pcminfo);

    CAudDevInfo info;
    info.m_device_name = device;
    info.m_device_type = AudDeviceTypeFromName(device);

    if (cardNr >= 0) {

        /* "HDA NVidia", "HDA Intel", "HDA ATI HDMI", "SB Live! 24-bit External", ... */
        char *cardName;
        if (snd_card_get_name(cardNr, &cardName) == 0)
            info.m_display_name = cardName;

        if (info.m_device_type == AUD_DEVTYPE_HDMI && info.m_display_name.size() > 5 &&
                info.m_display_name.substr(info.m_display_name.size()-5) == " HDMI") {
            /* We already know this is HDMI, strip it */
            info.m_display_name.erase(info.m_display_name.size()-5);
        }

        /* "CONEXANT Analog", "USB Audio", "HDMI 0", "ALC889 Digital" ... */
        std::string pcminfoName = snd_pcm_info_get_name(pcminfo);

        /*
         * Filter "USB Audio", in those cases snd_card_get_name() is more
         * meaningful already
         */
        if (pcminfoName != "USB Audio")
            info.m_display_name_extra = pcminfoName;

        if (info.m_device_type == AUD_DEVTYPE_HDMI) {
            /* replace, this was likely "HDMI 0" */
            info.m_display_name_extra = "HDMI";

            int dev = snd_pcm_info_get_device(pcminfo);
            if (dev >= 0) {
                /* lets see if we can get ELD info */

                snd_ctl_t *ctlhandle;
                std::stringstream sstr;
                sstr << "hw:" << cardNr;
                std::string strHwName = sstr.str();

                if (snd_ctl_open_lconf(&ctlhandle, strHwName.c_str(), 0, config) == 0) {
                    snd_hctl_t *hctl;
                    if (snd_hctl_open_ctl(&hctl, ctlhandle) == 0) {
                        snd_hctl_load(hctl);
                        bool badHDMI = false;
                        /* snd_hctl_close also closes ctlhandle */
                        snd_hctl_close(hctl);

                        if (badHDMI) {
                            Logger::LogOut(LOG_LEVEL_DEBUG, "%s HDMI device \"%s\" may be unconnected (no ELD data)", CMN_MSG, device.c_str());
                        }

                    }
                    else {
                        snd_ctl_close(ctlhandle);
                    }
                }
            }
        }
        else if (info.m_device_type == AUD_DEVTYPE_IEC958) {
            /* append instead of replace, pcminfoName is useful for S/PDIF */
            if (!info.m_display_name_extra.empty())
                info.m_display_name_extra += ' ';
            info.m_display_name_extra += "S/PDIF";
        }
        else if (info.m_display_name_extra.empty()) {
            /* for USB audio, it gets a bit confusing as there is
             * - "SB Live! 24-bit External"
             * - "SB Live! 24-bit External, S/PDIF"
             * so add "Analog" qualifier to the first one */
            info.m_display_name_extra = "Analog";
        }

        /* "default" is a device that will be used for all inputs, while
         * "@" will be mangled to front/default/surroundXX as necessary */
        if (device == "@" || device == "default") {
            /* Make it "Default (whatever)" */
            info.m_display_name = "Default (" + info.m_display_name + (info.m_display_name_extra.empty() ? "" : " " + info.m_display_name_extra + ")");
            info.m_display_name_extra = "";
        }
    }
    else {
        /* virtual devices: "default", "pulse", ... */
        /* description can be e.g. "PulseAudio Sound Server" - for hw devices it is
         * normally uninteresting, like "HDMI Audio Output" or "Default Audio Device",
         * so we only use it for virtual devices that have no better display name */
        info.m_display_name = description;
    }

    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);
    memset(hwparams, 0, snd_pcm_hw_params_sizeof());

    /* ensure we can get a playback configuration for the device */
    if (snd_pcm_hw_params_any(pcmhandle, hwparams) < 0) {
        Logger::LogOut(LOG_LEVEL_INFO, "%s No playback configurations available for device \"%s\"", CMN_MSG, device.c_str());
        snd_pcm_close(pcmhandle);
        return;
    }

    /* detect the available sample rates */
    for (unsigned int *rate = ALSASampleRateList; *rate != 0; ++rate) {
        if (snd_pcm_hw_params_test_rate(pcmhandle, hwparams, *rate, 0) >= 0) {
            info.m_sample_rates.push_back(*rate);
        }
    }

    /* detect the channels available */
    int channels = 0;
    for (int i = ALSA_MAX_CHANNELS; i >= 1; --i) {
        /* Reopen the device if needed on the special "surroundXX" cases */
        if (info.m_device_type == AUD_DEVTYPE_PCM && (i == 8 || i == 6 || i == 4)) {
            OpenPCMDevice(device, "", i, &pcmhandle, config);
        }

        if (snd_pcm_hw_params_test_channels(pcmhandle, hwparams, i) >= 0) {
            channels = i;
            break;
        }
    }

    if (device == "default" && channels == 2) {
        /* This looks like the ALSA standard default stereo dmix device, we
         * probably want to use "@" instead to get surroundXX. */
        snd_pcm_close(pcmhandle);
        EnumerateDevice(list, "@", description, config);
        return;
    }

    CAudChInfo alsaChannels;
    for (int i = 0; i < channels; ++i) {
        if (!info.m_channels.HasChannel(ALSAChannelMap[i])) {
            info.m_channels += ALSAChannelMap[i];
        }
        alsaChannels += ALSAChannelMap[i];
    }

    /* remove the channels from m_channels that we cant use */
    info.m_channels.ResolveChannels(alsaChannels);

    /* detect the PCM sample formats that are available */
    for (enum AudDataFormat i = AUD_FMT_MAX; i > AUD_FMT_INVALID; i = (enum AudDataFormat)((int)i - 1)) {
        if (AUD_IS_RAW(i) || i == AUD_FMT_MAX) {
            continue;
        }
        snd_pcm_format_t fmt = AudFormatToALSAFormat(i);
        if (fmt == SND_PCM_FORMAT_UNKNOWN) {
            continue;
        }

        if (snd_pcm_hw_params_test_format(pcmhandle, hwparams, fmt) >= 0) {
            info.m_data_formats.push_back(i);
        }
    }

    snd_pcm_close(pcmhandle);
    list.push_back(info);
}

bool CAudSinkALSA::SoftSuspend()
{
    if(m_pcm) {
        Deinitialize();
    }

    return true;
}

bool CAudSinkALSA::SoftResume()
{
    bool ret = true;
    if(!m_pcm) {
        if (!snd_config) {
            snd_config_update();
        }

        ret = Initialize(m_init_format, m_init_device);
    }

    return ret;
}

void CAudSinkALSA::sndLibErrorHandler(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    char *errorStr;
    if (vasprintf(&errorStr, fmt, arg) >= 0) {
        Logger::LogOut(LOG_LEVEL_INFO, "%s ALSA: %s:%d:(%s) %s%s%s", CMN_MSG,
                file, line, function, errorStr, err ? ": " : "", err ? snd_strerror(err) : "");
        free(errorStr);
    }

    va_end(arg);
}

}
