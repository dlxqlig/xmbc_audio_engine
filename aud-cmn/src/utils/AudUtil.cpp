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

#include <time.h>
#include <stdint.h>
#include <cassert>
#include <climits>
#include <cmath>

#include "AudUtil.h"
#include "Logger.h"

using namespace std;

unsigned int CAudUtil::m_seed = (unsigned int)(CAudUtil::CurrentHostCounter() / 1000.0f);

CAudChInfo CAudUtil::GuessChLayout(const unsigned int channels)
{

    CAudChInfo result;
    if (channels < 1 || channels > 8)
        return result;

    switch (channels)
    {
    case 1: result = CH_LAYOUT_1_0; break;
    case 2: result = CH_LAYOUT_2_0; break;
    case 3: result = CH_LAYOUT_3_0; break;
    case 4: result = CH_LAYOUT_4_0; break;
    case 5: result = CH_LAYOUT_5_0; break;
    case 6: result = CH_LAYOUT_5_1; break;
    case 7: result = CH_LAYOUT_7_0; break;
    case 8: result = CH_LAYOUT_7_1; break;
    }

    return result;
}

const char* CAudUtil::GetStdChLayoutName(const enum AudStdChLayout layout)
{
    if (layout < 0 || layout >= CH_LAYOUT_MAX)
        return "UNKNOWN";

    static const char* layouts[CH_LAYOUT_MAX] =
    {
        "1.0",
        "2.0", "2.1", "3.0", "3.1", "4.0",
        "4.1", "5.0", "5.1", "7.0", "7.1"
    };

    return layouts[layout];
}

const unsigned int CAudUtil::DataFormatToBits(const enum AudDataFormat dataFormat)
{
    if (dataFormat < 0 || dataFormat >= AUD_FMT_MAX)
        return 0;

    static const unsigned int formats[AUD_FMT_MAX] =
    {
    8,                   /* U8     */
    8,                   /* S8     */

    16,                  /* S16BE  */
    16,                  /* S16LE  */
    16,                  /* S16NE  */

    32,                  /* S32BE  */
    32,                  /* S32LE  */
    32,                  /* S32NE  */

    32,                  /* S24BE  */
    32,                  /* S24LE  */
    32,                  /* S24NE  */

    24,                  /* S24BE3 */
    24,                  /* S24LE3 */
    24,                  /* S24NE3 */

    sizeof(double) << 3, /* DOUBLE */
    sizeof(float ) << 3, /* FLOAT  */

    16,                  /* AAC    */
    16,                  /* AC3    */
    16,                  /* DTS    */
    16,                  /* EAC3   */
    16,                  /* TRUEHD */
    16,                  /* DTS-HD */
    32                   /* LPCM   */
    };

    return formats[dataFormat];
}

const char* CAudUtil::DataFormatToStr(const enum AudDataFormat dataFormat)
{
    if (dataFormat < 0 || dataFormat >= AUD_FMT_MAX)
        return "UNKNOWN";

    static const char *formats[AUD_FMT_MAX] =
    {
    "AUD_FMT_U8",
    "AUD_FMT_S8",

    "AUD_FMT_S16BE",
    "AUD_FMT_S16LE",
    "AUD_FMT_S16NE",

    "AUD_FMT_S32BE",
    "AUD_FMT_S32LE",
    "AUD_FMT_S32NE",

    "AUD_FMT_S24BE4",
    "AUD_FMT_S24LE4",
    "AUD_FMT_S24NE4",  /* S24 in 4 bytes */

    "AUD_FMT_S24BE3",
    "AUD_FMT_S24LE3",
    "AUD_FMT_S24NE3", /* S24 in 3 bytes */

    "AUD_FMT_DOUBLE",
    "AUD_FMT_FLOAT",

    /* for passthrough streams and the like */
    "AUD_FMT_AAC",
    "AUD_FMT_AC3",
    "AUD_FMT_DTS",
    "AUD_FMT_EAC3",
    "AUD_FMT_TRUEHD",
    "AUD_FMT_DTSHD",
    "AUD_FMT_LPCM"
    };

    return formats[dataFormat];
}

