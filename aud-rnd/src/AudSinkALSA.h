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
#include <alsa/asoundlib.h>

#include "AudSink.h"
#include "AudDevInfo.h"

#define ALSA_PCM_NEW_HW_PARAMS_API

namespace AudRender {

class CAudSinkALSA : public IAudSink
{
public:
    virtual const char *GetName() { return "ALSA"; }

    CAudSinkALSA();
    virtual ~CAudSinkALSA();

    virtual bool Initialize(AudFormat &format, std::string &device);
    virtual void Deinitialize();
    virtual bool IsCompatible(const AudFormat format, const std::string &device);

    virtual void Stop();
    virtual double GetDelay();
    virtual double GetCacheTime();
    virtual double GetCacheTotal();
    virtual unsigned int AddPackets(uint8_t *data, unsigned int frames, bool hasAudio);
    virtual void Drain();
    virtual bool SoftSuspend();
    virtual bool SoftResume();

    static void EnumerateDevicesEx(AudDevInfoList &list, bool force = false);

private:
    CAudChInfo GetChannelLayout(AudFormat format);
    void GetAudSParams(const AudFormat format, std::string& params);
    void HandleError(const char* name, int err);
    static snd_pcm_format_t AudFormatToALSAFormat(const enum AudDataFormat format);

    bool InitializeHW(AudFormat &format);
    bool InitializeSW(AudFormat &format);

    static void AppendParams(std::string &device, const std::string &params);
    static bool TryDevice(const std::string &name, snd_pcm_t **pcmp, snd_config_t *lconf);
    static bool TryDeviceWithParams(const std::string &name, const std::string &params, snd_pcm_t **pcmp, snd_config_t *lconf);
    static bool OpenPCMDevice(const std::string &name, const std::string &params, int channels, snd_pcm_t **pcmp, snd_config_t *lconf);
    static AudDeviceType AudDeviceTypeFromName(const std::string &name);
    static std::string GetParamFromName(const std::string &name, const std::string &param);
    static void EnumerateDevice(AudDevInfoList &list, const std::string &device, const std::string &description, snd_config_t *config);
    static bool SoundDeviceExists(const std::string& device);
    static void sndLibErrorHandler(const char *file, int line, const char *function, int err, const char *fmt, ...);

    std::string m_init_device;
    AudFormat m_init_format;
    AudFormat m_format;
    unsigned int m_buffer_size;
    double m_format_sample_rate_mul;
    bool m_passthrough;
    CAudChInfo m_channel_layout;
    std::string  m_device;
    snd_pcm_t *m_pcm;
    int m_timeout;

};

}

