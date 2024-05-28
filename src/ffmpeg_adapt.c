/*
 * Code here is largely based on Michael Meeuwisse's example code (copyright
 * and license below).  The only changes I've made have been to rename some
 * things.
 *
 * Author: Nathan Clack and Mark Bolstadt
 *   Date: May 31, 2010
 */
/* 
 * (C) Copyright 2010 Michael Meeuwisse
 *
 * Adapted from avcodec_sample.0.5.0.c, license unknown
 *
 * ffmpeg_test is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ffmpeg_test is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ffmpeg_test. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ffmpeg_adapt.h"
#include "image_lib.h"
#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_FFMPEG
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixfmt.h>
#include <libavutil/mem.h>

#define ENDL "\n"
#define HERE         printf("%s(%d): HERE"ENDL,__FILE__,__LINE__);
#define REPORT(expr) printf("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr)
#define TRY(expr)    do{if(!(expr)) {REPORT(expr); goto Error;}}while(0)
#define DIE          do{printf("%s(%d): Fatal error.  Aborting."ENDL,__FILE__,__LINE__); exit(-1);}while(0)
#define AVTRY(expr,msg) \
  do{                                                   \
    int v=(expr);                                       \
    if(v<0 && v!=AVERROR_EOF)                       \
    { char buf[1024];                                   \
      av_strerror(v,buf,sizeof(buf));                 \
      if(msg) \
        fprintf(stderr, "%s(%d):"ENDL "%s"ENDL "%s"ENDL "FFMPEG: %s"ENDL,   \
            __FILE__,__LINE__,#expr,(char*)msg,buf);                 \
      else \
        fprintf(stderr, "%s(%d):"ENDL "%s"ENDL "FFMPEG Error: %s"ENDL,   \
            __FILE__,__LINE__,#expr,buf);                 \
      goto Error;                                         \
    }                                                   \
  }while(0)

static const AVRational ONE  = {1,1};
static const AVRational FREQ = {1,AV_TIME_BASE}; // same as AV_TIME_BASE_Q, but more MSVC friendly
#define STREAM(e)   ((e)->streams[0])
#define DURATION(e) (av_rescale_q((e)->duration,av_mul_q(FREQ,STREAM(e)->r_frame_rate),ONE)) ///< gets the duration in #frames

// Definition of the ffmpeg_video structure is now in ffmpeg_adapt.h

void ffmpeg_video_video_debug_ppm(ffmpeg_video *cur, char *file);

static int is_one_time_inited = 0;

// Initialize FFmpeg libraries if they haven't been initialized
void maybeInit() {
    static int is_one_time_inited = 0;
    if (is_one_time_inited)
        return;
    avformat_network_init(); // Use avformat_network_init instead of deprecated av_register_all and avcodec_register_all
    is_one_time_inited = 1;
}

int is_pix_fmt_supported(const AVCodec *codec, enum AVPixelFormat pix_fmt) {
    const enum AVPixelFormat *p = codec->pix_fmts;
    while (*p != AV_PIX_FMT_NONE) {
        if (*p == pix_fmt)
            return 1;
        p++;
    }
    return 0;
}

void ffmpeg_video_quit(ffmpeg_video *v) {
    if (v->pDat) {
        av_freep(&v->pDat);
        v->pDat = NULL;
    }
    if (v->pRaw) {
        av_frame_free(&v->pRaw);
        v->pRaw = NULL;
    }
    if (v->pCtx) {
        avcodec_close(v->pCtx);
        v->pCtx = NULL;
    }
    if (v->pFormatCtx) {
        avformat_close_input(&v->pFormatCtx);
        v->pFormatCtx = NULL;
    }
    if (v->data[0]) {
        av_freep(&v->data[0]);
    }
    if (v) {
        free(v);
        v = NULL;
    }
    fprintf(stdout, "FFmpeg video quit\n");
}

/* Init ffmpeg_video source
 * file: path to open
 * format: AV_PIX_FMT_GRAY8, AV_PIX_FMT_RGB24, or AV_PIX_FMT_YUV420P
 * Returns ffmpeg_video context on success, NULL otherwise
 */