void CAudUtil::EndianSwap16Buf(uint16_t *dst, uint16_t *src, int w)
{
    int i;

    for (i = 0; i + 8 <= w; i += 8) {
        dst[i + 0] = EndianSwap16(src[i + 0]);
        dst[i + 1] = EndianSwap16(src[i + 1]);
        dst[i + 2] = EndianSwap16(src[i + 2]);
        dst[i + 3] = EndianSwap16(src[i + 3]);
        dst[i + 4] = EndianSwap16(src[i + 4]);
        dst[i + 5] = EndianSwap16(src[i + 5]);
        dst[i + 6] = EndianSwap16(src[i + 6]);
        dst[i + 7] = EndianSwap16(src[i + 7]);
    }

    for (; i < w; i++)
        dst[i + 0] = EndianSwap16(src[i + 0]);
}

/*
  Rand implementations based on:
  http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
  This is NOT safe for crypto work, but perfectly fine for audio usage (dithering)
*/
float CAudUtil::FloatRand1(const float min, const float max)
{
    const float delta  = (max - min) / 2;
    const float factor = delta / (float)INT32_MAX;
    return ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
}

void CAudUtil::FloatRand4(const float min, const float max, float result[4])
{

    const float delta  = (max - min) / 2.0f;
    const float factor = delta / (float)INT32_MAX;

    /* cant return sseresult if we are not using SSE intrinsics */
    // TODO:
    // ASSERT(result && !sseresult);

    result[0] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
    result[1] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
    result[2] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
    result[3] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;

}

// GCC does something stupid with optimization on release builds if we try
// to assert in these functions

/*! \brief Round to nearest integer.
 This routine does fast rounding to the nearest integer.
 In the case (k + 0.5 for any integer k) we round up to k+1, and in all other
 instances we should return the nearest integer.
 Thus, { -1.5, -0.5, 0.5, 1.5 } is rounded to { -1, 0, 1, 2 }.
 It preserves the property that round(k) - round(k-1) = 1 for all doubles k.

 Make sure MathUtils::test() returns true for each implementation.
 \sa truncate_int, test
*/
int CAudUtil::RoundInt(double x)
{
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast<double>(INT_MAX / 2) + 1.0);
    const float round_to_nearest = 0.5f;
    int i;

    i = floor(x + round_to_nearest);
    return i;
}

int64_t CAudUtil::CurrentHostCounter(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return( ((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec );
}

int64_t CAudUtil::CurrentHostFrequency(void)
{
    return( (int64_t)1000000000L );
}

bool CAudUtil::S16NeedsByteSwap(AudDataFormat in, AudDataFormat out)
{
    const AudDataFormat nativeFormat =
#ifdef WORDS_BIGENDIAN
    AUD_FMT_S16BE;
#else
    AUD_FMT_S16LE;
#endif

    if (in == AUD_FMT_S16NE || AUD_IS_RAW(in))
        in = nativeFormat;
    if (out == AUD_FMT_S16NE || AUD_IS_RAW(out))
        out = nativeFormat;

    return in != out;
}

void CAudUtil::AudFormatToString(AudFormat format)
{
    Logger::LogOut(LOG_LEVEL_DEBUG, "  Sample Rate   : %d", format.m_sample_rate);
    Logger::LogOut(LOG_LEVEL_DEBUG, "  Sample Format : %s", CAudUtil::DataFormatToStr(format.m_data_format));
    Logger::LogOut(LOG_LEVEL_DEBUG, "  Channel Count : %d", format.m_channel_layout.Count());
    Logger::LogOut(LOG_LEVEL_DEBUG, "  Channel Layout: %s", format.m_channel_layout.ToChString().c_str());
    Logger::LogOut(LOG_LEVEL_DEBUG, "  Frames        : %d", format.m_frames);
    Logger::LogOut(LOG_LEVEL_DEBUG, "  Frame Samples : %d", format.m_frame_samples);
    Logger::LogOut(LOG_LEVEL_DEBUG, "  Frame Size    : %d", format.m_frame_size);
}
