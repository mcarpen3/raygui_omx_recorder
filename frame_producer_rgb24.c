#include "frame_producer_rgb24.h"
#include "fifo.h"

void *produce_frames(void *fifo) {
    int *ret = malloc(sizeof(int));
    const int src_w = 800;
    const int src_h = 480;
    const enum AVPixelFormat src_fmt = AV_PIX_FMT_RGB24;
    const int expected_pkt = src_w * src_h * 3;
    int rc;
    AVDictionary *opts = NULL;

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

    av_dict_set(&opts, "pixel_format", "rgb24", 0);
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
    AVPacket *pkt = av_packet_alloc();

    while (av_read_frame(ctx, pkt) == 0) {
        if (pkt->stream_index == 0) {
            if (pkt->size == expected_pkt) {
                if ((fifo_write((fifo_buffer_t *)fifo, pkt->data)) == false) {
                    break;
                }
            }
        }
        av_packet_unref(pkt);
    }
    if (pkt) {
        av_packet_unref(pkt);
        av_packet_free(&pkt);
    }
    avformat_close_input(&ctx);
    avdevice_free_list_devices(&list);
    
    *ret = 0;
    return ret;
}
