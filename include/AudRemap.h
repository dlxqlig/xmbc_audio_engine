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

#include "../include/AudFormat.h"
#include "../include/AudChInfo.h"

class CAudRemap {
public:
    CAudRemap();
    ~CAudRemap();

    bool Initialize(CAudChInfo input, CAudChInfo output, bool finalStage, bool forceNormalize = false, enum AudStdChLayout stdChLayout = CH_LAYOUT_INVALID);
    void Remap(float * const in, float * const out, const unsigned int frames) const;

private:
    typedef struct {
        int index;
        float level;
    } AudMixLevel;

    typedef struct {
        bool in_src;
        bool in_dst;
        int outIndex;
        int srcCount;
        AudMixLevel srcIndex[CH_MAX];
        int cpyCount; /* the number of times the channel has been cloned */
    } AudMixInfo;

    AudMixInfo m_mixInfo[CH_MAX+1];
    CAudChInfo m_output;
    int m_inChannels;
    int m_outChannels;

    void ResolveMix(const AudChannel from, CAudChInfo to);
    void BuildUpmixMatrix(const CAudChInfo& input, const CAudChInfo& output);
};
