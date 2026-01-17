#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

int main(void)
{
    int rc;
    AVCodec *encoder;
    printf("Hello, world\n");
    encoder = avcodec_find_encoder_by_name("h264_omx");
    if (NULL == encoder)
    {
        fprintf(stderr, "encoder not found %s", av_err2str(errno));
    }
    else
    {

}
// gcc -Wall -g test_omx.c `pkg-config libavcodec` -o test_omx
    
