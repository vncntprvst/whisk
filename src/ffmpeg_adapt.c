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

#include <common.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//---
#ifdef HAVE_FFMPEG
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixfmt.h>
//#include <avcodec.h>
//#include <avformat.h>
//#include <swscale.h>
//---
//
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
        printf("%s(%d):"ENDL "%s"ENDL "%s"ENDL "FFMPEG: %s"ENDL,   \
            __FILE__,__LINE__,#expr,(char*)msg,buf);                 \
      else \
        printf("%s(%d):"ENDL "%s"ENDL "FFMPEG Error: %s"ENDL,   \
            __FILE__,__LINE__,#expr,buf);                 \
      goto Error;                                         \
    }                                                   \
  }while(0)

static const AVRational ONE  = {1,1};
static const AVRational FREQ = {1,AV_TIME_BASE}; // same as AV_TIME_BASE_Q, but more MSVC friendly
#define STREAM(e)   ((e)->streams[0])
#define DURATION(e) (av_rescale_q((e)->duration,av_mul_q(FREQ,STREAM(e)->r_frame_rate),ONE)) ///< gets the duration in #frames


typedef struct _ffmpeg_video
{
   AVFormatContext *pFormatCtx;
   AVCodecContext *pCtx;
   AVCodec *pCodec;
   AVFrame *pRaw;
   AVFrame *pDat;
   uint8_t *data[AV_NUM_DATA_POINTERS];
   int linesize[AV_NUM_DATA_POINTERS];
   struct SwsContext *Sctx;
   int videoStream, width, height;
   int numBytes;
   int numFrames;
   Image currentImage;
   int last;
   int pix_fmt;
} ffmpeg_video;

void ffmpeg_video_video_debug_ppm(ffmpeg_video *cur, char *file);

//---
//  IMPLEMENTATION
//

static int is_one_time_inited = 0;

/* Init ffmpeg */
void maybeInit()
{ if(is_one_time_inited)
    return;
  avdevice_register_all();
  is_one_time_inited = 1;
}

/* Close & Free cur video context
 * This function is also called on failed init, so check existence before de-init
 */
ffmpeg_video *ffmpeg_video_quit( ffmpeg_video *cur )
{ if(!cur) return;
  if( cur->Sctx ) sws_freeContext( cur->Sctx );

  if( cur->pRaw ) av_frame_free( &cur->pRaw );
  if( cur->pDat ) av_frame_free( &cur->pDat );

  if( cur->pCtx ) avcodec_close( cur->pCtx );
  if( cur->pFormatCtx ) avformat_close_input( &cur->pFormatCtx );

  if(cur->data) av_freep(&cur->data[0]);
  free(cur);

  return NULL;
}

/* Init ffmpeg_video source
 * file: path to open
 * format: AV_PIX_FMT_GRAY8 or AV_PIX_FMT_RGB24
 * Returns ffmpeg_video context on succes, NULL otherwise
 */
