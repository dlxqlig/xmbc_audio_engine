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
#include <string>

enum AudChannel
{
    CH_NULL = -1,
    CH_RAW ,

    CH_FL,     // Front Left
    CH_FR,     // Front Right
    CH_FC,     // Front Center
    CH_LFE,    // Low Frequency Effects
    CH_BL,     // Back Left
    CH_BR,     // Back Right
    CH_FLOC,   // Front Left Of Center
    CH_FROC,   // Front Right Of Center
    CH_BC,     // Back Center
    CH_SL,     // Side Left
    CH_SR,     // Side Right
    CH_TFL,    // Top Front Left
    CH_TFR,    // Top Front Right
    CH_TFC,    // Top Front Center
    CH_TC,     // Top Center
    CH_TBL,    // Top Back Left
    CH_TBR,    // Top Back Right
    CH_TBC,    // Top Back Center
    CH_BLOC,   // Top Left Of Center
    CH_BROC,   // Top Right Of Center

    CH_MAX
};

enum AudStdChLayout
{
    CH_LAYOUT_INVALID = -1,

    CH_LAYOUT_1_0 = 0,
    CH_LAYOUT_2_0,
    CH_LAYOUT_2_1,
    CH_LAYOUT_3_0,
    CH_LAYOUT_3_1,
    CH_LAYOUT_4_0,
    CH_LAYOUT_4_1,
    CH_LAYOUT_5_0,
    CH_LAYOUT_5_1,
    CH_LAYOUT_7_0,
    CH_LAYOUT_7_1,

    CH_LAYOUT_MAX
};

class CAudChInfo {

public:
    CAudChInfo();
    ~CAudChInfo();

    CAudChInfo(const enum AudChannel* rhs);
    CAudChInfo(const enum AudStdChLayout rhs);

    CAudChInfo& operator=(const CAudChInfo& rhs);
    CAudChInfo& operator=(const enum AudChannel* rhs);
    CAudChInfo& operator=(const enum AudStdChLayout rhs);

    bool operator==(const CAudChInfo& rhs);
    bool operator!=(const CAudChInfo& rhs);

    CAudChInfo& operator+=(const enum AudChannel& rhs);
    CAudChInfo& operator-=(const enum AudChannel& rhs);

    const enum AudChannel operator[](unsigned int i) const;
    std::string ToChString();

    void ResolveChannels(const CAudChInfo& rhs);
    void Reset();

    inline unsigned int Count() const { return m_channel_count; }
    static const char* GetChName(const enum AudChannel ch);
    bool HasChannel(const enum AudChannel ch) const;
    bool ContainsChannels(CAudChInfo& rhs) const;

private:
    unsigned int m_channel_count;
    enum AudChannel m_channels[CH_MAX];
};
