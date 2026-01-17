#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>

static int open_fb(uint8_t **fbp, size_t *fb_size, int *w, int *h, int *stride) {
    int fb = open("/dev/fb0", O_RDWR);
    if (fb < 0) { perror("open /dev/fb0"); return -1; }

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo) < 0) { perror("FBIOGET_FSCREENINFO"); close(fb); return -1; }
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0) { perror("FBIOGET_VSCREENINFO"); close(fb); return -1; }

    *w = vinfo.xres;
    *h = vinfo.yres;
    *stride = finfo.line_length;
    *fb_size = finfo.smem_len;

    *fbp = mmap(NULL, *fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
    if (*fbp == MAP_FAILED) { perror("mmap fb"); close(fb); return -1; }

    // keep fb fd open for lifetime; omit close(fb) here or store it
    return fb;
}

int main(void) {
    const int W = 800, H = 480;
    const int BYTES_PER_LINE = W * 4;
    const int FRAME_BYTES = BYTES_PER_LINE * H;
    const int src_w = 800;
    const int src_h = 480;
    const int dst_w = 400;
    const int dst_h = 240;
    const enum AVPixelFormat src_fmt = AV_PIX_FMT_YUV420P;
    const enum AVPixelFormat dst_fmt = AV_PIX_FMT_BGRA;
    const int y_size = src_w * src_h;
    const int uv_w = src_w / 2;
    const int uv_h = src_h / 2;
    const int uv_size = uv_w * uv_h;
    const int expected_pkt = y_size + 2 * uv_size;

    int rc, frame_buffer;
    uint8_t *fbp = NULL;
    size_t fb_size = 0;
    int fb_w=0, fb_h=0, fb_stride=0;
    AVDictionary *opts = NULL;
    int fb_fd = open_fb(&fbp, &fb_size, &fb_w, &fb_h, &fb_stride);
    if (fb_fd < 0) return 1;
    

    uint8_t *dst_data[4] = {0};
    int dst_linesize[4] = {0};
    if (av_image_alloc(dst_data, dst_linesize, fb_w, fb_h, dst_fmt, 1) < 0) {
        fprintf(stderr, "av_image_alloc failed\n");
        return 1;
    }
    avdevice_register_all();
    av_log_set_level(AV_LOG_DEBUG);
    AVInputFormat *fmt = av_find_input_format("v4l2");
    AVFormatContext *ctx = NULL; 
    AVDeviceInfoList *list = NULL;
    int nb_devs = avdevice_list_input_sources(fmt, NULL, NULL, &list);
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
    if (rc != 0) { fprintf(stderr, "ERROR avformat_find_stream_info %d\n", rc); return 1; }
    av_dump_format(ctx, 0, list->devices[0]->device_name, 0);
    av_dict_free(&opts);
    AVPacket *pkt = av_packet_alloc();
    struct SwsContext *sws = sws_getContext(src_w, src_h, src_fmt, dst_w, dst_h, dst_fmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    while (av_read_frame(ctx, pkt) == 0) {
        if (pkt->stream_index == 0) {
            if (pkt->size == expected_pkt) {
                const uint8_t *src_slices[4] = { pkt->data, pkt->data + y_size, pkt->data + y_size + uv_size, NULL };
                int src_strides[4] = { src_w, uv_w, uv_w, 0 };
                sws_scale(sws, src_slices, src_strides, 0, src_h, dst_data, dst_linesize);

                memcpy(fbp, dst_data[0], FRAME_BYTES);
            }
        }
        av_packet_unref(pkt);
    }
    av_read_frame(ctx, pkt);
    if (pkt) {
        av_packet_free(&pkt);
    }
    return 0;
}
