/*
 * Copyright (c) 2005, James Jacobsson All rights reserved.
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

#include <string.h>
#include <stdlib.h>
#include <png.h>

typedef struct {
	void  *pPtr;
	unsigned int off;
} mypngio_t;

static void user_read_fn(png_structp png_ptr, png_bytep dest,png_size_t bytestoread) {
	mypngio_t *my = (mypngio_t*)png_get_io_ptr(png_ptr);

	memcpy( dest, (unsigned char*)my->pPtr + my->off, bytestoread );

	my->off += bytestoread;
}

#if PNG_LIBPNG_VER >= 10209
#define png_set_gray_1_2_4_to_8 png_set_expand_gray_1_2_4_to_8
#endif

int LoadPNG(void *pRaw, int rawlen, unsigned int *puWidth, unsigned int *puHeight, void **ppData)
{
	png_bytep *row_pointers;
	png_uint_32 w, h;
	png_structp png_ptr;
	png_infop info_ptr;
	int bit_depth, color_type, interlace_type;
	int i;
	int number_of_passes;
	void *pixels;
	mypngio_t *pMy;

	png_ptr = NULL; info_ptr = NULL; row_pointers = NULL;

	if( png_sig_cmp((png_bytep)pRaw, 0, 8) != 0 ) {
		goto err_exit;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) {
		goto err_exit;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		goto err_exit;
	}

	if (setjmp (png_jmpbuf (png_ptr))) {
		goto err_exit;
	}

	pMy = (mypngio_t*)calloc(1,sizeof(mypngio_t));
	pMy->pPtr = pRaw;
	pMy->off  = 8;
	png_set_read_fn(png_ptr, (void *)pMy, user_read_fn);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info (png_ptr, info_ptr);
	png_get_IHDR (png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
		&interlace_type, 0, 0);

	/*** Set up some transformations to get everything in our nice ARGB format. ***/
	/* 8 bits per channel: */
	if (bit_depth == 16)
		png_set_strip_16 (png_ptr);
	/* Convert palette to RGB: */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb (png_ptr), color_type = PNG_COLOR_TYPE_RGB;
	/* Extract 1/2/4bpp to 8bpp: */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8 (png_ptr);
	/* Convert colorkey to alpha: */
	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha (png_ptr);
	/* Convert gray to RGB: */
	if (color_type == PNG_COLOR_TYPE_GRAY)
		png_set_gray_to_rgb (png_ptr),
		color_type = PNG_COLOR_TYPE_RGB;

	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb (png_ptr),
		color_type = PNG_COLOR_TYPE_RGB_ALPHA;

	/* Convert RGB to RGBA */
	if (color_type == PNG_COLOR_TYPE_RGB)
		png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);

	//png_set_bgr(png_ptr);
	number_of_passes = png_set_interlace_handling (png_ptr);

	/* Update the info struct. */
	png_read_update_info (png_ptr, info_ptr);

	/* Allocate our surface. */
	pixels = malloc( w * h * 4 );
	if (!pixels) {
		goto err_exit;
	}

	row_pointers = (png_bytep*)malloc (h * sizeof(png_bytep));
	if (!row_pointers) {
		goto err_exit;
	}

	/* Build the array of row pointers. */
	for (i = 0; i < (int)h; i++) {
		row_pointers[i] = (png_bytep)( (unsigned char*)pixels + (i * w * 4) );
	}

	/* Read the thing. */
	png_read_image (png_ptr, row_pointers);

	/* Read the rest. */
	png_read_end (png_ptr, info_ptr);

	if (png_ptr)
		png_destroy_read_struct (&png_ptr, info_ptr? &info_ptr : 0, 0);
	if (row_pointers)
		free (row_pointers);

	*puWidth  = w;
	*puHeight = h;
	*ppData   = pixels;

	return 0;

err_exit:
	if (png_ptr)
		png_destroy_read_struct (&png_ptr, info_ptr? &info_ptr : 0, 0);
	if (row_pointers)
		free (row_pointers);

	return -1;
}
