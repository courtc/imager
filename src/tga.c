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

#include <string.h>
#include <stdlib.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef   signed short  int16;
#pragma pack(push,1)
typedef struct {
	uint8  identsize;       /* size of ID field that follows 18 byte header (0 usually) */
	uint8  colourmaptype;   /* type of colour map 0=none, 1=has palette */
	uint8  imagetype;       /* type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed */

	uint16 colourmapstart;  /* first colour map entry in palette */
	uint16 colourmaplength; /* number of colours in palette */
	uint8  colourmapbits;   /* number of bits per palette entry 15,16,24,32 */

	int16  xstart;          /* image x origin */
	int16  ystart;          /* image y origin */
	uint16 width;           /* image width in pixels */
	uint16 height;          /* image height in pixels */
	uint8  bits;            /* image bits per pixel 8,16,24,32 */
	uint8  descriptor;      /* image descriptor bits (vh flip bits) */
} TGAHDR_t;
#pragma pack(pop)

int LoadTGA(void *pRaw, int rawlen, unsigned int *puWidth, unsigned int *puHeight, void **ppData)
{
	TGAHDR_t hdr;

	memcpy(&hdr,pRaw,sizeof(TGAHDR_t));
	if( hdr.colourmaptype != 0 ) {
		return -10;
	}
	if( hdr.imagetype != 2 ) {
		return -11;
	}
	if( hdr.bits != 32 ) {
		return -12;
	}

	*puWidth  = hdr.width;
	*puHeight = hdr.height;
	*ppData   = malloc(hdr.width*hdr.height*4);

	memcpy(*ppData,(uint8*)pRaw + sizeof(TGAHDR_t) + hdr.identsize,rawlen - (sizeof(TGAHDR_t) + hdr.identsize));

	return 0;
}
