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

#include <math.h>
#include <sstream>
#include <string.h>

#include "AudRemap.h"
#include "Logger.h"

using namespace std;

CAudRemap::CAudRemap() : m_inChannels(0), m_outChannels(0)
{
    memset(m_mixInfo, 0, sizeof(m_mixInfo));
}

CAudRemap::~CAudRemap()
{
}

bool CAudRemap::Initialize(CAudChInfo input, CAudChInfo output, bool finalStage, bool forceNormalize/* = false */, enum AudStdChLayout stdChLayout/* = CH_LAYOUT_INVALID */)
{
    if (!input.Count() || !output.Count())
        return false;

    // build the downmix matrix
    memset(m_mixInfo, 0, sizeof(m_mixInfo));
    m_output = output;

    // figure which channels we have
    for (unsigned int o = 0; o < output.Count(); ++o)
        m_mixInfo[output[o]].in_dst = true;

    // flag invalid channels for forced downmix
    if (stdChLayout != CH_LAYOUT_INVALID) {
        CAudChInfo layout = stdChLayout;
        for (unsigned int o = 0; o < output.Count(); ++o)
            if (!layout.HasChannel(output[o]))
                m_mixInfo[output[o]].in_dst = false;
    }

    m_outChannels = output.Count();

    // lookup the channels that exist in the output
    for (unsigned int i = 0; i < input.Count(); ++i) {
        AudMixInfo  *info = &m_mixInfo[input[i]];
        AudMixLevel *lvl  = &info->srcIndex[info->srcCount++];
        info->in_src = true;
        lvl->index   = i;
        lvl->level   = 1.0f;
        if (!info->in_dst) {
            for (unsigned int o = 0; o < output.Count(); ++o) {
                if (input[i] == output[o]) {
                    info->outIndex = o;
                    break;
                }
            }
        }

        m_inChannels = i;
    }
    ++m_inChannels;

    // the final stage does not need any down/upmix
    if (finalStage)
        return true;

    // downmix from the specified channel to the specified list of channels
    #define RM(from, ...) \
        static AudChannel downmix_##from[] = {__VA_ARGS__, CH_NULL}; \
        ResolveMix(from, CAudChInfo(downmix_##from));

  /*
    the order of this is important as we can not mix channels
    into ones that have already been resolved... eg

    TBR -> BR
    TBC -> TBL & TBR

    TBR will get resolved to BR, and then TBC will get
    resolved to TBL, but since TBR has been resolved it will
    never make it to the output. The order has to be reversed
    as the TBC center depends on the TBR downmix... eg

    TBC -> TBL & TBR
    TBR -> BR

    if for any reason out of order mapping becomes required
    looping this list should resolve the channels.
  */
    RM(CH_TBC , CH_TBL, CH_TBR);
    RM(CH_TBR , CH_BR);
    RM(CH_TBL , CH_BL);
    RM(CH_TC  , CH_TFL, CH_TFR);
    RM(CH_TFC , CH_TFL, CH_TFR);
    RM(CH_TFR , CH_FR);
    RM(CH_TFL , CH_FL);

    // Some 5.1 Setups announce a BR and BL instead of SR and SL - put out SR and SL on those instead
    if (!m_mixInfo[CH_BL].in_src && !m_mixInfo[CH_BR].in_src && // make sure not to not mix 7.1 channels
            (m_mixInfo[CH_SL].in_src && !m_mixInfo[CH_SL].in_dst && m_mixInfo[CH_BL].in_dst) &&
            (m_mixInfo[CH_SR].in_src && !m_mixInfo[CH_SR].in_dst && m_mixInfo[CH_BR].in_dst))
    {
        RM(CH_SR  , CH_BR);
        RM(CH_SL  , CH_BL);
    }
    else
    {
        RM(CH_SR  , CH_BR, CH_FR);
        RM(CH_SL  , CH_BL, CH_FL);
    }

    RM(CH_BC  , CH_BL, CH_BR);
    RM(CH_FROC, CH_FR, CH_FC);
    RM(CH_FLOC, CH_FL, CH_FC);
    RM(CH_BL  , CH_FL);
    RM(CH_BR  , CH_FR);
    RM(CH_LFE , CH_FL, CH_FR);
    RM(CH_FL  , CH_FC);
    RM(CH_FR  , CH_FC);
    RM(CH_BROC, CH_BR, CH_BC);
    RM(CH_BLOC, CH_BL, CH_BC);

    /* since everything eventually mixes down to FC we need special handling for it */
    if (m_mixInfo[CH_FC].in_src)
    {
        /* if there is no output FC channel, try to mix it the best possible way */
        if (!m_mixInfo[CH_FC].in_dst)
        {
            /* if we have TFC & FL & FR */
            if (m_mixInfo[CH_TFC].in_dst && m_mixInfo[CH_FL].in_dst && m_mixInfo[CH_FR].in_dst)
            {
                RM(CH_FC, CH_TFC, CH_FL, CH_FR);
            }
            /* if we have TFC */
            else if (m_mixInfo[CH_TFC].in_dst)
            {
                RM(CH_FC, CH_TFC);
            }
            /* if we have FLOC & FROC */
         else if (m_mixInfo[CH_FLOC].in_dst && m_mixInfo[CH_FROC].in_dst)
         {
             RM(CH_FC, CH_FLOC, CH_FROC);
        }
            /* if we have TC & FL & FR */
         else if (m_mixInfo[CH_TC].in_dst && m_mixInfo[CH_FL].in_dst && m_mixInfo[CH_FR].in_dst)
         {
             RM(CH_FC, CH_TC, CH_FL, CH_FR);
         }
            /* if we have FL & FR */
         else if (m_mixInfo[CH_FL].in_dst && m_mixInfo[CH_FR].in_dst)
         {
             RM(CH_FC, CH_FL, CH_FR);
         }
            /* if we have TFL & TFR */
         else if (m_mixInfo[CH_TFL].in_dst && m_mixInfo[CH_TFR].in_dst)
         {
         RM(CH_FC, CH_TFL, CH_TFR);
         }
            /* we dont have enough speakers to emulate FC */
         else
             return false;
        }
        else
        {
            /* if there is only one channel in the source and it is the FC and we have FL & FR, upmix to dual mono */
            if (m_inChannels == 1 && m_mixInfo[CH_FL].in_dst && m_mixInfo[CH_FR].in_dst)
            {
                RM(CH_FC, CH_FL, CH_FR);
            }
        }
    }

    #undef RM

    // if (CSettings::Get().GetBool("audiooutput.stereoupmix"))
    //    BuildUpmixMatrix(input, output);

    // normalize the values
    if (forceNormalize) {
        float max = 0;
        for (unsigned int o = 0; o < output.Count(); ++o) {
            AudMixInfo *info = &m_mixInfo[output[o]];
            float sum = 0;
            for (int i = 0; i < info->srcCount; ++i)
                sum += info->srcIndex[i].level;

            if (sum > max)
                max = sum;
        }

        float scale = 1.0f / max;
        for (unsigned int o = 0; o < output.Count(); ++o) {
            AudMixInfo *info = &m_mixInfo[output[o]];
            for (int i = 0; i < info->srcCount; ++i)
                info->srcIndex[i].level *= scale;
        }
    }

#if 0
  /* dump the matrix */
  Logger::LogOut(LOG_LEVEL_INFO, "==[Downmix Matrix]==");
  for (unsigned int o = 0; o < output.Count(); ++o)
  {
    AudMixInfo *info = &m_mixInfo[output[o]];
    if (info->srcCount == 0)
      continue;

    std::stringstream s;
    s << CAudChInfo::GetChName(output[o]) << " =";
    for (int i = 0; i < info->srcCount; ++i)
      s << " " << CAudChInfo::GetChName(input[info->srcIndex[i].index]) << "(" << info->srcIndex[i].level << ")";

    Logger::LogOut(LOG_LEVEL_INFO, "%s", s.str().c_str());
  }
  Logger::LogOut(LOG_LEVEL_INFO, "====================\n");
#endif

    return true;
}

void CAudRemap::ResolveMix(const AudChannel from, CAudChInfo to)
{
    AudMixInfo *fromInfo = &m_mixInfo[from];
    if (fromInfo->in_dst || !fromInfo->in_src)
        return;

    for (unsigned int i = 0; i < to.Count(); ++i) {
        AudMixInfo *toInfo = &m_mixInfo[to[i]];
        toInfo->in_src = true;
        for (int o = 0; o < fromInfo->srcCount; ++o) {
            AudMixLevel *fromLvl = &fromInfo->srcIndex[o];
            AudMixLevel *toLvl   = NULL;

            // if its already in the output, then we need to combine the levels
            for (int l = 0; l < toInfo->srcCount; ++l)
                if (toInfo->srcIndex[l].index == fromLvl->index) {
                    toLvl = &toInfo->srcIndex[l];
                    toLvl->level = (fromLvl->level + toLvl->level) / sqrt((float)to.Count() + 1.0f);
                    break;
                }

            if (toLvl)
                continue;

            toLvl = &toInfo->srcIndex[toInfo->srcCount++];
            toLvl->index = fromLvl->index;
            toLvl->level = fromLvl->level / sqrt((float)to.Count());
        }
    }

    fromInfo->srcCount = 0;
    fromInfo->in_src   = false;
}

// This method has unrolled loop for higher performance
void CAudRemap::Remap(float * const in, float * const out, const unsigned int frames) const
{
    const unsigned int frameBlocks = frames & ~0x3;

    for (int o = 0; o < m_outChannels; ++o) {
        const AudMixInfo *info = &m_mixInfo[m_output[o]];
        if (!info->in_dst) {
            unsigned int f = 0;
            unsigned int odx = 0;
            for(; f < frameBlocks; f += 4) {
                out[odx + o] = 0.0f, odx += m_outChannels;
                out[odx + o] = 0.0f, odx += m_outChannels;
                out[odx + o] = 0.0f, odx += m_outChannels;
                out[odx + o] = 0.0f, odx += m_outChannels;
            }

            switch (frames & 0x3) {
            case 3: out[odx + o] = 0.0f, odx += m_outChannels;
            case 2: out[odx + o] = 0.0f, odx += m_outChannels;
            case 1: out[odx + o] = 0.0f;
            }
            continue;
        }

        // if there is only 1 source, just copy it so we dont break DPL
        if (info->srcCount == 1) {
            unsigned int f = 0;
            unsigned int idx = 0;
            unsigned int odx = 0;
            unsigned int srcIndex = info->srcIndex[0].index;
            // the compiler has a better chance of optimizing this if it is done in parallel
            for (; f < frameBlocks; f += 4) {
                out[odx + o] = in[idx + srcIndex], idx += m_inChannels, odx += m_outChannels;
                out[odx + o] = in[idx + srcIndex], idx += m_inChannels, odx += m_outChannels;
                out[odx + o] = in[idx + srcIndex], idx += m_inChannels, odx += m_outChannels;
                out[odx + o] = in[idx + srcIndex], idx += m_inChannels, odx += m_outChannels;
            }

            switch (frames & 0x3) {
            case 3: out[odx + o] = in[idx + srcIndex], idx += m_inChannels, odx += m_outChannels;
            case 2: out[odx + o] = in[idx + srcIndex], idx += m_inChannels, odx += m_outChannels;
            case 1: out[odx + o] = in[idx + srcIndex];
            }
        }
        else {
            for (unsigned int f = 0; f < frames; ++f) {
                float *outOffset = out + (f * m_outChannels) + o;
                float *inOffset  = in  + (f * m_inChannels);
                *outOffset = 0.0f;

                int blocks = info->srcCount & ~0x3;

                // the compiler has a better chance of optimizing this if it is done in parallel
                int i = 0;
                float f1 = 0.0, f2 = 0.0, f3 = 0.0, f4 = 0.0;
                for (; i < blocks; i += 4) {
                    f1 += inOffset[info->srcIndex[i].index] * info->srcIndex[i].level;
                    f2 += inOffset[info->srcIndex[i+1].index] * info->srcIndex[i+1].level;
                    f3 += inOffset[info->srcIndex[i+2].index] * info->srcIndex[i+2].level;
                    f4 += inOffset[info->srcIndex[i+3].index] * info->srcIndex[i+3].level;
                }

                // unrolled loop for higher performance
                switch (info->srcCount & 0x3)
                {
                case 3: f3 += inOffset[info->srcIndex[i+2].index] * info->srcIndex[i+2].level;
                case 2: f2 += inOffset[info->srcIndex[i+1].index] * info->srcIndex[i+1].level;
                case 1: f1 += inOffset[info->srcIndex[i].index] * info->srcIndex[i].level;
                }

                *outOffset += (f1+f2+f3+f4);
            }
        }
    }
}

inline void CAudRemap::BuildUpmixMatrix(const CAudChInfo& input, const CAudChInfo& output)
{
    #define UM(from, to) \
    if (!m_mixInfo[to].in_src && m_mixInfo[to].in_dst) \
    { \
        AudMixInfo *toInfo   = &m_mixInfo[to  ]; \
        AudMixInfo *fromInfo = &m_mixInfo[from]; \
        toInfo->srcIndex[toInfo->srcCount].level = 1.0f; \
        toInfo->srcIndex[toInfo->srcCount].index = fromInfo->srcIndex[0].index; \
        toInfo  ->srcCount++; \
        fromInfo->cpyCount++; \
    }

    if (m_mixInfo[CH_FL].in_src)
    {
        UM(CH_FL, CH_BL );
        UM(CH_FL, CH_SL );
        UM(CH_FL, CH_FC );
        UM(CH_FL, CH_LFE);
    }

    if (m_mixInfo[CH_FR].in_src)
    {
        UM(CH_FR, CH_BR );
        UM(CH_FR, CH_SR );
        UM(CH_FR, CH_FC );
        UM(CH_FR, CH_LFE);
    }

    // fix the levels of anything we added
    for (unsigned int i = 0; i < output.Count(); ++i)
    {
        AudMixInfo *outputInfo = &m_mixInfo[output[i]];
        if (!outputInfo->in_src && outputInfo->srcCount > 0)
        for (int src = 0; src < outputInfo->srcCount; ++src)
        {
            AudChannel srcChannel = input[outputInfo->srcIndex[src].index];
                AudMixInfo *inputInfo = &m_mixInfo[srcChannel];
            outputInfo->srcIndex[src].level /= sqrt((float)(inputInfo->cpyCount + 1));
            }
    }

    // fix the source levels also
    for (unsigned int i = 0; i < input.Count(); ++i) {
        AudMixInfo *inputInfo = &m_mixInfo[input[i]];
        if (!inputInfo->in_dst || inputInfo->cpyCount == 0)
            continue;
        inputInfo->srcIndex[0].level /= sqrt((float)(inputInfo->cpyCount + 1));
    }
}
