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

#include "System.h"
#include "../include/AudFormat.h"

class CAudConvert{

private:
    static unsigned int U8_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S8_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S16LE_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S16BE_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S24LE4_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S24BE4_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S24LE3_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S24BE3_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S32LE_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int S32BE_Float(uint8_t *data, const unsigned int samples, float   *dest);
    static unsigned int DOUBLE_Float(uint8_t *data, const unsigned int samples, float   *dest);

    static unsigned int Float_U8(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S8(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S16LE(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S16BE(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S24NE4(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S24NE3(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S32LE(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S32BE(float   *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_DOUBLE(float   *data, const unsigned int samples, uint8_t *dest);

    static unsigned int S32LE_Float_Neon(uint8_t *data, const unsigned int samples, float *dest);
    static unsigned int S32BE_Float_Neon(uint8_t *data, const unsigned int samples, float *dest);
    static unsigned int Float_S32LE_Neon(float *data, const unsigned int samples, uint8_t *dest);
    static unsigned int Float_S32BE_Neon(float *data, const unsigned int samples, uint8_t *dest);

public:
    typedef unsigned int (*AudConvertToFn)(uint8_t *data, const unsigned int samples, float *dest);
    typedef unsigned int (*AudConvertFrFn)(float *data, const unsigned int samples, uint8_t *dest);

    static AudConvertToFn ToFloat(enum AudDataFormat dataFormat);
    static AudConvertFrFn FrFloat(enum AudDataFormat dataFormat);
};
