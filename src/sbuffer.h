#pragma once

#include "simpletcp.h"

typedef struct sbuffer sbuffer_t;

sbuffer_t *sbuffer_create(unsigned int size, int blocking);
void       sbuffer_destroy(sbuffer_t *buf);
int        sbuffer_set_socket(sbuffer_t *buf, stcp_t *sock);
int        sbuffer_read(sbuffer_t *buf, char *to, unsigned int len);
int        sbuffer_write(sbuffer_t *buf, const char *from, unsigned int len);
char      *sbuffer_readline(sbuffer_t *buf, char *s, unsigned int len);
int        sbuffer_printf(sbuffer_t *buf, const char *fmt, ...);