ffmpeg_video* ffmpeg_video_init(const char *fname, int format) {
    int ret;
    ffmpeg_video *v = NULL;
    v = (ffmpeg_video*)malloc(sizeof(ffmpeg_video));
    if (!v) {
        fprintf(stderr, "Could not allocate ffmpeg_video struct\n");
        return NULL;
    }
    memset(v, 0, sizeof(ffmpeg_video));

    maybeInit();
    fprintf(stdout, "Entering ffmpeg_video_init\n");

    // Open video file
    if (avformat_open_input(&v->pFormatCtx, fname, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file: %s\n", fname);
        goto Error;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(v->pFormatCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto Error;
    }

    // Find the first video stream
    v->videoStream = -1;
    for (int i = 0; i < v->pFormatCtx->nb_streams; i++) {
        if (v->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            v->videoStream = i;
            break;
        }
    }

    if (v->videoStream == -1) {
        fprintf(stderr, "Did not find a video stream\n");
        goto Error;
    }

    // Get a pointer to the codec context for the video stream
    v->pCodec = avcodec_find_decoder(v->pFormatCtx->streams[v->videoStream]->codecpar->codec_id);
    if (v->pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        goto Error;
    }

    v->pCtx = avcodec_alloc_context3(v->pCodec);
    if (avcodec_parameters_to_context(v->pCtx, v->pFormatCtx->streams[v->videoStream]->codecpar) < 0) {
        fprintf(stderr, "Couldn't copy codec context\n");
        goto Error;
    }

    // Open codec
    if (avcodec_open2(v->pCtx, v->pCodec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        goto Error;
    }

    // Allocate video frame
    v->pRaw = av_frame_alloc();
    if (!v->pRaw) {
        fprintf(stderr, "Could not allocate raw frame\n");
        goto Error;
    }

    // Allocate an AVFrame structure
    v->pDat = av_frame_alloc();
    if (!v->pDat) {
        fprintf(stderr, "Could not allocate frame\n");
        goto Error;
    }

    // Determine required buffer size and allocate buffer
    v->width = v->pCtx->width;
    v->height = v->pCtx->height;
    v->pix_fmt = AV_PIX_FMT_RGB24; // Ensure the pixel format is supported

    v->numBytes = av_image_alloc(v->data, v->linesize, v->width, v->height, v->pix_fmt, 32);
    if (v->numBytes < 0) {
        fprintf(stderr, "Error allocating image: %d\n", v->numBytes);
        goto Error;
    }

    fprintf(stdout, "Image allocated with buffer size %d\n", v->numBytes);
    return v;

Error:
    ffmpeg_video_quit(v);
    return NULL;
}


int ffmpeg_video_bytes_per_frame( ffmpeg_video* v )
{
  return v->numBytes;
}

/* Parse next packet from cur video
 * Returns 0 on success, -1 otherwise
 */
int ffmpeg_video_next(ffmpeg_video *cur, int target)
{
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        fprintf(stderr, "Error allocating AVPacket\n");
        return -1;
    }

    do {
        int ret = av_read_frame(cur->pFormatCtx, packet);
        if (ret < 0) {
            av_packet_free(&packet);
            if (ret == AVERROR_EOF)
                return 0; // End of file
            return -1; // Other errors
        }

        if (packet->stream_index == cur->videoStream) {
            ret = avcodec_send_packet(cur->pCtx, packet);
            if (ret < 0) {
                av_packet_free(&packet);
                fprintf(stderr, "Error sending packet: %s\n", av_err2str(ret));
                return -1;
            }

            ret = avcodec_receive_frame(cur->pCtx, cur->pRaw);
            if (ret == AVERROR(EAGAIN)) {
                // The decoder needs more data to produce a frame.
                // No action required here. The next packet will be processed in the next loop iteration.
            } else if (ret < 0) {
                av_packet_free(&packet);
                fprintf(stderr, "Error receiving frame: %s\n", av_err2str(ret));
                return -1;
            }
        }

        av_packet_unref(packet);
    } while (cur->pRaw->best_effort_timestamp < target);

    av_packet_free(&packet);

    int ret = av_frame_make_writable(cur->pDat);
    if (ret < 0) {
        fprintf(stderr, "Error making frame writable: %s\n", av_err2str(ret));
        return -1;
    }

    sws_scale(cur->Sctx,                       // sws context
              (const uint8_t * const *)cur->pRaw->data, // src slice
              cur->pRaw->linesize,             // src stride
              0,                               // src slice origin y
              cur->height,                     // src slice height
              cur->pDat->data,                 // dst
              cur->pDat->linesize);            // dst stride

    /* copy out raw data */
    av_image_copy(cur->data, cur->linesize, (const uint8_t **)(cur->pDat->data), cur->pDat->linesize, cur->pix_fmt, cur->width, cur->height);

    return 0;
Error:
    return -1;
}

// \returns current frame on success, otherwise -1
int ffmpeg_video_seek( ffmpeg_video *cur, int64_t iframe )
{ int64_t duration = cur->pFormatCtx->streams[cur->videoStream]->duration;
  int64_t ts = av_rescale(duration,iframe,cur->numFrames),
         tol = av_rescale(duration,1,2*cur->numFrames);
  TRY(iframe>=0 && iframe<cur->numFrames);

#if 0
  AVTRY(av_seek_frame(      cur->pFormatCtx, //format context
                            cur->videoStream,//stream id
                            ts,              //target timestamp
                            0),//AVSEEK_FLAG_ANY),//flags
    "Failed to seek.");
#else
  AVTRY(avformat_seek_file( cur->pFormatCtx, //format context
                            cur->videoStream,//stream id
                            0,               //min timestamp
                            ts,              //target timestamp
                            ts,              //max timestamp
                            0),//AVSEEK_FLAG_ANY),//flags
    "Failed to seek.");
#endif
  avcodec_flush_buffers(cur->pCtx);

  TRY(ffmpeg_video_next(cur,iframe)==0);
  return iframe;
Error:
  return -1;
}


/* Output frame to file in PPM format */
void ffmpeg_video_debug_ppm( ffmpeg_video *cur, char *file ) 
{
  int i = 0;
  FILE *out = fopen( file, "wb" );

  if( !out )
    return;

  /* PPM header */
  fprintf( out, "P%d\n%d %d\n255\n", cur->pix_fmt == AV_PIX_FMT_GRAY8? 5: 6, 
      cur->width, cur->height );

  /* Spit out raw data */
  for( i = 0; i < cur->height; i++ )
    fwrite( cur->pDat->data[0] + i * cur->pDat->linesize[0], 1,
        cur->width * ( cur->pix_fmt == AV_PIX_FMT_GRAY8? 1: 3 ), out );

  fclose( out );
}

//--- Wrappers
int _handle_open_status(const char *filename, void *c)
{
    if (c == NULL)
    {
        fprintf(stderr, "Could not open file: %s\n", filename);
        if (c) ffmpeg_video_quit(c);
        return 0;
    }
    return 1;
}

SHARED_EXPORT void *FFMPEG_Open(const char *filename)
{
    void *ctx = NULL;
    ctx = ffmpeg_video_init(filename, AV_PIX_FMT_GRAY8);
    if (!_handle_open_status(filename, ctx))
        return NULL;
    return ctx; // NULL on error
}

SHARED_EXPORT void FFMPEG_Close(void *context)
{
    if (context) ffmpeg_video_quit(context);
}

SHARED_EXPORT Image *FFMPEG_Fetch(void *context, int iframe)
{
    ffmpeg_video *v = (ffmpeg_video *)context;
    if (iframe < 0 || iframe >= v->numFrames)
    {
        fprintf(stderr, "Iframe out of bounds: %d\n", iframe);
        return NULL; // Ensure iframe is in bounds
    }

    if (iframe == v->last + 1)
    {
        if (ffmpeg_video_next(v, iframe) < 0)
        {
            fprintf(stderr, "Failed to get next frame: %d\n", iframe);
            return NULL;
        }
    }
    else
    {
        if (ffmpeg_video_seek(v, iframe) < 0)
        {
            fprintf(stderr, "Failed to seek to frame: %d\n", iframe);
            return NULL;
        }
    }

    v->last = iframe;
    v->currentImage.array = v->data[0]; // Just in case the pointer changed...which it didn't
    return &v->currentImage;
}

SHARED_EXPORT unsigned int FFMPEG_Frame_Count(void *ctx)
{
    ffmpeg_video *v = (ffmpeg_video *)ctx;
    int frameCount = 0;
    int frameFinished;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();

    while (av_read_frame(v->pFormatCtx, &packet) >= 0)
    {
        if (packet.stream_index == v->videoStream)
        {
            avcodec_decode_video2(v->pCtx, frame, &frameFinished, &packet);
            if (frameFinished)
            {
                frameCount++;
            }
        }
        av_packet_unref(&packet);
    }

    av_frame_free(&frame);
    av_seek_frame(v->pFormatCtx, v->videoStream, 0, AVSEEK_FLAG_BACKWARD); // Seek back to the start
    return frameCount;
}

//--- UI2.PY interface

int FFMPEG_Get_Stack_Dimensions(char *filename, int *width, int *height, int *depth, int *kind)
{ ffmpeg_video *ctx = ffmpeg_video_init(filename,AV_PIX_FMT_GRAY8); 
  if(!_handle_open_status(filename,ctx))
    return 0;
  *width = ctx->width;
  *height = ctx->height;
  *depth = ctx->numFrames;
  *kind = 1;
  if(ctx) ffmpeg_video_quit(ctx);
  return 1;
}

// This involves an unneccesary copy.
int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf)
{ ffmpeg_video *ctx;
  TRY(ctx=ffmpeg_video_init(filename,AV_PIX_FMT_GRAY8));  
  { int planestride = ffmpeg_video_bytes_per_frame(ctx);
    int i;
    for(i=0;i<ctx->numFrames;++i)
    { int sts;
      TRY(ffmpeg_video_next(ctx,i)==0);
      memcpy(buf+i*planestride,ctx->data[0],planestride);
    }
  }
  if(ctx) ffmpeg_video_quit(ctx);
  return 1;
Error:
  return 0;
}

#else // HAVE_FFMPEG not defined

void _handle_ffmpeg_not_installed(void)
{ error("FFMPEG was not built into this package.\n");
}

SHARED_EXPORT void         *FFMPEG_Open       (const char* filename)     {_handle_ffmpeg_not_installed(); return 0;}
SHARED_EXPORT void          FFMPEG_Close      (void *context)            {_handle_ffmpeg_not_installed();}             
SHARED_EXPORT Image        *FFMPEG_Fetch      (void *context, int iframe){_handle_ffmpeg_not_installed(); return 0;}   
SHARED_EXPORT unsigned int  FFMPEG_Frame_Count(void *context)            {_handle_ffmpeg_not_installed(); return 0;}   

//--- UI2.PY interface
SHARED_EXPORT int FFMPEG_Get_Stack_Dimensions(char *filename, int *width, int *height, int *depth, int *kind)
{_handle_ffmpeg_not_installed(); return 0;}

SHARED_EXPORT int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf)
{_handle_ffmpeg_not_installed(); return 0;}

#endif // if/else defined HAVE_FFMPEG
