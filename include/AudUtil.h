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

#include <math.h>

#include "../include/AudFormat.h"
#include "../include/AudChInfo.h"
#include "System.h"

#define AUDIO_IS_BITSTREAM(x) ((x) == AUDIO_IEC958 || (x) == AUDIO_HDMI)

enum AudioOutputs
{
    AUDIO_ANALOG  = 0,
    AUDIO_IEC958,
    AUDIO_HDMI
};

// AV sync options
enum AVSync
{
    SYNC_DISCON   = 0,
    SYNC_SKIPDUP,
    SYNC_RESAMPLE
};

class CAudUtil;

#ifndef WORDS_BIGENDIAN
#define EndianSwapLE16(X) (X)
#define EndianSwapLE32(X) (X)
#define EndianSwapLE64(X) (X)
#define EndianSwapBE16(X) CAudUtil::EndianSwap16(X)
#define EndianSwapBE32(X) CAudUtil::EndianSwap32(X)
#define EndianSwapBE64(X) CAudUtil::EndianSwap64(X)
#else
#define EndianSwapLE16(X) CAudUtil::EndianSwap16(X)
#define EndianSwapLE32(X) CAudUtil::EndianSwap32(X)
#define EndianSwapLE64(X) CAudUtil::EndianSwap64(X)
#define EndianSwapBE16(X) (X)
#define EndianSwapBE32(X) (X)
#define EndianSwapBE64(X) (X)
#endif

class CAudUtil
{
private:
    static unsigned int m_seed;

public:
    static CAudChInfo GuessChLayout (const unsigned int channels);
    static const char* GetStdChLayoutName(const enum AudStdChLayout layout);
    static const unsigned int DataFormatToBits (const enum AudDataFormat dataFormat);
    static const char* DataFormatToStr (const enum AudDataFormat dataFormat);

    static __inline__ uint16_t EndianSwap16(uint16_t x) {
        return((x<<8)|(x>>8));
    }

    static __inline__ uint32_t EndianSwap32(uint32_t x) {
        return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
    }
    static __inline__ uint64_t EndianSwap64(uint64_t x) {
        uint32_t hi, lo;

        // Separate into high and low 32-bit values and swap them
        lo = (uint32_t)(x&0xFFFFFFFF);
        x >>= 32;
        hi = (uint32_t)(x&0xFFFFFFFF);
        x = EndianSwap32(lo);
        x <<= 32;
        x |= EndianSwap32(hi);
        return(x);

    }
    static void EndianSwap16Buf(uint16_t *dst, uint16_t *src, int w);
    static float FloatRand1(const float min, const float max);
    static void FloatRand4(const float min, const float max, float result[4]);
    static int RoundInt(double x);
    static int64_t CurrentHostCounter(void);
    static int64_t CurrentHostFrequency(void);
    static bool S16NeedsByteSwap(AudDataFormat in, AudDataFormat out);
    static void AudFormatToString(AudFormat format);
};

