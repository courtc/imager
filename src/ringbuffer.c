/*
 * Copyright (c) 2011, Courtney Cavin
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include "ringbuffer.h"

#define MIN(x,y) (((x)<(y))?(x):(y))

int ringbuffer_create(ringbuffer_t *buf, unsigned int size)
{
	buf->data = malloc(size);
	if (buf->data == NULL)
		return -1;
	buf->size = size;
	buf->in = 0;
	buf->out = 0;
	return 0;
}

void ringbuffer_destroy(ringbuffer_t *buf)
{
	free(buf->data);
}

void ringbuffer_flush(ringbuffer_t *buf)
{
	buf->in = 0;
	buf->out = 0;
}

unsigned int ringbuffer_empty(ringbuffer_t *buf)
{
	if (buf->in < buf->out)
		return buf->out - buf->in;
	else
		return buf->size - (buf->in - buf->out);
}

unsigned int ringbuffer_full(ringbuffer_t *buf)
{
	if (buf->in >= buf->out)
		return buf->in - buf->out;
	else
		return buf->size - (buf->out - buf->in);
}

int ringbuffer_write(ringbuffer_t *buf, const void *data, unsigned int size)
{
	char *b = (char *)buf->data;
	unsigned int left = ringbuffer_empty(buf);
	unsigned int wlen = MIN(size, left);
	char *to = b + buf->in;

	if (wlen == 0) {
		return 0;
	} else if (wlen < (buf->size - buf->in)) {
		memcpy(to, data, wlen);
		buf->in += wlen;
	} else {
		const char *d2 = (const char *)data + (buf->size - buf->in);
		memcpy(to, data, buf->size - buf->in);
		memcpy(b, d2, wlen - (buf->size - buf->in));
		buf->in = wlen - (buf->size - buf->in);
	}

	return wlen;
}

int ringbuffer_read(ringbuffer_t *buf, void *data, unsigned int size)
{
	const char *b = (const char *)buf->data;
	unsigned int left = ringbuffer_full(buf);
	unsigned int rlen = MIN(size, left);
	const char *from = b + buf->out;

	if (rlen == 0) {
		return 0;
	} else if (rlen < (buf->size - buf->out)) {
		memcpy(data, from, rlen);
		buf->out += rlen;
	} else {
		char *d2 = (char *)data + (buf->size - buf->out);
		memcpy(data, from, buf->size - buf->out);
		memcpy(d2, b, rlen - (buf->size - buf->out));
		buf->out = rlen - (buf->size - buf->out);
	}

	return rlen;
}
