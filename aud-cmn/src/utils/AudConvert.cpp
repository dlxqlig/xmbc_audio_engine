
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
#include <math.h>
#include <string.h>
#include <limits.h>

#include "../include/AudConvert.h"
#include "../include/AudUtil.h"

#define CLAMP(x) std::min(-1.0f, std::max(1.0f, (float)(x)))

#ifndef INT24_MAX
#define INT24_MAX (0x7FFFFF)
#endif

#define INT32_SCALE (-1.0f / INT_MIN)

static inline int safeRound(double f)
{
    // if the value is larger then we can handle, then clamp it
    if (f >= INT_MAX)
        return INT_MAX;
    if (f <= INT_MIN)
        return INT_MIN;

    // if the value is out of the MathUtils::round_int range, then round it normally
    if (f <= static_cast<double>(INT_MIN / 2) - 1.0 || f >= static_cast <double>(INT_MAX / 2) + 1.0)
        return (int)floor(f+0.5);

    return CAudUtil::RoundInt(f);

}

CAudConvert::AudConvertToFn CAudConvert::ToFloat(enum AudDataFormat dataFormat)
{
    switch (dataFormat)
    {
    case AUD_FMT_U8    : return &U8_Float;
    case AUD_FMT_S8    : return &S8_Float;
#ifdef __BIG_Endian_
    case AUD_FMT_S16NE : return &S16BE_Float;
    case AUD_FMT_S32NE : return &S32BE_Float;
    case AUD_FMT_S24NE4: return &S24BE4_Float;
    case AUD_FMT_S24NE3: return &S24BE3_Float;
#else
    case AUD_FMT_S16NE : return &S16LE_Float;
    case AUD_FMT_S32NE : return &S32LE_Float;
    case AUD_FMT_S24NE4: return &S24LE4_Float;
    case AUD_FMT_S24NE3: return &S24LE3_Float;
#endif
    case AUD_FMT_S16LE : return &S16LE_Float;
    case AUD_FMT_S16BE : return &S16BE_Float;
    case AUD_FMT_S24LE4: return &S24LE4_Float;
    case AUD_FMT_S24BE4: return &S24BE4_Float;
    case AUD_FMT_S24LE3: return &S24LE3_Float;
    case AUD_FMT_S24BE3: return &S24BE3_Float;
    case AUD_FMT_S32LE : return &S32LE_Float;
    case AUD_FMT_S32BE : return &S32BE_Float;
    case AUD_FMT_DOUBLE: return &DOUBLE_Float;
    default:
        return NULL;
    }
}

CAudConvert::AudConvertFrFn CAudConvert::FrFloat(enum AudDataFormat dataFormat)
{
    switch (dataFormat)
    {
    case AUD_FMT_U8    : return &Float_U8;
    case AUD_FMT_S8    : return &Float_S8;
#ifdef __BIG_Endian_
    case AUD_FMT_S16NE : return &Float_S16BE;
    case AUD_FMT_S32NE : return &Float_S32BE;
#else
    case AUD_FMT_S16NE : return &Float_S16LE;
    case AUD_FMT_S32NE : return &Float_S32LE;
#endif
    case AUD_FMT_S16LE : return &Float_S16LE;
    case AUD_FMT_S16BE : return &Float_S16BE;
    case AUD_FMT_S24NE4: return &Float_S24NE4;
    case AUD_FMT_S24NE3: return &Float_S24NE3;
    case AUD_FMT_S32LE : return &Float_S32LE;
    case AUD_FMT_S32BE : return &Float_S32BE;
    case AUD_FMT_DOUBLE: return &Float_DOUBLE;
    default:
      return NULL;
    }
}

unsigned int CAudConvert::U8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    const float mul = 2.0f / UINT8_MAX;

    for (unsigned int i = 0; i < samples; ++i)
        *dest++ = *data++ * mul - 1.0f;

    return samples;
}

unsigned int CAudConvert::S8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    const float mul = 1.0f / (INT8_MAX + 0.5f);

    for (unsigned int i = 0; i < samples; ++i)
        *dest++ = *data++ * mul;

    return samples;
}

unsigned int CAudConvert::S16LE_Float(uint8_t* data, const unsigned int samples, float *dest)
{
    static const float mul = 1.0f / (INT16_MAX + 0.5f);

    for (unsigned int i = 0; i < samples; ++i, data += 2)
        *dest++ = EndianSwapLE16(*(int16_t*)data) * mul;
    return samples;
}

