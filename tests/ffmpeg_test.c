#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

void maybeInit() {
    av_log_set_level(AV_LOG_DEBUG);
    avdevice_register_all();
    avformat_network_init();
}

int ffmpeg_video_init(const char *filename) {
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (!pFormatCtx) {
        fprintf(stderr, "Could not allocate context.\n");
        return -1;
    }

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file.\n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information.\n");
        return -1;
    }

    av_dump_format(pFormatCtx, 0, filename, 0);

    AVCodec *pCodec = NULL;
    int videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, (const AVCodec**)&pCodec, 0);
    if (videoStream < 0) {
        fprintf(stderr, "Could not find video stream.\n");
        return -1;
    }

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        fprintf(stderr, "Could not allocate codec context.\n");
        return -1;
    }

    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        fprintf(stderr, "Could not open codec.\n");
        return -1;
    }

    // Clean up
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <video_file>\n", argv[0]);
        return -1;
    }

    maybeInit();

    if (ffmpeg_video_init(argv[1]) < 0) {
        fprintf(stderr, "Error initializing video.\n");
        return -1;
    }

    printf("Video initialized successfully.\n");
    return 0;
}
