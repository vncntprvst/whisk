#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>

int is_pix_fmt_supported(const AVCodec *codec, enum AVPixelFormat pix_fmt) {
    const enum AVPixelFormat *p = codec->pix_fmts;
    if (p) {
        while (*p != AV_PIX_FMT_NONE) {
            if (*p == pix_fmt)
                return 1;
            p++;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <video_file>\n", argv[0]);
        return -1;
    }

    char *filename = argv[1];

    av_log_set_level(AV_LOG_DEBUG); // Set log level to debug

    AVFormatContext *pFormatCtx = NULL;
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&pFormatCtx);
        return -1;
    }

    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        fprintf(stderr, "Did not find a video stream\n");
        avformat_close_input(&pFormatCtx);
        return -1;
    }

    AVCodecParameters *pCodecParameters = pFormatCtx->streams[videoStream]->codecpar;
    const AVCodec *pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        avformat_close_input(&pFormatCtx);
        return -1;
    }

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        fprintf(stderr, "Could not allocate codec context\n");
        avformat_close_input(&pFormatCtx);
        return -1;
    }

    if (avcodec_parameters_to_context(pCodecCtx, pCodecParameters) < 0) {
        fprintf(stderr, "Couldn't copy codec context\n");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return -1;
    }

    printf("Checking pixel format support...\n");

    // List all supported pixel formats
    printf("Supported pixel formats:\n");
    const enum AVPixelFormat *p = pCodec->pix_fmts;
    if (p) {
        while (*p != AV_PIX_FMT_NONE) {
            printf("  %s\n", av_get_pix_fmt_name(*p));
            p++;
        }
    } else {
        printf("  None\n");
    }

    // Check if the pixel format AV_PIX_FMT_GRAY8 is supported
    if (is_pix_fmt_supported(pCodec, AV_PIX_FMT_GRAY8)) {
        printf("Pixel format AV_PIX_FMT_GRAY8 is supported\n");
    } else {
        printf("Pixel format AV_PIX_FMT_GRAY8 is NOT supported\n");
    }

    // Check if the pixel format AV_PIX_FMT_RGB24 is supported
    if (is_pix_fmt_supported(pCodec, AV_PIX_FMT_RGB24)) {
        printf("Pixel format AV_PIX_FMT_RGB24 is supported\n");
    } else {
        printf("Pixel format AV_PIX_FMT_RGB24 is NOT supported\n");
    }

    // Check if the pixel format AV_PIX_FMT_YUV420P is supported
    if (is_pix_fmt_supported(pCodec, AV_PIX_FMT_YUV420P)) {
        printf("Pixel format AV_PIX_FMT_YUV420P is supported\n");
    } else {
        printf("Pixel format AV_PIX_FMT_YUV420P is NOT supported\n");
    }

    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}

// Output: 
// ./pixel_format_test /mnt/md0/analysis/whisker_asym/Analysis/test/sc016_0630_001_30sWhisking.mp4
// [NULL @ 0x610aa1494d80] Opening '/mnt/md0/analysis/whisker_asym/Analysis/test/sc016_0630_001_30sWhisking.mp4' for reading
// [file @ 0x610aa14953c0] Setting default whitelist 'file,crypto,data'
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] Format mov,mp4,m4a,3gp,3g2,mj2 probed with size=2048 and score=100
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] ISO: File Type Major Brand: isom
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] Unknown dref type 0x206c7275 size 12
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] Processing st: 0, edit list 0 - media time: -1, duration: 8000
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] Processing st: 0, edit list 1 - media time: 96, duration: 472160
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] Offset DTS by 96 to make first pts zero.
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] Setting codecpar->delay to 1 for stream st: 0
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] Before avformat_find_stream_info() pos: 7973468 bytes read:151994 seeks:1 nb_streams:1
// [h264 @ 0x610aa1496240] nal_unit_type: 7(SPS), nal_ref_idc: 3
// [h264 @ 0x610aa1496240] Decoding VUI
// [h264 @ 0x610aa1496240] nal_unit_type: 8(PPS), nal_ref_idc: 3
// [h264 @ 0x610aa1496240] nal_unit_type: 7(SPS), nal_ref_idc: 3
// [h264 @ 0x610aa1496240] Decoding VUI
// [h264 @ 0x610aa1496240] nal_unit_type: 8(PPS), nal_ref_idc: 3
// [h264 @ 0x610aa1496240] ct_type:0 pic_struct:0
// [h264 @ 0x610aa1496240] nal_unit_type: 6(SEI), nal_ref_idc: 0
// [h264 @ 0x610aa1496240] nal_unit_type: 6(SEI), nal_ref_idc: 0
// [h264 @ 0x610aa1496240] nal_unit_type: 5(IDR), nal_ref_idc: 3
// [h264 @ 0x610aa1496240] Format yuv420p chosen by get_format().
// [h264 @ 0x610aa1496240] Reinit context to 720x544, pix_fmt: yuv420p
// [h264 @ 0x610aa1496240] ct_type:0 pic_struct:0
// [h264 @ 0x610aa1496240] no picture 
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] All info found
// [mov,mp4,m4a,3gp,3g2,mj2 @ 0x610aa1494d80] After avformat_find_stream_info() pos: 13050 bytes read:184762 seeks:2 frames:1
// [h264 @ 0x610aa1499700] nal_unit_type: 7(SPS), nal_ref_idc: 3
// [h264 @ 0x610aa1499700] Decoding VUI
// [h264 @ 0x610aa1499700] nal_unit_type: 8(PPS), nal_ref_idc: 3
// Checking pixel format support...
// Supported pixel formats:
//   None
// Pixel format AV_PIX_FMT_GRAY8 is NOT supported
// Pixel format AV_PIX_FMT_RGB24 is NOT supported
// Pixel format AV_PIX_FMT_YUV420P is NOT supported
// [AVIOContext @ 0x610aa149d900] Statistics: 184762 bytes read, 2 seeks

// Note: The output indicates that the pixel format support check in pixel_format_test is not functioning as expected. 
// Despite the fact that the video stream uses the yuv420p pixel format, the test reports that no pixel formats are supported by the codec. 
// This inconsistency suggests an issue in how the supported pixel formats are being retrieved or reported.