unsigned int CAudConvert::S16BE_Float(uint8_t* data, const unsigned int samples, float *dest)
{
    static const float mul = 1.0f / (INT16_MAX + 0.5f);

    for (unsigned int i = 0; i < samples; ++i, data += 2)
        *dest++ = EndianSwapBE16(*(int16_t*)data) * mul;

    return samples;
}

unsigned int CAudConvert::S24LE4_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    for (unsigned int i = 0; i < samples; ++i, data += 4)
    {
        int s = (data[2] << 24) | (data[1] << 16) | (data[0] << 8);
        *dest++ = (float)s * INT32_SCALE;
    }
    return samples;
}

unsigned int CAudConvert::S24BE4_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    for (unsigned int i = 0; i < samples; ++i, data += 4)
    {
        int s = (data[0] << 24) | (data[1] << 16) | (data[2] << 8);
        *dest++ = (float)s * INT32_SCALE;
    }
    return samples;
}

unsigned int CAudConvert::S24LE3_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    for (unsigned int i = 0; i < samples; ++i, data += 3)
    {
        int s = (data[2] << 24) | (data[1] << 16) | (data[0] << 8);
        *dest++ = (float)s * INT32_SCALE;
    }
    return samples;
}

unsigned int CAudConvert::S24BE3_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    for (unsigned int i = 0; i < samples; ++i, data += 3)
    {
        int s = (data[1] << 24) | (data[2] << 16) | (data[3] << 8);
        *dest++ = (float)s * INT32_SCALE;
    }
    return samples;
}

unsigned int CAudConvert::S32LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    static const float factor = 1.0f / (float)INT32_MAX;
    int32_t *src = (int32_t*)data;

    // do this in groups of 4 to give the compiler a better chance of optimizing this
    for (float *end = dest + (samples & ~0x3); dest < end;)
    {
        *dest++ = (float)EndianSwapLE32(*src++) * factor;
        *dest++ = (float)EndianSwapLE32(*src++) * factor;
        *dest++ = (float)EndianSwapLE32(*src++) * factor;
        *dest++ = (float)EndianSwapLE32(*src++) * factor;
    }

    // process any remaining samples
    for (float *end = dest + (samples & 0x3); dest < end;)
        *dest++ = (float)EndianSwapLE32(*src++) * factor;

    return samples;
}

unsigned int CAudConvert::S32LE_Float_Neon(uint8_t *data, const unsigned int samples, float *dest)
{
    return samples;
}

unsigned int CAudConvert::S32BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    static const float factor = 1.0f / (float)INT32_MAX;
    int32_t *src = (int32_t*)data;

    // do this in groups of 4 to give the compiler a better chance of optimizing this
    for (float *end = dest + (samples & ~0x3); dest < end;)
    {
        *dest++ = (float)EndianSwapBE32(*src++) * factor;
        *dest++ = (float)EndianSwapBE32(*src++) * factor;
        *dest++ = (float)EndianSwapBE32(*src++) * factor;
        *dest++ = (float)EndianSwapBE32(*src++) * factor;
    }

    // process any remaining samples
    for (float *end = dest + (samples & 0x3); dest < end;)
        *dest++ = (float)EndianSwapBE32(*src++) * factor;

    return samples;
}

unsigned int CAudConvert::S32BE_Float_Neon(uint8_t *data, const unsigned int samples, float *dest)
{
    return samples;
}

unsigned int CAudConvert::DOUBLE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
    double *src = (double*)data;
    for (unsigned int i = 0; i < samples; ++i)
        *dest++ = CLAMP(*src++ / (float)INT32_MAX);

    return samples;
}

unsigned int CAudConvert::Float_U8(float *data, const unsigned int samples, uint8_t *dest)
{
    for (uint32_t i = 0; i < samples; ++i)
        *dest++ = safeRound((*data++ + 1.0f) * ((float)INT8_MAX+.5f));
    return samples;
}

unsigned int CAudConvert::Float_S8(float *data, const unsigned int samples, uint8_t *dest)
{
    for (uint32_t i = 0; i < samples; ++i)
        *dest++ = safeRound(*data++ * ((float)INT8_MAX+.5f));

    return samples;
}

