#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "ringbuffer.h"
#include "sbuffer.h"

struct sbuffer {
	stcp_t *sock;
	ringbuffer_t ring;
};

#define MIN(x,y) (((x)<(y))?(x):(y))

sbuffer_t *sbuffer_create(unsigned int size, int blocking)
{
	sbuffer_t *buf;
	int rc;

	buf = (sbuffer_t *)calloc(1, sizeof(*buf));
	if (buf == NULL)
		return NULL;

	rc = ringbuffer_create(&buf->ring, size);
	if (rc != 0) {
		free(buf);
		return NULL;
	}

	return buf;
}

int sbuffer_set_socket(sbuffer_t *buf, stcp_t *sock)
{
	ringbuffer_flush(&buf->ring);
	buf->sock = sock;

	return 0;
}

void sbuffer_destroy(sbuffer_t *buf)
{
	ringbuffer_destroy(&buf->ring);
	free(buf);
}

int        sbuffer_wait(sbuffer_t *buf, int ms)
{
	unsigned int filled = ringbuffer_full(&buf->ring);

	if (filled > 0)
		return 0;

	return stcp_wait(buf->sock, ms);
}

int sbuffer_read(sbuffer_t *buf, char *to, unsigned int len)
{
	unsigned int filled = ringbuffer_full(&buf->ring);
	unsigned int fin;
	char tmp[1024];

	if (filled >= len)
		return ringbuffer_read(&buf->ring, to, len);

	fin = ringbuffer_read(&buf->ring, to, filled);
	do {
		unsigned int avail = ringbuffer_empty(&buf->ring);
		unsigned int toread = MIN(avail, sizeof(tmp));
		int olen;

		olen = stcp_read(buf->sock, toread, tmp);
		if (olen <= 0) {
			if (olen == 0 || errno == EAGAIN)
				break;
			return -1;
		}
		ringbuffer_write(&buf->ring, tmp, olen);
		fin += ringbuffer_read(&buf->ring, to + fin, len - fin);
	} while (fin < len);

	return fin;
}

int sbuffer_write(sbuffer_t *buf, const char *from, unsigned int len)
{
	return stcp_write(buf->sock, len, (void *)from);
}

char *sbuffer_readline(sbuffer_t *buf, char *s, unsigned int len)
{
	char *p = s;
	int r;
	if (len == 0)
		return NULL;
	if (len == 1) {
		*s = 0;
		return s;
	}
	do {
		r = sbuffer_read(buf, p, 1);
		if (r == -1)
			return NULL;
		*(p += r) = 0;
	} while (r > 0 && *(p - r) != '\n' && (p - s) < (int)(len - 1));

	return (p - s) ? s : 0;
}

int sbuffer_printf(sbuffer_t *buf, const char *fmt, ...)
{
	int len;
	va_list argp;
	char tmp[2048];

	va_start(argp, fmt);
	len = vsnprintf(tmp, sizeof(tmp), fmt, argp);
	va_end(argp);

	return sbuffer_write(buf, tmp, len);
}
