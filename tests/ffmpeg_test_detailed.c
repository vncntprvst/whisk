#include <stdio.h>
#include "../include/ffmpeg_adapt.h"
#include "../build/src/parameters/param.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <video_file>\n", argv[0]);
        return -1;
    }

    const char *filename = argv[1];

    // Test FFMPEG_Open
    ffmpeg_video *ctx = FFMPEG_Open(filename);
    if (!ctx)
    {
        fprintf(stderr, "FFMPEG_Open failed for file: %s\n", filename);
        return -1;
    }
    printf("FFMPEG_Open succeeded for file: %s\n", filename);

    // Test FFMPEG_Frame_Count
    unsigned int frameCount = FFMPEG_Frame_Count(ctx);
    printf("Frame count: %u\n", frameCount);

    // Test FFMPEG_Fetch
    for (int i = 0; i < frameCount; i++)
    {
        Image *img = FFMPEG_Fetch(ctx, i);
        if (!img)
        {
            fprintf(stderr, "FFMPEG_Fetch failed for frame: %d\n", i);
            FFMPEG_Close(ctx);
            return -1;
        }
        printf("FFMPEG_Fetch succeeded for frame: %d\n", i);
    }

    // Test FFMPEG_Close
    FFMPEG_Close(ctx);
    printf("FFMPEG_Close succeeded\n");

    return 0;
}
