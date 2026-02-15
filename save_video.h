#ifndef SAVE_VIDEO_H
#define SAVE_VIDEO_H
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdbool.h>
#include "utl_assert.h"
#include <libavutil/pixdesc.h>


void open_output(AVFormatContext *in_fmt_ctx, AVFormatContext **out_fmt_ctx, AVCodecContext **dec_ctx, AVCodecContext **enc_ctx, AVStream *in_stream);
void close_output(AVFormatContext *out_fmt_ctx, AVCodecContext *dec_ctx, AVCodecContext *enc_ctx);
#endif // SAVE_VIDEO_H
