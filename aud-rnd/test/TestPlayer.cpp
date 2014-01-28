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
 *  You can also contact with mail(dlxqlig@gmail.com)
 */

extern "C" {

#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif
#define INT64_C(val) val##64
#define _64BITARG_ "I64"
#define PRId64 _64BITARG_"d"
#define PRIu64 -64BITARG_"u"
#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include "AudRender.h"
#include "AudStream.h"
#include "AudFormat.h"
#include "AudChInfo.h"

using namespace AudRender;
static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *audio_dec_ctx;
static AVStream *audio_stream = NULL;
static const char *src_filename = NULL;
static int audio_stream_idx = -1;
static AVFrame *frame = NULL;
static AVPacket pkt;
static int audio_frame_count = 0;

static CAudRender* render = NULL;
static CAudStream* stream = NULL;
static bool start_stream = false;
pthread_t thread_h;
struct SwrContext* sw_convert = NULL;
unsigned char* convert_buffer = NULL;
const size_t convert_buffer_size = 288000 * 2 + FF_INPUT_BUFFER_PADDING_SIZE;

static void* write_audio_frames_render(void* pcontext)
{
    printf("@@@@@@write_audio_frames_render start\n");

    convert_buffer = (unsigned char*)malloc_aligned(convert_buffer_size, 16);
    memset(convert_buffer, 0, convert_buffer_size);

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    unsigned int space = stream->GetSpace();
    while(1) {
        if (av_read_frame(fmt_ctx, &pkt) >= 0) {

            AVPacket orig_pkt = pkt;
            if (pkt.stream_index != audio_stream_idx) {
                av_free_packet(&orig_pkt);
                continue;
            }

            do {
                int got_frame;
                int ret = avcodec_decode_audio4(audio_dec_ctx, frame, &got_frame, &pkt);
                if (ret < 0) {
                    fprintf(stderr, "Error decoding audio frame\n");
                    break;
                }
                printf("Decode(%p,%d) planar[%d] len=%d format=%d(%d) samples=%d linesize=%d,%d,%d,%d,%d,%d,%d,%d date=%p,%p,%p,%p,%p,%p,%p,%p edata=%p,%p,%p,%p,%p,%p,%p,%p\n",
                          pkt.data, pkt.size, av_sample_fmt_is_planar(audio_dec_ctx->sample_fmt), ret,
                          0, (int)frame->format,
                          (int)frame->nb_samples,
                          frame->linesize[0],
                          frame->linesize[1],
                          frame->linesize[2],
                          frame->linesize[3],
                          frame->linesize[4],
                          frame->linesize[5],
                          frame->linesize[6],
                          frame->linesize[7],
                          frame->data[0],
                          frame->data[1],
                          frame->data[2],
                          frame->data[3],
                          frame->data[4],
                          frame->data[5],
                          frame->data[6],
                          frame->data[7],
                          frame->extended_data[0],
                          frame->extended_data[1],
                          frame->extended_data[2],
                          frame->extended_data[3],
                          frame->extended_data[4],
                          frame->extended_data[5],
                          frame->extended_data[6],
                          frame->extended_data[7]);

                if (got_frame) {
                    int linesize1, linesize2;
                    size_t data_size1 = av_samples_get_buffer_size(&linesize1, audio_dec_ctx->channels, frame->nb_samples, audio_dec_ctx->sample_fmt, 1);
                    size_t data_size2 = av_samples_get_buffer_size(&linesize2, audio_dec_ctx->channels, frame->nb_samples, AV_SAMPLE_FMT_FLT, 1);

                    bool is_planar = av_sample_fmt_is_planar(audio_dec_ctx->sample_fmt);
                    size_t unpadded_linesize = frame->nb_samples * (av_get_bytes_per_sample((AVSampleFormat)frame->format))*audio_dec_ctx->channels;

                    printf("audio_frame n:%d data_size1 %d nb_samples:%d pts:%s planar:%d\n",
                           audio_frame_count++, data_size1, frame->nb_samples,
                           av_ts2timestr(frame->pts, &audio_dec_ctx->time_base), is_planar);
                    printf("ch[%d] format:%d data_size1:%d data_size2:%d linesize1:%d linesize2:%d nb_samples %d ret %d unpadded_linesize[%d]\n",
                            audio_dec_ctx->channels, audio_dec_ctx->sample_fmt, data_size1, data_size2,
                            linesize1, linesize2, frame->nb_samples, ret, unpadded_linesize);
                    int size_tmp;
                    if (is_planar) {
                        size_tmp = data_size2;
                    }
                    else {
                        size_tmp = unpadded_linesize;
                    }

                    unsigned int added = 0;
                    while (size_tmp > 0) {
                        space = stream->GetSpace();
                        unsigned int samples = std::min((unsigned int)size_tmp, space);

                        if (!samples) {
                            //printf("####there is no space \n");
                            sleep(0.1);
                            continue;
                        }
                        printf("####unpadded_linesize[%d] data_size1[%d] space[%d], samples[%d] \n",
                                unpadded_linesize, data_size1, space, samples);

                        if (is_planar) {
                            if (!sw_convert) {
                                //printf("#### channels[%d] sample_fmt[%d] sample_rate[%d] \n", audio_dec_ctx->channels, audio_dec_ctx->sample_fmt, audio_dec_ctx->sample_rate);
                                sw_convert = swr_alloc_set_opts(NULL,
                                            av_get_default_channel_layout(audio_dec_ctx->channels),
                                            AV_SAMPLE_FMT_FLT,
                                            audio_dec_ctx->sample_rate,
                                            av_get_default_channel_layout(audio_dec_ctx->channels),
                                            audio_dec_ctx->sample_fmt,
                                            audio_dec_ctx->sample_rate,
                                            0, NULL);
                            }
                            if (!sw_convert || swr_init(sw_convert) < 0) {
                                printf("####failed to swr_init.\n");
                                break;
                            }

                            unsigned char *out_planes[]={convert_buffer + 0 * linesize2,convert_buffer + 1 * linesize2, convert_buffer + 2 * linesize2,
                                convert_buffer + 3 * linesize2, convert_buffer + 4 * linesize2, convert_buffer + 5 * linesize2, convert_buffer + 6 * linesize2,
                                convert_buffer + 7 * linesize2,
                            };

                            if (swr_convert(sw_convert, out_planes, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples) < 0) {
                                printf("####failed to sw_convert.\n");
                                break;
                            }
                            added = stream->AddData(convert_buffer, data_size2);
                            //printf("####AddData [%d].\n", added);
                        }
                        else
                        {
                            added = stream->AddData(frame->extended_data[0], samples);
                        }
                        size_tmp -= added;
                    }

                }

                pkt.data += ret;
                pkt.size -= ret;
            } while (pkt.size > 0);

            av_free_packet(&orig_pkt);
        }else{
            break;
        }
    }

    return (void*)0;
}

static int open_codec_context(int *stream_idx,
                              AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        printf("failed to find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
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

int main (int argc, char **argv)
{
    int ret = 0;
    AudDataFormat format = AUD_FMT_S16LE;
    int sample_rate = 8000;
    int encoded_sample_rate = 0;
    CAudChInfo ch_info(CH_LAYOUT_2_0);
    CAudChInfo ch_info_6(CH_LAYOUT_5_1);
    CAudChInfo ch_info_8(CH_LAYOUT_7_1);

    if (argc != 2) {
        printf("pls input media file.\n");
        exit(1);
    }
    src_filename = argv[1];

    render = new CAudRender();
    if (!render) {
        printf("failed to create AudRender.\n");
        goto end;;
    }
    render->Initialize();

    av_register_all();
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "failed to avformat_open_input %s\n", src_filename);
        exit(1);
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "faied to avformat_find_stream_info\n");
        exit(1);
    }

    if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dec_ctx = audio_stream->codec;

    }

    av_dump_format(fmt_ctx, 0, src_filename, 0);
    if (!audio_stream ) {
        printf("failed to av_dump_format.\n");
        ret = 1;
        goto end;
    }

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
        ret = 1;
        goto end;
    }

    if (audio_dec_ctx->channels == 6) {
        ch_info = ch_info_6;
    }
    else if (audio_dec_ctx->channels == 8) {
        ch_info = ch_info_8;
    }

    sample_rate = audio_dec_ctx->sample_rate;
    stream = render->MakeStream(format, sample_rate, encoded_sample_rate, ch_info, AUDSTREAM_PAUSED);
    if (!stream){
        printf("failed to create stream.\n");
        goto end;
    }

    frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "failed to avcodec_alloc_frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (!start_stream) {
        stream->Resume();
    }

    pthread_create(&thread_h, NULL, &write_audio_frames_render, NULL);
    pthread_join(thread_h, NULL);

    printf("work is over!\n");

end:
    if (render) {
        delete render;
        render = NULL;
    }
    if (audio_dec_ctx)
        avcodec_close(audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_free(frame);
    return ret < 0;
}
