#include <stdio.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>

int main() {
    int width = 720, height = 540;
    // enum AVPixelFormat pix_fmt = AV_PIX_FMT_RGB24;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
    int align = 1;
    uint8_t *buffers[4];
    int linesizes[4];

    printf("Testing av_image_alloc with width=%d, height=%d, pix_fmt=%d\n", width, height, pix_fmt);

    int buffer_size = av_image_alloc(buffers, linesizes, width, height, pix_fmt, align);
    if (buffer_size < 0) {
        fprintf(stderr, "Could not allocate image\n");
        return 1;
    }

    printf("Buffer[0] allocated at %p with linesize %d\n", buffers[0], linesizes[0]);

    av_freep(&buffers[0]);

    return 0;
}
