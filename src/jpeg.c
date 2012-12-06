/*
 * Copyright (c) 2012, James Jacobsson All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * - Neither the name of the organization nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <png.h>
#include <jpeglib.h>
#include <jerror.h>

typedef struct {
  int width,height;
  void *pData;
} decjpeg_t;

typedef struct {
    struct jpeg_source_mgr pub;
    JOCTET eoi_buffer[2];
} ljpg_src_mgr_t;

static void ljpg_init_source(j_decompress_ptr cinfo) {
}
 
static boolean ljpg_fill_input_buffer(j_decompress_ptr cinfo) {
    return 1;
}
static void ljpg_term_source(j_decompress_ptr cinfo) {
}

static void ljpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    ljpg_src_mgr_t *src = (void *)cinfo->src;

    if (num_bytes > 0) {
      while (num_bytes > (long)src->pub.bytes_in_buffer) {
	num_bytes -= (long)src->pub.bytes_in_buffer;
	ljpg_fill_input_buffer(cinfo);
      }
    }
    src->pub.next_input_byte += num_bytes;
    src->pub.bytes_in_buffer -= num_bytes;
}

void ljpg_memory_src(j_decompress_ptr cinfo, unsigned char const *buffer, size_t bufsize) {
    ljpg_src_mgr_t *src;

    if(!cinfo->src) {
      cinfo->src = (*cinfo->mem->alloc_small)((void *)cinfo, JPOOL_PERMANENT, sizeof(ljpg_src_mgr_t));
    }
    src = (void *)cinfo->src;
    src->pub.init_source       = ljpg_init_source;
    src->pub.fill_input_buffer = ljpg_fill_input_buffer;
    src->pub.skip_input_data   = ljpg_skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    src->pub.term_source       = ljpg_term_source;
    src->pub.next_input_byte   = buffer;
    src->pub.bytes_in_buffer   = bufsize;
}

static void ljpg_output_message(j_common_ptr cinfo)
{
}

static void ljpg_error_exit(j_common_ptr cinfo)
{
  cinfo->client_data = NULL;
  //jpeg_destroy(cinfo);
}

static decjpeg_t *jpeg_decode(void *indata,unsigned int indatasize) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;
  int     w,h;
  unsigned int *data;
  unsigned char *buffer[1];
  decjpeg_t    *ret = NULL;

  jpeg_create_decompress(&cinfo);
  cinfo.err           = jpeg_std_error(&jerr);
  cinfo.client_data   = indata;
  jerr.error_exit     = ljpg_error_exit;
  jerr.output_message = ljpg_output_message;

  ljpg_memory_src(&cinfo,indata,indatasize);

  jpeg_read_header(&cinfo, TRUE);

  /* Ask for a pre-defined color format */
  cinfo.out_color_space = JCS_RGB;

  jpeg_start_decompress(&cinfo);
  if (cinfo.client_data == NULL) {
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }

  w = cinfo.output_width;
  h = cinfo.output_height;
  data = (unsigned int*)malloc(w*h*4);

  buffer[0] = (unsigned char*)malloc(w * 4);
  while( (int)cinfo.output_scanline < h ) {
    int y = cinfo.output_scanline;
    int x;
    jpeg_read_scanlines(&cinfo, (void *)buffer, 1);
 
    for (x = 0; x < w; x++) {
      unsigned char r = buffer[0][x * 3 + 0];
      unsigned char g = buffer[0][x * 3 + 1];
      unsigned char b = buffer[0][x * 3 + 2];

      data[y*w+x] = (0xFF<<24)|(b<<16)|(g<<8)|(r<<0);
    }
  }
  free(buffer[0]);
  jpeg_destroy_decompress(&cinfo);

  /* Allocate our surface. */
  ret = (decjpeg_t*)calloc(1,sizeof(decjpeg_t));
  ret->width  = w;
  ret->height = h;
  ret->pData  = data;

  return ret;
}

int LoadJPEG(void *pRaw, int rawlen, unsigned int *puWidth, unsigned int *puHeight, void **ppData) {
  decjpeg_t *pJPEG;

  pJPEG = jpeg_decode(pRaw,rawlen);

  if( pJPEG == NULL )
    return -1;

  *puWidth  = pJPEG->width;
  *puHeight = pJPEG->height;
  *ppData   = pJPEG->pData;

  free(pJPEG);

  return 0;
}
