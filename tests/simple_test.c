#include <stdio.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libavutil/mem.h>

int main() {
    int width = 720;
    int height = 540;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_RGB24;
    int align = 4;

    // Initialize FFmpeg libraries (deprecated in newer versions but useful for older versions)
    // av_register_all();
    // avcodec_register_all();

    // Allocate the image buffer
    uint8_t *buffers[4] = {0};
    int linesizes[4] = {0};

    printf("Calling av_image_alloc with width=%d, height=%d, pix_fmt=%d\n", width, height, pix_fmt);
    int num_bytes = av_image_alloc(buffers, linesizes, width, height, pix_fmt, align);
    if (num_bytes < 0) {
        fprintf(stderr, "Error allocating image: %d\n", num_bytes);
        return 1;
    }

    // Print the allocated buffer information
    for (int i = 0; i < 4; i++) {
        if (buffers[i]) {
            printf("Buffer[%d] allocated at %p with linesize %d\n", i, buffers[i], linesizes[i]);
        }
    }

    // Free the allocated buffer
    av_freep(&buffers[0]);

    return 0;
}
