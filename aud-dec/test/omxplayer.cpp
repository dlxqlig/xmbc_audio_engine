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

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <string.h>
#include <utility>
#include <sys/stat.h>

#include <AudRender.h>
#include <AudStream.h>
#include <AudFormat.h>
#include <AudChInfo.h>
#include <AudBuffer.h>

#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

#include "OMXProcess.h"
#include "Logger.h"

using namespace AudRender;

bool m_stop = false;
bool m_has_audio = false;
bool g_abort = false;
pthread_t m_thread;

static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *audio_dec_ctx;
static AVStream *audio_stream = NULL;
static int audio_stream_idx = -1;
static OMXPacket *m_omx_pkt = NULL;

OMXPlayerAudio *m_audio_player = NULL;
CAudRender* m_render = NULL;
CAudStream* m_stream = NULL;

char* m_filename = NULL;
AudDataFormat format = AUD_FMT_S16LE;
int sample_rate = 8000;
int encoded_sample_rate = 0;
bool m_eof = false;
bool m_send_eos = false;
CAudChInfo ch_info(CH_LAYOUT_2_0);
CAudChInfo ch_info_6(CH_LAYOUT_5_1);
CAudChInfo ch_info_8(CH_LAYOUT_7_1);

void Sleep(int dwMilliSeconds)
{
    struct timespec req;
    req.tv_sec = dwMilliSeconds / 1000;
    req.tv_nsec = (dwMilliSeconds % 1000) * 1000000;

    while ( nanosleep(&req, &req) == -1 && errno == EINTR && (req.tv_nsec > 0 || req.tv_sec > 0));
}

static void* write_audio_frames_render(void* pcontext)
{
    OMXPacket *pkt = NULL;
    int added = 0, space = 0;

    while (1) {

        if(g_abort)
            break;

        if (m_eof && (0 == m_audio_player->GetOutCached()) && (0 == m_audio_player->GetCached()))
            break;

        pkt = m_audio_player->GetData();
        if (!pkt) {
            Sleep(10);
            continue;
        }

        int pkt_size = pkt->size;
        while (pkt_size > 0 && m_stream) {

            space = m_stream->GetSpace();
            unsigned int samples = std::min(pkt_size, space);
            if (!samples) {
                Sleep(10);
                continue;
            }

            added = m_stream->AddData(pkt->data, pkt->size);
            printf("^^^^^^^^added = %d\n", added);
            pkt_size -= added;
        }

        if (pkt) {
            FreePacket(pkt);
        }
    }

    return (void*)0;
}

void sig_handler(int s)
{
    if (s==SIGINT && !g_abort) {
        signal(SIGINT, SIG_DFL);
        g_abort = true;
        return;
    }
    signal(SIGABRT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    abort();
}

bool file_exist(const std::string& path)
{
    struct stat buf;
    int error = stat(path.c_str(), &buf);
    return !error || errno != ENOENT;
}

int open_codec_context(int *stream_idx,
                       AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        printf("failed to find %s stream in input file '%s'\n",
                av_get_media_type_string(type), m_filename);
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];

        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            printf("failed to find %s codec\n", av_get_media_type_string(type));
            return ret;
        }

        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            printf("failed to open %s codec\n", av_get_media_type_string(type));
            return ret;
        }
    }

    return 0;
}

int init_av_context() {

    av_register_all();
    if (m_filename == NULL) {
        fprintf(stderr, "file name is null\n");
        return -1;
    }
    if (avformat_open_input(&fmt_ctx, m_filename, NULL, NULL) < 0) {
        fprintf(stderr, "failed to avformat_open_input %s\n", m_filename);
        return -1;
    }
    printf("init_av_context internal start 1\n");
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "faied to avformat_find_stream_info\n");
        return -1;
    }
    printf("init_av_context internal start 2\n");
    if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dec_ctx = audio_stream->codec;
    }
    else {
        return -1;
    }
    printf("init_av_context internal start 3\n");
    av_dump_format(fmt_ctx, 0, m_filename, 0);

    return 0;
}