ffmpeg_video *ffmpeg_video_init(const char *fname, int format )
{
  int i = 0;
  ffmpeg_video *ret;
  maybeInit();

  TRY(ret=(ffmpeg_video*)malloc(sizeof(ffmpeg_video)));
  memset(ret,0,sizeof(ffmpeg_video));
  ret->pix_fmt = format;

  AVDictionary* options = NULL;
  av_dict_set(&options, "pixel_format", "gray8", 0);
  ret->pFormatCtx = NULL;
  AVTRY(avformat_open_input(&ret->pFormatCtx, fname, NULL, &options), NULL);
  av_dict_free(&options);

  AVTRY(avformat_find_stream_info(ret->pFormatCtx, NULL), NULL);
  ret->videoStream = av_find_best_stream(ret->pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  AVStream* st = ret->pFormatCtx->streams[ret->videoStream];
  ret->pCodec = avcodec_find_decoder(st->codecpar->codec_id);
  if (!ret->pCodec)
    goto Error;

  ret->pCtx = avcodec_alloc_context3(ret->pCodec);
  AVTRY(avcodec_parameters_to_context(ret->pCtx, st->codecpar), NULL);
  AVTRY(avcodec_open2(ret->pCtx, ret->pCodec, NULL), NULL);
  ret->width = ret->pCtx->width;
  ret->height = ret->pCtx->height;
  ret->numBytes = av_image_alloc(ret->data, ret->linesize, ret->width, ret->height, ret->pix_fmt, 1);
  if (ret->numBytes < 0)
    goto Error;

  ret->numFrames = DURATION(ret->pFormatCtx); //(int)(( ret->pFormatCtx->duration / (double)AV_TIME_BASE ) * ret->pCtx->time_base.den );

  /* Init buffers */
  ret->pRaw = av_frame_alloc();
  ret->pDat = av_frame_alloc();
  ret->pDat->format = ret->pix_fmt;
  ret->pDat->width = ret->width;
  ret->pDat->height = ret->height;
  AVTRY(av_frame_get_buffer(ret->pDat, 0), NULL);

  /* Init scale & convert */
  ret->Sctx=sws_getContext(
        ret->pCtx->width,
        ret->pCtx->height,
        ret->pCtx->pix_fmt,
        ret->width,
        ret->height,
        ret->pix_fmt,
        SWS_BICUBIC,NULL,NULL,NULL);

  /* Give some info on stderr about the file & stream */
  av_dump_format(ret->pFormatCtx, 0, fname, 0);

  // setup currentImage
  ret->currentImage.kind   = 1;
  ret->currentImage.width  = ret->width;
  ret->currentImage.height = ret->height;
  ret->currentImage.text   = "\0";
  ret->currentImage.array  = ret->data[0];

  ret->last = -1;
  return ret;
Error:
  ffmpeg_video_quit(ret);
  return NULL;
}

int ffmpeg_video_bytes_per_frame( ffmpeg_video* v )
{
  return v->numBytes;
}

/* Parse next packet from cur video
 * Returns 0 on success, -1 otherwise
 */
int ffmpeg_video_next( ffmpeg_video *cur, int target )
{
  AVPacket* packet = av_packet_alloc();
  do {
    AVTRY(av_read_frame(cur->pFormatCtx, packet), NULL);
    if( packet->stream_index == cur->videoStream ) {
      AVTRY(avcodec_send_packet(cur->pCtx, packet), NULL);
      AVTRY(avcodec_receive_frame(cur->pCtx, cur->pRaw), NULL);
#if 0 // very useful for debugging
      printf("Packet - pts:%5d dts:%5d (%5d) - flag: %1d - finished: %3d - Frame pts:%5d %5d\n",
          (int)packet.pts,(int)packet.dts,target,
          packet.flags,finished,
          (int)cur->pRaw->pts,(int)cur->pRaw->best_effort_timestamp);
#endif
    }
    av_packet_unref(packet);
  } while (cur->pRaw->best_effort_timestamp < target);

  AVTRY(av_frame_make_writable(cur->pDat), NULL);

  sws_scale(cur->Sctx,              // sws context
            cur->pRaw->data,        // src slice
            cur->pRaw->linesize,    // src stride
            0,                      // src slice origin y
            cur->height,            // src slice height
            cur->pDat->data,        // dst
            cur->pDat->linesize );  // dst stride

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

#include "image_lib.h"

int _handle_open_status(const char *filename, void *c)
{
  if(c==NULL)
  { //warning("Could not open file: %s\n",filename);
    if(c) ffmpeg_video_quit(c); 
    return 0;
  }
  return 1;
}

SHARED_EXPORT void *FFMPEG_Open(const char* filename)
{ void *ctx = NULL;
  ctx = ffmpeg_video_init(filename,AV_PIX_FMT_GRAY8);
  if(!_handle_open_status(filename,ctx))
    return NULL;
  return ctx; // NULL on error
}

SHARED_EXPORT void FFMPEG_Close(void *context)
{ if(context) ffmpeg_video_quit(context);
}

SHARED_EXPORT Image *FFMPEG_Fetch(void *context, int iframe)
{ 
  ffmpeg_video *v = (ffmpeg_video*)context;
  TRY(iframe>=0 && iframe<v->numFrames);     // ensure iframe is in bounds
  if(iframe==v->last+1)
    TRY(ffmpeg_video_next(v,iframe)>=0);
  else
    TRY(ffmpeg_video_seek(v,iframe)>=0);
  v->last = iframe;
  v->currentImage.array  = v->data[0];      // just in case the pointer changed...which it didn't
  return &v->currentImage;
Error:
  return NULL;
}

SHARED_EXPORT unsigned int  FFMPEG_Frame_Count(void* ctx)
{ return ((ffmpeg_video*)ctx)->numFrames; }

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
/*int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf)
{ ffmpeg_video *ctx;
  TRY(ctx=ffmpeg_video_init(filename,AV_PIX_FMT_GRAY8));  
  { int planestride = ffmpeg_video_bytes_per_frame(ctx);
    int i;
    for(i=0;i<ctx->numFrames;++i)
    { int sts;
      TRY(ffmpeg_video_next(ctx,i)==0);
      memcpy(buf+i*planestride,ctx->buffer,planestride);
    }
  }
  if(ctx) ffmpeg_video_quit(ctx);
  return 1;
Error:
  return 0;
}*/


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

/*SHARED_EXPORT int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf)
{_handle_ffmpeg_not_installed(); return 0;}*/

#endif // if/else defined HAVE_FFMPEG
