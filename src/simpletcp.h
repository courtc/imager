#pragma once

typedef struct stcp stcp_t;
stcp_t *stcp_listen(int port);
int     stcp_accept(stcp_t *s, stcp_t **nsock, int timeout);
stcp_t *stcp_connect(const char *host, int port);
void    stcp_close(stcp_t *s);
int     stcp_read(stcp_t *s, int n, void *p);
int     stcp_wait(stcp_t *s, int ms);
int     stcp_write(stcp_t *s, int n, void *p);
