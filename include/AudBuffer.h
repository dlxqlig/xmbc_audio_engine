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

#include <stdio.h>
#include <string.h>

#include "System.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

typedef enum PARAMETER_TYPE {
    OMX_PARAMETER_INVALID = -1,
    OMX_PARAMETER_SAMPLEFORMAT,
    OMX_PARAMETER_MAX
}PARAMETER_TYPE;

typedef struct OMXPacket
{
    int size;
    uint8_t *data;
} OMXPacket;

void *malloc_aligned(size_t s, size_t alignTo);
void free_aligned(void *p);
OMXPacket *AllocPacket(int size);
void FreePacket(OMXPacket *pkt);

class CAudBuffer
{
private:
    uint8_t *m_buffer;
    size_t m_buffer_size;
    size_t m_buffer_pos;
    size_t m_cursor_pos;

public:
    CAudBuffer();
    ~CAudBuffer();

    void ReAlloc(const size_t size);
    void Alloc(const size_t size);
    void DeAlloc();

    inline size_t Size() { return m_buffer_size; }
    inline size_t Used() { return m_buffer_pos; }
    inline size_t Free() { return m_buffer_size - m_buffer_pos; }
    inline void Empty() { m_buffer_pos = 0; }

    inline void Write(const void *src, const size_t size)
    {
        if (src && size <= m_buffer_size) {
            memcpy(m_buffer, src, size);
            m_buffer_pos = 0;
        }
    }

    inline void Push(const void *src, const size_t size)
    {
        if (src && size <= m_buffer_size - m_buffer_pos) {
            memcpy(m_buffer + m_buffer_pos, src, size);
            m_buffer_pos += size;
        }
    }

    inline void UnShift(const void *src, const size_t size)
    {
        if (src && size < m_buffer_size - m_buffer_pos) {
            memmove(m_buffer + size, m_buffer, m_buffer_size - size);
            memcpy(m_buffer, src, size);
            m_buffer_pos += size;
        }
    }

    inline void* Take(const size_t size)
    {
        void* ret = NULL;
        if (size <= m_buffer_size - m_buffer_pos) {
            ret = m_buffer + m_buffer_pos;
            m_buffer_pos += size;
        }
        return ret;
    }

    inline void* Raw(const size_t size)
    {
        if (size <= m_buffer_size)
            return m_buffer;
        else
            return NULL;
    }

    inline void Read(void *dst, const size_t size)
    {
        if (dst && size <= m_buffer_size)
            memcpy(dst, m_buffer, size);
    }

    inline void Pop(void *dst, const size_t size)
    {
        if (size <= m_buffer_pos) {
            m_buffer_pos -= size;
            if (dst)
                memcpy(dst, m_buffer + m_buffer_pos, size);
        }
    }

    inline void Shift(void *dst, const size_t size)
    {
        if (size <= m_buffer_pos) {
            if (dst)
                memcpy(dst, m_buffer, size);
            if ((m_buffer_pos != size) && size <= m_buffer_pos)
                memmove(m_buffer, m_buffer + size, m_buffer_size - size);
            if (size <= m_buffer_pos)
                m_buffer_pos -= size;
        }
    }

    inline void CursorReset()
    {
        m_cursor_pos = 0;
    }

    inline size_t CursorOffset()
    {
        return m_cursor_pos;
    }

    inline bool CursorEnd()
    {
        return m_cursor_pos == m_buffer_pos;
    }

    inline void CursorSeek(const size_t pos )
    {
        if (pos <= m_buffer_size) {
            m_cursor_pos = pos;
        }
    }

    inline void* CursorRead(const size_t size)
    {
        uint8_t* out = NULL;
        if (m_cursor_pos + size <= m_buffer_pos) {
            out = m_buffer + m_cursor_pos;
            m_cursor_pos += size;
        }
        return out;
    }
};
