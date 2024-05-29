#include <stdio.h>
#include "../include/ffmpeg_adapt.h"
#include "../build/src/parameters/param.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <video_file>\n", argv[0]);
        return -1;
    }

    char *filename = argv[1];
    printf("Entering ffmpeg_video_init\n");
    ffmpeg_video *ctx = ffmpeg_video_init(filename, AV_PIX_FMT_YUV420P);

    if (ctx == NULL) {
        printf("ffmpeg_video_init failed for file: %s\n", filename);
        return -1;
    }

    printf("Image allocated with buffer size %d\n", ctx->numBytes);
    printf("ffmpeg_video_init succeeded for file: %s\n", filename);

    // Check pointers before use
    if (ctx->pFormatCtx) {
        printf("Format context initialized\n");
    }
    if (ctx->pCodec) {
        printf("Codec initialized\n");
    }
    if (ctx->pCtx) {
        printf("Codec context initialized\n");
    }

    ffmpeg_video_quit(ctx);
    return 0;
}

// Output: 
// ./ffmpeg_video_init_test /mnt/md0/analysis/whisker_asym/Analysis/test/sc016_0630_001_30sWhisking.mp4
// Entering ffmpeg_video_init
// Entering ffmpeg_video_init
// Image allocated with buffer size 604800
// Image allocated with buffer size 604800
// ffmpeg_video_init succeeded for file: /mnt/md0/analysis/whisker_asym/Analysis/test/sc016_0630_001_30sWhisking.mp4
// Format context initialized
// Codec initialized
// Codec context initialized
// FFmpeg video quit

// Note: 
// The "double all" behavior in the ffmpeg_video_init output suggests that the function is being called twice. 
// This could be due to the test being run twice or some other unintended duplicate call.