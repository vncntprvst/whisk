#include <stdio.h>
#include <stdlib.h>
#include "../include/ffmpeg_adapt.h"
#include "../build/src/parameters/param.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <video_file>\n", argv[0]);
        return -1;
    }

    const char *filename = argv[1];
    ffmpeg_video *ctx = ffmpeg_video_init(filename, AV_PIX_FMT_RGB24);
    if (!ctx) {
        fprintf(stderr, "ffmpeg_video_init failed for file: %s\n", filename);
        return -1;
    }

    printf("ffmpeg_video_init succeeded for file: %s\n", filename);

    // Test fetching a frame
    if (ffmpeg_video_next(ctx, 0) < 0) {
        fprintf(stderr, "ffmpeg_video_next failed to fetch frame 0\n");
        ffmpeg_video_quit(ctx);
        return -1;
    }
    printf("ffmpeg_video_next succeeded for frame 0\n");

    // Check frame writable state
    if (av_frame_make_writable(ctx->pDat) < 0) {
        fprintf(stderr, "Error making frame writable\n");
        ffmpeg_video_quit(ctx);
        return -1;
    }
    printf("Frame is writable\n");

    ffmpeg_video_quit(ctx);
    return 0;
}
