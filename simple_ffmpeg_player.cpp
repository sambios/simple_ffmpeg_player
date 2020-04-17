#include <stdio.h>
#include <iostream>

#define __STDC_CONSTANT_MACROS

/**
 * In order to verify the bm_ffmpeg output format whether is NV12.
 */

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include "getopt.h"

#ifdef __cplusplus
};
#endif
#endif

#if USE_DUMPYUV_FILE
static void handle_frame(AVFrame *pFrame, FILE *fp_yuv)
{

    uint32_t y_size = pFrame->width * pFrame->height;
    if (pFrame->format == AV_PIX_FMT_YUV420P || pFrame->format == AV_PIX_FMT_YUVJ420P) {

        fwrite(pFrame->data[0], 1, y_size, fp_yuv);    //Y
                    fwrite(pFrame->data[1], 1, y_size / 4, fp_yuv);  //U
                    fwrite(pFrame->data[2], 1, y_size / 4, fp_yuv);  //V
    } else if (pFrame->format == AV_PIX_FMT_NV12) {

        fwrite(pFrame->data[0], 1, y_size, fp_yuv);    //Y
                    fwrite(pFrame->data[1], 1, y_size >> 1, fp_yuv); //UV
    }
}
#else

#include "opencv2/opencv.hpp"
cv::Mat avframe_to_cvmat(const AVFrame *src)
{
    uint8_t /**src_data[4],*/ *dst_data[4];
    int /*src_linesize[4],*/ dst_linesize[4];
    int src_w, src_h, dst_w, dst_h;

    struct SwsContext *convert_ctx = NULL;
    enum AVPixelFormat src_pix_fmt = (enum AVPixelFormat)src->format;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;

    src_w = src->width;
    dst_w = src->width;
    src_h = src->height;
    dst_h = src->height;

    cv::Mat out_img = cv::Mat(dst_h, dst_w, CV_8UC3);

    av_image_fill_arrays(dst_data, dst_linesize, out_img.data, dst_pix_fmt, dst_w, dst_h, 1);

    convert_ctx = sws_getContext(src_w, src_h, src_pix_fmt, dst_w, dst_h, dst_pix_fmt,
                                 SWS_FAST_BILINEAR, NULL, NULL, NULL);

    sws_scale(convert_ctx, src->data, src->linesize, 0, dst_h,
              dst_data, dst_linesize);

    sws_freeContext(convert_ctx);

    return out_img;
}

static void handle_frame(AVFrame *pFrame)
{
    cv::Mat img = avframe_to_cvmat(pFrame);
    cv::imshow("simple_ffmpeg_player", img);
    cv::waitKey(1);
}
#endif


int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame;
    unsigned char *out_buffer;
    AVPacket *packet;
    int y_size;
    int ret, got_picture, got_first_picture = 0;
    int frame_idx = 0;
    int frame_num = 100;

    char *filepath = NULL;

#if USE_DUMPYUV_FILE
    FILE *fp_yuv = fopen("output.yuv", "wb+");
#endif

    int ch;
    int option_index = 0;
    int cache_count = 0;
    struct option long_options[] =
            {
                    {0, 0, 0, 0},
            };

    while ((ch = getopt_long(argc, argv, "i:n:", long_options, &option_index)) != -1) {

        switch (ch) {
            case 0:
                if (optarg) {
                    //todo
                }
                break;
            case 'i':
                filepath = optarg;
                break;
            case 'n': {
                int num = atoi(optarg);
                if (num > 0) {
                    frame_num = num;
                }
            }
                break;
        }
    }

    if (NULL == filepath) {
        printf("Please input file with -i option.\n");
        return -1;
    }


    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
        printf("Couldn't open input stream, file=%s\n", filepath);
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }

    if (videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }

    //SOC, default is nv12, so don't need to set options.
    AVDictionary *opts = NULL;

#if USE_BM_CODEC
    // cbcr_interleave=0: OUTPUT is yuv420p;
    // cbcr_interleave=1: OUTPUT is NV12
    av_dict_set_int(&opts, "cbcr_interleave", 0, 0);
    av_dict_set(&opts, "handle_packet_loss", "1", 0);
    av_dict_set(&opts, "pcie_no_copyback", "0", 0);
#endif

    if (avcodec_open2(pCodecCtx, pCodec, &opts) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();

    packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //Output Info-----------------------------
    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx, 0, filepath, 0);

    while (1) {

        if (frame_idx > frame_num) {
            break;
        }

        if (av_read_frame(pFormatCtx, packet) < 0) {
            break;
        }

        if (packet->stream_index == videoindex) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                printf("Decode Error.\n");
                av_packet_unref(packet);
                return -1;
            }

            cache_count++;
            if (got_first_picture == 0) {
                std::cout << "got_picture = " << got_picture << ", cache=" << cache_count << std::endl;
            }

            if (got_picture) {
                frame_idx++;
                got_first_picture = 1;

                pFrame->data[4] = pFrame->data[5] = (uint8_t *) 0x11223344;

#ifdef DEBUG
                AVFrame *frame_new = av_frame_alloc();
                av_frame_ref(frame_new, pFrame);

                av_frame_unref(frame_new);
                av_frame_free(&frame_new);
#endif //!DEBUG

                if (frame_idx == 1) {
                    printf("decode output format: %s\n", av_get_pix_fmt_name((enum AVPixelFormat) pFrame->format));
                }

#if USE_DUMPYUV_FILE
                handle_frame(pFrame, fp_yuv);
#else
                handle_frame(pFrame);
#endif
            }
        }

        av_packet_unref(packet);
    }

    printf("Normal:decoded %d frames\n", frame_idx);

    //flush decoder
    int flush_frame_num = 0;
    while (1) {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if (ret < 0)
            break;
        if (!got_picture)
            break;

        flush_frame_num++;

#if USE_DUMPYUV_FILE
        handle_frame(pFrame, fp_yuv);
#else
        handle_frame(pFrame);
#endif
    }

    printf("Flush: decoded %d frame!", flush_frame_num);

#if USE_DUMPYUV_FILE
    fclose(fp_yuv);
#endif

    av_freep(&packet);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}
