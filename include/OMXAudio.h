/*
  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies)

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

  $Date: 2009-10-09 14:34:15 +0200 (Fri, 09 Oct 2009) $
  Revision $Rev: 875 $
  Author $Author: gsent $

  You can also contact with mail(gangx.li@intel.com)
*/
#ifndef __OPENMAXAUDIORENDER_H__
#define __OPENMAXAUDIORENDER_H__

#include "OMXCore.h"
#include "AudBuffer.h"

class COMXAudio
{
public:
    int Decode(uint8_t *pData, int iSize);
    OMXPacket *GetData();
    unsigned int GetChunkLen();
    float GetDelay();
    float GetCacheTime();
    float GetCacheTotal();
    unsigned int GetAudioRenderingLatency();
    COMXAudio();
    bool Initialize(int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, long initialVolume = 0, float fifo_size = 0);
    ~COMXAudio();
    bool PortSettingsChanged();

    unsigned int AddPackets(const void* data, unsigned int len);
    unsigned int AddPackets(const void* data, unsigned int len, double dts, double pts);
    unsigned int GetSpace();
    bool Deinitialize();

    long GetCurrentVolume() const;
    void Mute(bool bMute);
    bool SetCurrentVolume(long nVolume);
    void SubmitEOS();
    bool IsEOS();

    void Flush();

    void Process();
    bool SetParameter(PARAMETER_TYPE type, int param);
    bool GetParameter(PARAMETER_TYPE type, int *param);

private:
    bool          m_Initialized;
    long          m_CurrentVolume;
    unsigned int  m_BytesPerSec;
    unsigned int  m_BufferLen;
    unsigned int  m_ChunkLen;
    unsigned int  m_InputChannels;
    OMX_AUDIO_CODINGTYPE m_eEncoding;
    uint8_t       *m_extradata;
    int           m_extrasize;
    float         m_fifo_size;

protected:
    COMXCoreComponent m_omx_render;
    COMXCoreComponent m_omx_decoder;
    bool              m_settings_changed;
};
#endif

