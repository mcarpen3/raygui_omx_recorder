#include "frame_producer.h"
#include "fifo.h"
#include "save_video.h"
//SWS_PARAM_DEFAULT
static bool check_codec_rcv_rc(int rc)
{
    return AVERROR(EAGAIN) == rc || 0 == rc;
}

void *produce_frames(void *fifo) {
    av_log_set_level(AV_LOG_DEBUG);
    int *ret = malloc(sizeof(int));
    const int src_w = 800;
    const int src_h = 480;
    const int dst_w = 800;
    const int dst_h = 480;
    const enum AVPixelFormat src_fmt = AV_PIX_FMT_YUV420P;
    const enum AVPixelFormat dst_fmt = AV_PIX_FMT_RGBA;
    const int luma_size = src_w * src_h;
    const int uv_w = src_w / 2;
    const int uv_h = src_h / 2;
    const int chroma_size = uv_w * uv_h;
    const int expected_pkt = luma_size + 2 * chroma_size;
    AVFormatContext *out_fmt_ctx = NULL;
    AVCodecContext *dec_ctx = NULL;
    AVCodecContext *enc_ctx = NULL;
    AVPacket *pkt, *enc_pkt = NULL;
    AVFrame  *dec_frame;

    if (NULL == fifo) {
        fprintf(stderr, "fifo_buffer_t malloc failed\n");
        *ret = 1;
        return ret;
    }
    int rc;
    AVDictionary *opts = NULL;

    uint8_t *dst_data[4] = {0};
    int dst_linesize[4] = {0};
    // allocate an image space for the reformatted/scaled video
    if (av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, dst_fmt, 1) < 0) {
        fprintf(stderr, "av_image_alloc failed\n");
        *ret = 1;
        return ret;
    } else {
        av_assert0(((src_w * 4) == dst_linesize[0]));
    }

    avdevice_register_all();
    av_log_set_level(AV_LOG_DEBUG);
    AVInputFormat *fmt = av_find_input_format("v4l2");
    AVFormatContext *ctx = NULL; 
    AVDeviceInfoList *list = NULL;
    int nb_devs = avdevice_list_input_sources(fmt, NULL, NULL, &list);
    (void)nb_devs;
    for (int i = 0; i < list->nb_devices; ++i) {
        printf("AVDeviceInfo[%d]{name: \"%s\", desc: \"%s\"}\n", i, list->devices[i]->device_name, list->devices[i]->device_description);
    }

    av_dict_set(&opts, "pixel_format", "yuv420p", 0);
    av_dict_set(&opts, "video_size", "800x480", 0);
    av_dict_set(&opts, "framerate", "30", 0);
    AVDictionaryEntry *t = NULL;
    while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX))) {
       printf("key=%s, value=%s\n", t->key, t->value); 
    }

    
    printf("number of devices = %d\n", list->nb_devices);
    printf("opening %s\n", list->devices[0]->device_name);
    
    avformat_open_input(&ctx, list->devices[0]->device_name, fmt, &opts);
    rc = avformat_find_stream_info(ctx, NULL);
    if (rc != 0) { fprintf(stderr, "ERROR avformat_find_stream_info %d\n", rc); *ret = 1; return ret; }
    av_dump_format(ctx, 0, list->devices[0]->device_name, 0);
    av_dict_free(&opts);
    pkt = av_packet_alloc();
    enc_pkt = av_packet_alloc();
    dec_frame = av_frame_alloc();
    
    struct SwsContext *sws = sws_getContext(src_w, src_h, src_fmt, dst_w, dst_h, dst_fmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    while (av_read_frame(ctx, pkt) == 0) 
    {
        if (pkt->stream_index == 0) 
        {
            if (pkt->size == expected_pkt) 
            {
                if (INV == fifo_recorder_state(fifo))
                {
                    fifo_recorder_state_set(fifo, IDLE);
                }
                const uint8_t *src_slices[4] = { pkt->data, pkt->data + luma_size, pkt->data + luma_size + chroma_size, NULL };
                int src_strides[4] = { src_w, uv_w, uv_w, 0 };
                rc = sws_scale(sws, src_slices, src_strides, 0, src_h, dst_data, dst_linesize);
                if ((fifo_write((fifo_buffer_t *)fifo, dst_data[0])) == false) {
                    fifo_recorder_state_set(fifo, INV);
                    break;
                }
                if (IDLE == fifo_recorder_state(fifo) && CLIENT_REC == fifo_client_state(fifo))
                {
                    fifo_recorder_state_set(fifo, INV);
                    open_output(ctx, &out_fmt_ctx, &dec_ctx, &enc_ctx, ctx->streams[pkt->stream_index]);
                    if (NULL != out_fmt_ctx)
                    {
                        fifo_recorder_state_set(fifo, REC);
                    }
                }
                else if (REC == fifo_recorder_state(fifo) && CLIENT_REC == fifo_client_state(fifo) && NULL != out_fmt_ctx)
                {
                    if (pkt->pts == AV_NOPTS_VALUE || pkt->dts == AV_NOPTS_VALUE)
                    {
                        utl_assert_continue_local(false);
                        continue;
                    }
                    pkt->dts -= ctx->streams[pkt->stream_index]->start_time;
                    pkt->pts -= ctx->streams[pkt->stream_index]->start_time;

                    rc = avcodec_send_packet(dec_ctx, pkt);
                    if (rc != 0)
                    {
                        fprintf(stderr, "ERROR! avcodec_send_packet decoder %s\n", av_err2str(rc));
                        break;
                    }
                    while ((rc = avcodec_receive_frame(dec_ctx, dec_frame)) == 0)
                    {
                        dec_frame->pts = av_rescale_q(pkt->pts, ctx->streams[pkt->stream_index]->time_base, enc_ctx->time_base);
                        rc = avcodec_send_frame(enc_ctx, dec_frame);
                        if (rc != 0)
                        {
                            fprintf(stderr, "ERROR! avcodec_send_frame encoder %s\n", av_err2str(rc));
                            break;
                        }
                        while ((rc = avcodec_receive_packet(enc_ctx, enc_pkt)) == 0)
                        {
                            enc_pkt->stream_index = pkt->stream_index;
                            av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_fmt_ctx->streams[pkt->stream_index]->time_base);
                            printf("pts: %"PRIi64", dts: %"PRIi64", dur: %"PRIi64"\n", pkt->pts, pkt->dts, pkt->duration);
                            rc = av_interleaved_write_frame(out_fmt_ctx, enc_pkt);
                            if (0 != rc)
                            {
                                fprintf(stderr, "ERROR! av_interleaved_write_frame %s\n", av_err2str(rc));
                            }
                            av_packet_unref(enc_pkt);
                        }
                        av_frame_unref(dec_frame);
                        if(!check_codec_rcv_rc(rc))
                        {
                            fprintf(stderr, "ERROR! avcodec_receive_packet %s\n", av_err2str(rc));
                        }
                    }
                    if (!check_codec_rcv_rc(rc))
                    {
                        fprintf(stderr, "ERROR! avcodec_receive_frame %s\n", av_err2str(rc));
                    }
                }
                else if (REC == fifo_recorder_state(fifo) && CLIENT_STOP == fifo_client_state(fifo))
                {
                    fifo_recorder_state_set(fifo, INV);
                    close_output(out_fmt_ctx, dec_ctx, enc_ctx);
                    out_fmt_ctx = NULL;
                    dec_ctx = NULL;
                    enc_ctx = NULL;
                    fifo_recorder_state_set(fifo, IDLE);
                }
            }
        }
        av_packet_unref(pkt);
    }
    if (pkt) {
        av_packet_unref(pkt);
        av_packet_free(&pkt);
    }
    if (enc_pkt)
    {
        av_packet_unref(enc_pkt);
        av_packet_free(&enc_pkt);
    }
    if (dec_frame)
    {
        av_frame_unref(dec_frame);
        av_frame_free(&dec_frame);
    }
    av_freep(&dst_data[0]);
    sws_freeContext(sws);
    avformat_close_input(&ctx);
    avdevice_free_list_devices(&list);
    *ret = 0;
    return ret;
}
