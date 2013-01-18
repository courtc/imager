#ifndef _PROXY_H_
#define _PROXY_H_

typedef struct proxy_context proxy_context_t;
struct proxy_ops {
	proxy_context_t *(* connect)(const char *host, int port);
	void (* close)(proxy_context_t *s);
	int  (* read)(proxy_context_t *s, int n, void *p);
	int  (* wait)(proxy_context_t *s, int ms);
	int  (* write)(proxy_context_t *s, int n, const void *p);
};

typedef struct proxy proxy_t;
proxy_t *proxy_connect(const char *host, int port);
void proxy_close(proxy_t *s);
int  proxy_read(proxy_t *s, int n, void *p);
int  proxy_wait(proxy_t *s, int ms);
int  proxy_write(proxy_t *s, int n, const void *p);

#endif
