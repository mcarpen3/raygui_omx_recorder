#include "save_video.h"

void open_output(AVFormatContext **out_fmt_ctx, AVCodecContext **dec_ctx, AVCodecContext **enc_ctx, AVStream *in_stream)
{
    time_t rawtime;
    struct tm *tm;
    char output_filename[256];
    AVStream *out_stream;
    AVCodec *decoder;
    AVCodec *encoder;
    AVDictionary *out_fmt_ctx_opts;
    bool success;
    int rc;

    /** Get the RAW video decoder */
    decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
    success = (NULL != decoder);
    utl_assert_continue_local(success);

    if (success)
    {
        /**
          * Get the h264 encoder
          */
        encoder = avcodec_find_encoder_by_name("h264_omx");
        success = (NULL != encoder);
        utl_assert_continue_local(success);
    }

    if (success)
    {
        *dec_ctx = avcodec_alloc_context3(decoder);
        success = (NULL != *dec_ctx);
        utl_assert_continue_local(success);
    }

    if (success)
    {
        /**
          * Setup the encoder context
          */
        *enc_ctx = avcodec_alloc_context3(encoder);
        success = (NULL != *enc_ctx);
        utl_assert_continue_local(success);
    }

    if (success)
    {
        rc = avcodec_parameters_to_context(*dec_ctx, in_stream->codecpar);
        success = (0 >= rc);
        utl_assert_continue_local(success);
    }

    if (success)
    {
        (*enc_ctx)->time_base = (AVRational){1,30};
        (*enc_ctx)->bit_rate = 4000000;
        (*enc_ctx)->framerate = (AVRational){30,1};
        (*enc_ctx)->gop_size = 12;
        (*enc_ctx)->pix_fmt = AV_PIX_FMT_YUV420P;
        (*enc_ctx)->width = in_stream->codecpar->width;
        (*enc_ctx)->height = in_stream->codecpar->height;
    }
    
    if (success)
    {
        rc = avcodec_open2(*dec_ctx, decoder, NULL);
        success = (0 == rc);
        utl_assert_continue_local(success);
    }

    if (success)
    {
        rc = avcodec_open2(*enc_ctx, encoder, NULL);
        success = (0 == rc);
        utl_assert_continue_local(success);
    }
    
    if (success)
    {
        /**
          * Get the current time
          */
        time(&rawtime);
        tm = gmtime(&rawtime);

        rc = sprintf(output_filename, "%s%04d%02d%02d%02d%02d%02d.%s",
                "/home/pi/Video/",
                1900 + tm->tm_year,
                tm->tm_mon,
                tm->tm_mday,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec,
                "mp4"
                );
        success = (-1 != rc);
    }

    // setup the output context
    if (success)
    {
        rc = avformat_alloc_output_context2(&(*out_fmt_ctx), NULL, NULL, output_filename);
        success = (NULL != out_fmt_ctx && rc == 0);
    }
   
    // create an output stream
    if (success)
    {
        out_stream = avformat_new_stream(*out_fmt_ctx, NULL);
        success = (NULL != out_stream);
        utl_assert_continue_local(success);
    }
    
    if (success)
    {
        /**
          * Copy the encoder parameters to the out_stream
          */
        rc = avcodec_parameters_from_context(out_stream->codecpar, *enc_ctx);
        success = (0 >= rc);
        utl_assert_continue_local(success);
    }

    if (success)
    {
        out_stream->codecpar->codec_tag = 0;
    }

    if (success)
    {
        av_dump_format(*out_fmt_ctx, 0, output_filename, 1);
    } 
        
    if (success)
    {
        rc = avio_open(&(*out_fmt_ctx)->pb, output_filename, AVIO_FLAG_WRITE);
        success = (rc >= 0);
        utl_assert_continue_local(success);
    }

    if (success)
    { 
        /**
          * Write the output header
          */
        out_fmt_ctx_opts = NULL;
        av_dict_set_int(&out_fmt_ctx_opts, "moov_size", 10000000, 0);
        rc = avformat_write_header(*out_fmt_ctx, &out_fmt_ctx_opts);
        utl_assert_continue_local(success);
        av_dict_free(&out_fmt_ctx_opts);
    }

    if (!success)
    {
        fprintf(stderr, "ERROR occurred!!!!! freeing resources\n");
        if (*enc_ctx) 
        {
            avcodec_free_context(&(*enc_ctx));
            enc_ctx = NULL;
        }
        if (*dec_ctx) 
        {
            avcodec_free_context(&(*dec_ctx));
            dec_ctx = NULL;
        }
        if (*out_fmt_ctx) 
        {
            avformat_free_context(*out_fmt_ctx);
            *out_fmt_ctx = NULL;
        }
    }
    return;
}

void close_output(AVFormatContext *out_fmt_ctx, AVCodecContext *dec_ctx, AVCodecContext *enc_ctx)
{
    av_write_trailer(out_fmt_ctx);
    avio_closep(&out_fmt_ctx->pb);
    avformat_free_context(out_fmt_ctx);
    avcodec_free_context(&enc_ctx);
    avcodec_free_context(&dec_ctx);
} 