unsigned int CAudConvert::Float_S16LE(float *data, const unsigned int samples, uint8_t *dest)
{
    int16_t *dst = (int16_t*)dest;

    uint32_t i    = 0;
    uint32_t even = samples & ~0x3;

    for(; i < even; i += 4)
    {
        // random round to dither
        float rand[4];
        CAudUtil::FloatRand4(-0.5f, 0.5f, rand);

        *dst++ = EndianSwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[0])));
        *dst++ = EndianSwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[1])));
        *dst++ = EndianSwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[2])));
        *dst++ = EndianSwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[3])));
    }

    for(; i < samples; ++i)
        *dst++ = EndianSwapLE16(safeRound(*data++ * ((float)INT16_MAX + CAudUtil::FloatRand1(-0.5f, 0.5f))));

    return samples << 1;
}

unsigned int CAudConvert::Float_S16BE(float *data, const unsigned int samples, uint8_t *dest)
{
    int16_t *dst = (int16_t*)dest;

    uint32_t i    = 0;
    uint32_t even = samples & ~0x3;

    for(; i < even; i += 4)
    {
        // random round to dither
        float rand[4];
        CAudUtil::FloatRand4(-0.5f, 0.5f, rand);

        *dst++ = EndianSwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[0])));
        *dst++ = EndianSwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[1])));
    *dst++ = EndianSwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[2])));
    *dst++ = EndianSwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[3])));
    }

    for(; i < samples; ++i, data++, dst++)
        *dst++ = EndianSwapBE16(safeRound(*data++ * ((float)INT16_MAX + CAudUtil::FloatRand1(-0.5f, 0.5f))));

    return samples << 1;
}

unsigned int CAudConvert::Float_S24NE4(float *data, const unsigned int samples, uint8_t *dest)
{
    int32_t *dst = (int32_t*)dest;

    for (uint32_t i = 0; i < samples; ++i)
        *dst++ = (safeRound(*data++ * ((float)INT24_MAX+.5f)) & 0xFFFFFF) << 8;

    return samples << 2;
}

unsigned int CAudConvert::Float_S24NE3(float *data, const unsigned int samples, uint8_t *dest)
{
    // We do not want to shift for S24LE3, since left-shifting would actually
    // push the MSB to the 4th byte.
    const int leftShift =
#ifdef __BIG_Endian_
    8;
#else
    0;
#endif

    for (uint32_t i = 0; i < samples; ++i, ++data, dest += 3)
        *((uint32_t*)(dest)) = (safeRound(*data * ((float)INT24_MAX+.5f)) & 0xFFFFFF) << leftShift;

    return samples * 3;
}

// float can't store INT32_MAX, it gets rounded up to INT32_MAX + 1
// INT32_MAX - 127 is the maximum value that can exactly be stored in both 32 bit float and int
#define AUD_MUL32 ((float)(INT32_MAX - 127))

unsigned int CAudConvert::Float_S32LE(float *data, const unsigned int samples, uint8_t *dest)
{
    int32_t *dst = (int32_t*)dest;
    /* no SIMD */
    for (uint32_t i = 0; i < samples; ++i, ++data, ++dst)
    {
        dst[0] = safeRound(data[0] * AUD_MUL32);
        dst[0] = EndianSwapLE32(dst[0]);
    }

    return samples << 2;
}

unsigned int CAudConvert::Float_S32LE_Neon(float *data, const unsigned int samples, uint8_t *dest)
{
    return samples << 2;
}

unsigned int CAudConvert::Float_S32BE(float *data, const unsigned int samples, uint8_t *dest)
{
    int32_t *dst = (int32_t*)dest;
    for (uint32_t i = 0; i < samples; ++i, ++data, ++dst)
    {
        dst[0] = safeRound(data[0] * AUD_MUL32);
        dst[0] = EndianSwapBE32(dst[0]);
    }

    return samples << 2;
}

unsigned int CAudConvert::Float_S32BE_Neon(float *data, const unsigned int samples, uint8_t *dest)
{
    return samples << 2;
}

unsigned int CAudConvert::Float_DOUBLE(float *data, const unsigned int samples, uint8_t *dest)
{
    double *dst = (double*)dest;
    for (unsigned int i = 0; i < samples; ++i)
        *dst++ = *data++;

    return samples * sizeof(double);
}
