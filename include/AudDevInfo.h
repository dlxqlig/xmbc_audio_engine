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

#include <string>
#include <vector>

#include "../include/AudFormat.h"
#include "../include/AudChInfo.h"

typedef std::vector<unsigned int> AudSampleRateList;
typedef std::vector<enum AudDataFormat> AudDataFormatList;

enum AudDeviceType {
    AUD_DEVTYPE_PCM,
    AUD_DEVTYPE_IEC958,
    AUD_DEVTYPE_HDMI,
};

class CAudDevInfo
{
public:
    std::string m_device_name;
    std::string m_display_name;
    std::string m_display_name_extra;
    enum AudDeviceType m_device_type;
    CAudChInfo m_channels;
    AudSampleRateList m_sample_rates;
    AudDataFormatList m_data_formats;

    std::string ToDevInfoString();
    static std::string DeviceTypeToString(enum AudDeviceType deviceType);
};

typedef std::vector<CAudDevInfo> AudDevInfoList;
