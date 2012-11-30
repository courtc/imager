#pragma once

typedef struct server server_t;
server_t *server_create(int port);
void      server_destroy(server_t *);
int       server_readline(server_t *, char *buf, int len);
