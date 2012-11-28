#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

typedef struct ringbuffer {
	unsigned int size;
	unsigned int in;
	unsigned int out;
	void *data;
} ringbuffer_t;

int ringbuffer_create(ringbuffer_t *buf, unsigned int size);
void ringbuffer_destroy(ringbuffer_t *buf);

void ringbuffer_flush(ringbuffer_t *buf);

unsigned int ringbuffer_empty(ringbuffer_t *buf);
unsigned int ringbuffer_full(ringbuffer_t *buf);

int ringbuffer_write(ringbuffer_t *buf, const void *data, unsigned int size);
int ringbuffer_read(ringbuffer_t *buf, void *data, unsigned int size);

#endif