int create_stream()
{
    switch (audio_dec_ctx->sample_fmt) {
    case AV_SAMPLE_FMT_S16:
        format = AUD_FMT_S16LE;
        break;
    case AV_SAMPLE_FMT_S16P:
        format = AUD_FMT_S16LE;
        break;
    case AV_SAMPLE_FMT_S32:
        format = AUD_FMT_S32LE;
        break;
    case AV_SAMPLE_FMT_FLT:
        format = AUD_FMT_FLOAT;
        break;
    case AV_SAMPLE_FMT_FLTP:
        format = AUD_FMT_FLOAT;
        break;
    default:
        printf("wrong sample format[%d].\n", audio_dec_ctx->sample_fmt);
        return -1;
    }

    // TODO:
    //format = AUD_FMT_S16LE;
    if (audio_dec_ctx->channels == 6) {
        ch_info = ch_info_6;
    }
    else if (audio_dec_ctx->channels == 8) {
        ch_info = ch_info_8;
    }

    sample_rate = audio_dec_ctx->sample_rate;
    m_stream = m_render->MakeStream(format, sample_rate, encoded_sample_rate, ch_info, AUDSTREAM_PAUSED);
    if (!m_stream){
        printf("failed to create stream.\n");
        return -1;
    }
    m_stream->Resume();

    return 0;

}

OMXPacket *read_packet()
{
    AVPacket pkt;
    OMXPacket *decode_pkt = NULL;
    int result = -1;

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    result = av_read_frame(fmt_ctx, &pkt);
    if(pkt.size <= 0){
        av_free_packet(&pkt);
        m_eof = true;
        return NULL;
    }else if (pkt.stream_index != audio_stream_idx){
        av_free_packet(&pkt);
        return NULL;
    }

    // prepare package for decode
    decode_pkt = AllocPacket(pkt.size);
    if (!decode_pkt){
        m_eof = true;
        av_free_packet(&pkt);
        return NULL;
    }
    decode_pkt->size = pkt.size;
    if (pkt.data) {
        memcpy(decode_pkt->data, pkt.data, pkt.size);
    }

    av_free_packet(&pkt);

    return decode_pkt;
}

int main(int argc, char *argv[])
{

    signal(SIGSEGV, sig_handler);
    signal(SIGABRT, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGINT, sig_handler);

    OMXPacket *decode_pkt = NULL;

    if (optind >= argc) {
        return 0;
    }

    m_filename = argv[optind];
    if(!file_exist(m_filename)) {
        printf("file is not found[%s]", m_filename);
        return 0;
    }

    m_audio_player = new OMXPlayerAudio();
    if (!m_audio_player) {
        printf("failed to create omx player audio\n");
        goto do_exit;
    }

    m_render = new CAudRender();
    if (!m_render) {
        printf("failed to create audio render\n");
        goto do_exit;
    }
    m_render->Initialize();
    printf("init_av_context start\n");
    if (0 != init_av_context()) {
        printf("failed to init av oontext.\n");
        goto do_exit;
    }
    printf("create_stream start\n");
    if (0 != create_stream()) {
        printf("failed to create stream.\n");
        goto do_exit;
    }

    printf("m_audio_player open channels %d, sample_rate %d, bit_rate %d\n",
            audio_dec_ctx->channels, audio_dec_ctx->sample_rate, audio_dec_ctx->bit_rate);
    if(!m_audio_player->Open(audio_dec_ctx->channels, audio_dec_ctx->sample_rate, audio_dec_ctx->bit_rate, 0.0, 0.0)) {
        printf("failed to open audio player.\n");
        goto do_exit;
    }
    
    pthread_create(&m_thread, NULL, &write_audio_frames_render, NULL);

    // main loop for read package from media
    while(!m_stop) {

        if (g_abort) {
            goto do_exit;
        }

        m_omx_pkt = read_packet();

        if (m_omx_pkt)
            m_send_eos = false;

        if (m_eof && !m_omx_pkt){
            if (!m_audio_player->GetCached()){
                Sleep(100);
                continue;
            }

            if (!m_send_eos)
                m_audio_player->SubmitEOS();

            m_send_eos = true;
            if (m_audio_player->IsEOS()){
                Sleep(100);
                continue;
            }
            break;
        }

        if(m_omx_pkt) {
            if(!m_audio_player->AddPacket(m_omx_pkt))
                Sleep(100);
        }
        else {
            if (m_omx_pkt) {
                FreePacket(m_omx_pkt);
                m_omx_pkt = NULL;
            }
        }

    }

do_exit:
    pthread_join(m_thread, NULL);

    m_audio_player->Close();

    if (decode_pkt){
        FreePacket(decode_pkt);
        decode_pkt = NULL;
    }

    if (m_audio_player)
        delete m_audio_player;
    m_audio_player = NULL;

    if (m_render) {
        delete m_render;
        m_render = NULL;
    }

    if (audio_dec_ctx)
        avcodec_close(audio_dec_ctx);
    avformat_close_input(&fmt_ctx);

    printf("have a nice day ;)\n");

    return 0;
}
