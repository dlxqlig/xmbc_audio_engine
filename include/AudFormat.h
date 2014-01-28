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

#include "AudChInfo.h"

// LE(Little Endian), BE(Big Endian), NE(Native Endian)
enum AudDataFormat
{
    AUD_FMT_INVALID = -1,

    AUD_FMT_U8,
    AUD_FMT_S8,

    AUD_FMT_S16BE,
    AUD_FMT_S16LE,
    AUD_FMT_S16NE,

    AUD_FMT_S32BE,
    AUD_FMT_S32LE,
    AUD_FMT_S32NE,

    AUD_FMT_S24BE4,
    AUD_FMT_S24LE4,
    AUD_FMT_S24NE4, // S24 in 4 bytes

    AUD_FMT_S24BE3,
    AUD_FMT_S24LE3,
    AUD_FMT_S24NE3, // S24 in 3 bytes

    AUD_FMT_DOUBLE,
    AUD_FMT_FLOAT,

    AUD_FMT_AAC,
    AUD_FMT_AC3,
    AUD_FMT_DTS,
    AUD_FMT_EAC3,
    AUD_FMT_TRUEHD,
    AUD_FMT_DTSHD,
    AUD_FMT_LPCM,

    AUD_FMT_MAX
};

#define AUD_IS_RAW(x) ((x) >= AUD_FMT_AAC && (x) < AUD_FMT_MAX)
#define AUD_IS_RAW_HD(x) ((x) >= AUD_FMT_EAC3 && (x) < AUD_FMT_MAX)

typedef struct AudFormat{
    enum AudDataFormat m_data_format;
    unsigned int m_sample_rate;
    unsigned int m_encoded_rate;
    CAudChInfo m_channel_layout;
    unsigned int m_frames; // frames per period
    unsigned int m_frame_samples; // samples in one frame
    unsigned int m_frame_size; // one frame in bytes

    AudFormat() {
        m_data_format = AUD_FMT_INVALID;
        m_sample_rate = 0;
        m_encoded_rate = 0;
        m_frames = 0;
        m_frame_samples = 0;
        m_frame_size = 0;
    }
} AudFormat;
