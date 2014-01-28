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
#ifndef _OMX_PLAYERAUDIO_H_
#define _OMX_PLAYERAUDIO_H_

#include <deque>
#include <string>
#include <sys/types.h>

#include "OMXAudio.h"
#include "Thread.h"
#include "AudBuffer.h"

using namespace std;

class OMXPlayerAudio : public AThread
{
protected:
    pthread_cond_t            m_packet_cond;
    pthread_cond_t            m_out_packet_cond;
    pthread_mutex_t           m_lock;
    pthread_mutex_t           m_lock_decoder;
    pthread_mutex_t           m_lock_out_packet;
    COMXAudio                 *m_decoder;
    unsigned int              m_cached_size;
    unsigned int              m_max_data_size;
    unsigned int              m_out_cached_size;
    unsigned int              m_out_max_data_size;
    int                       m_channel;
    unsigned int              m_sample_rate;
    unsigned int              m_sample_size;
    float                     m_fifo_size;
    COMXCore                  m_g_OMX;
    std::deque<OMXPacket *>   m_packets;
    std::deque<OMXPacket *>   m_out_packets;
    bool m_player_error;
    bool m_bAbort;
    void Lock();
    void UnLock();
    void LockDecoder();
    void UnLockDecoder();
    void LockOutPacket();
    void UnLockOutPacket();

public:
    OMXPlayerAudio();
    ~OMXPlayerAudio();
    bool Open(int channel, int sample_rate, int sample_size, float queue_size, float fifo_size);
    bool Close();
    bool Decode(OMXPacket *pkt);
    void Process();
    bool AddPacket(OMXPacket *pkt);
    bool OpenDecoder();
    bool CloseDecoder();
    unsigned int GetCached() {return m_cached_size;};
    unsigned int GetOutCached() {return m_out_cached_size;};
    OMXPacket *GetData();
    bool SetParameter(PARAMETER_TYPE type, int param);
    bool GetParameter(PARAMETER_TYPE type, int *param);
    void SubmitEOS();
    bool IsEOS();
};
#endif
