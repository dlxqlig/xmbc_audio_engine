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

#include <algorithm>

#include "AudBuffer.h"


#undef ALIGN
#define ALIGN(value, alignment) (((value)+(alignment-1))&~(alignment-1))

void *malloc_aligned(size_t s, size_t align_to)
{
    char *pFull = (char*)malloc(s + align_to + sizeof(char *));
    char *pAlligned = (char *)ALIGN(((unsigned long)pFull + sizeof(char *)), align_to);

    *(char **)(pAlligned - sizeof(char*)) = pFull;

    return(pAlligned);
}

void free_aligned(void *p)
{
    if (!p)
        return;

    char *pFull = *(char **)(((char *)p) - sizeof(char *));
    free(pFull);
}

OMXPacket *AllocPacket(int size)
{
    OMXPacket *pkt = (OMXPacket *)malloc(sizeof(OMXPacket));
    if (pkt) {
        memset(pkt, 0, sizeof(OMXPacket));

        pkt->data = (uint8_t*) malloc(size + 16);
        if(!pkt->data) {
            free(pkt);
            pkt = NULL;
        }
        else {
            memset(pkt->data + size, 0, 16);
            pkt->size = size;
        }
    }
    return pkt;
}

void FreePacket(OMXPacket *pkt)
{
    if (pkt) {
        if(pkt->data) {
            free(pkt->data);
            pkt->data = NULL;
        }
        free(pkt);
        pkt = NULL;
    }
}

CAudBuffer::CAudBuffer() :
        m_buffer(NULL),
        m_buffer_size(0),
        m_buffer_pos(0),
        m_cursor_pos(0)
{
}

CAudBuffer::~CAudBuffer()
{
    DeAlloc();
}

void CAudBuffer::Alloc(const size_t size)
{
    DeAlloc();
    m_buffer = (uint8_t*)malloc_aligned(size, 16);
    m_buffer_size = size;
    m_buffer_pos = 0;
}

void CAudBuffer::ReAlloc(const size_t size)
{
    uint8_t* buffer = (uint8_t*)malloc_aligned(size, 16);
    if (m_buffer) {
        size_t copy = std::min(size, m_buffer_size);
        memcpy(buffer, m_buffer, copy);
        free_aligned(m_buffer);
    }
    m_buffer = buffer;

    m_buffer_size = size;
    m_buffer_pos = std::min(m_buffer_pos, m_buffer_size);
}

void CAudBuffer::DeAlloc()
{
    if (m_buffer)
        free_aligned(m_buffer);
    m_buffer = NULL;
    m_buffer_size = 0;
     m_buffer_pos = 0;
}
