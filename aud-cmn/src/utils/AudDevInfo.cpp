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

#include <sstream>
#include "AudDevInfo.h"
#include "AudUtil.h"

std::string CAudDevInfo::ToDevInfoString()
{
    std::stringstream ss;
    ss << "m_device_name      : " << m_device_name << '\n';
    ss << "m_display_name     : " << m_display_name << '\n';
    ss << "m_display_name_extra: " << m_display_name_extra << '\n';
    ss << "m_device_type      : " << DeviceTypeToString(m_device_type) + '\n';
    ss << "m_channels        : " << m_channels.ToChString() << '\n';

    ss << "m_sample_rates     : ";
    for (AudSampleRateList::iterator itt = m_sample_rates.begin(); itt != m_sample_rates.end(); ++itt) {
        if (itt != m_sample_rates.begin())
            ss << ',';
        ss << *itt;
    }
    ss << '\n';

    ss << "m_data_formats     : ";
    for (AudDataFormatList::iterator itt = m_data_formats.begin(); itt != m_data_formats.end(); ++itt) {
        if (itt != m_data_formats.begin())
            ss << ',';
        ss << CAudUtil::DataFormatToStr(*itt);
    }
    ss << '\n';

    return ss.str();
}

std::string
CAudDevInfo::DeviceTypeToString(enum AudDeviceType deviceType)
{
    switch (deviceType) {
    case AUD_DEVTYPE_PCM   : return "AUD_DEVTYPE_PCM"   ; break;
    case AUD_DEVTYPE_IEC958: return "AUD_DEVTYPE_IEC958"; break;
    case AUD_DEVTYPE_HDMI  : return "AUD_DEVTYPE_HDMI"  ; break;
    }
    return "INVALID";
}

