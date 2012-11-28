#pragma once

typedef unsigned long long uint64;
typedef struct stream stream_t;
int       httpstrm_seek(stream_t *v, uint64 pos, int whence);
int       httpstrm_mime(stream_t *v, char *mime, unsigned int len);
int       httpstrm_read(stream_t *v, void *data, unsigned int len);
void      httpstrm_close(stream_t *v);
stream_t *httpstrm_open(const char *uri);
uint64    httpstrm_length(stream_t *v);
