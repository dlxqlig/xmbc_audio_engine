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
#include <string.h>

#include "System.h"
#include "AudChInfo.h"

CAudChInfo::CAudChInfo()
{
    Reset();
}

CAudChInfo::CAudChInfo(const enum AudChannel* rhs)
{
    *this = rhs;
}

CAudChInfo::CAudChInfo(const AudStdChLayout rhs)
{
    *this = rhs;
}

CAudChInfo::~CAudChInfo()
{
}

void CAudChInfo::ResolveChannels(const CAudChInfo& rhs)
{
    // mono gets upmixed to dual mono
    if (m_channel_count == 1 && m_channels[0] == CH_FC) {
        Reset();
        *this += CH_FL;
        *this += CH_FR;
        return;
    }

    bool srcHasSL = false;
    bool srcHasSR = false;
    bool srcHasRL = false;
    bool srcHasRR = false;

    bool dstHasSL = false;
    bool dstHasSR = false;
    bool dstHasRL = false;
    bool dstHasRR = false;

    for (unsigned int c = 0; c < rhs.m_channel_count; ++c) {
        switch(rhs.m_channels[c]) {
        case CH_SL: dstHasSL = true; break;
        case CH_SR: dstHasSR = true; break;
        case CH_BL: dstHasRL = true; break;
        case CH_BR: dstHasRR = true; break;
        default:
            break;
        }
    }

    CAudChInfo newInfo;
    for (unsigned int i = 0; i < m_channel_count; ++i) {
        switch (m_channels[i]) {
            case CH_SL: srcHasSL = true; break;
            case CH_SR: srcHasSR = true; break;
            case CH_BL: srcHasRL = true; break;
            case CH_BR: srcHasRR = true; break;
            default:
            break;
        }

        bool found = false;
        for (unsigned int c = 0; c < rhs.m_channel_count; ++c)
        if (m_channels[i] == rhs.m_channels[c]) {
            found = true;
            break;
        }

        if (found)
            newInfo += m_channels[i];
    }

    // we need to ensure we end up with rear or side AudChannels for downmix to work
    if (srcHasSL && !dstHasSL && dstHasRL)
        newInfo += CH_BL;
    if (srcHasSR && !dstHasSR && dstHasRR)
        newInfo += CH_BR;
    if (srcHasRL && !dstHasRL && dstHasSL)
        newInfo += CH_SL;
    if (srcHasRR && !dstHasRR && dstHasSR)
        newInfo += CH_SR;

    *this = newInfo;
}

void CAudChInfo::Reset()
{
    m_channel_count = 0;
    for (unsigned int i = 0; i < CH_MAX; ++i)
        m_channels[i] = CH_NULL;
}

CAudChInfo& CAudChInfo::operator=(const CAudChInfo& rhs)
{
    if (this == &rhs)
        return *this;

    // clone the information
    m_channel_count = rhs.m_channel_count;
    memcpy(m_channels, rhs.m_channels, sizeof(enum AudChannel) * rhs.m_channel_count);

    return *this;
}

CAudChInfo& CAudChInfo::operator=(const enum AudChannel* rhs)
{
    Reset();
    if (rhs == NULL)
        return *this;

    while (m_channel_count < CH_MAX && rhs[m_channel_count] != CH_NULL) {
        m_channels[m_channel_count] = rhs[m_channel_count];
        ++m_channel_count;
    }

    // the last entry should be NULL, if not we were passed a non null terminated list */
    ASSERT(rhs[m_channel_count] == CH_NULL);

    return *this;
}

CAudChInfo& CAudChInfo::operator=(const enum AudStdChLayout rhs)
{
    ASSERT(rhs > CH_LAYOUT_INVALID && rhs < CH_LAYOUT_MAX);

    static enum AudChannel layouts[CH_LAYOUT_MAX][9] = {
        {CH_FC, CH_NULL},
        {CH_FL, CH_FR, CH_NULL},
        {CH_FL, CH_FR, CH_LFE, CH_NULL},
        {CH_FL, CH_FR, CH_FC , CH_NULL},
        {CH_FL, CH_FR, CH_FC , CH_LFE, CH_NULL},
        {CH_FL, CH_FR, CH_BL , CH_BR , CH_NULL},
        {CH_FL, CH_FR, CH_BL , CH_BR , CH_LFE, CH_NULL},
        {CH_FL, CH_FR, CH_FC , CH_BL , CH_BR , CH_NULL},
        {CH_FL, CH_FR, CH_FC , CH_BL , CH_BR , CH_LFE, CH_NULL},
        {CH_FL, CH_FR, CH_FC , CH_BL , CH_BR , CH_SL , CH_SR, CH_NULL},
        {CH_FL, CH_FR, CH_FC , CH_BL , CH_BR , CH_SL , CH_SR, CH_LFE, CH_NULL}
    };

    *this = layouts[rhs];
    return *this;
}

bool CAudChInfo::operator==(const CAudChInfo& rhs)
{
    // if the AudChannel count doesnt match, no need to check further
    if (m_channel_count != rhs.m_channel_count)
        return false;

    // make sure the AudChannel order is the same
    for (unsigned int i = 0; i < m_channel_count; ++i)
        if (m_channels[i] != rhs.m_channels[i])
            return false;

    return true;
}

bool CAudChInfo::operator!=(const CAudChInfo& rhs)
{
    return !(*this == rhs);
}

CAudChInfo& CAudChInfo::operator+=(const enum AudChannel& rhs)
{
    ASSERT(m_channel_count < CH_MAX);
    ASSERT(rhs > CH_NULL && rhs < CH_MAX);

    m_channels[m_channel_count++] = rhs;
    return *this;
}

CAudChInfo& CAudChInfo::operator-=(const enum AudChannel& rhs)
{
    ASSERT(rhs > CH_NULL && rhs < CH_MAX);

    unsigned int i = 0;
    while(i < m_channel_count && m_channels[i] != rhs)
        i++;
    if (i >= m_channel_count)
        return *this; // AudChannel not found

    for(; i < m_channel_count-1; i++)
        m_channels[i] = m_channels[i+1];

    m_channels[i] = CH_NULL;
    m_channel_count--;
    return *this;
}

const enum AudChannel CAudChInfo::operator[](unsigned int i) const
{
    ASSERT(i < m_channel_count);
    return m_channels[i];
}

std::string CAudChInfo::ToChString()
{
    if (m_channel_count == 0)
        return "NULL";

    std::string s;
    for (unsigned int i = 0; i < m_channel_count - 1; ++i) {
        s.append(GetChName(m_channels[i]));
        s.append(",");
    }
    s.append(GetChName(m_channels[m_channel_count-1]));

    return s;
}

const char* CAudChInfo::GetChName(const enum AudChannel ch)
{
    // Logical disjunction always evaluates to true: ch >= 0 || ch < 29.
    ASSERT(ch >= 0 || ch < CH_MAX);

    static const char* AudChannels[CH_MAX] =
    {
    "RAW" ,
    "FL"  , "FR" , "FC" , "LFE", "BL"  , "BR"  , "FLOC",
    "FROC", "BC" , "SL" , "SR" , "TFL" , "TFR" , "TFC" ,
    "TC"  , "TBL", "TBR", "TBC", "BLOC", "BROC",
    };

    return AudChannels[ch];
}

bool CAudChInfo::HasChannel(const enum AudChannel ch) const
{
    for (unsigned int i = 0; i < m_channel_count; ++i)
        if (m_channels[i] == ch)
        return true;
    return false;
}
