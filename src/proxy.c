#include <stdlib.h>
#include "proxy.h"

extern const struct proxy_ops httpproxy_ops;

static const struct proxy_ops *proxies[] = {
	&httpproxy_ops,
};

struct proxy {
	const struct proxy_ops *ops;
	proxy_context_t *ctx;
};

proxy_t *proxy_connect(const char *host, int port)
{
	proxy_context_t *ctx;
	proxy_t *p;
	int i;

	for (i = 0; i < sizeof(proxies)/sizeof(proxies[0]); ++i) {
		ctx = proxies[i]->connect(host, port);
		if (ctx != NULL) {
			p = (proxy_t *)calloc(1, sizeof(*p));
			if (p == NULL) {
				proxies[i]->close(ctx);
				return NULL;
			}

			p->ctx = ctx;
			p->ops = proxies[i];
			return p;
		}
	}
	return NULL;
}

void proxy_close(proxy_t *s)
{
	s->ops->close(s->ctx);
	free(s);
}

int  proxy_read(proxy_t *s, int n, void *p)
{
	return s->ops->read(s->ctx, n, p);
}

int  proxy_wait(proxy_t *s, int ms)
{
	return s->ops->wait(s->ctx, ms);
}

int  proxy_write(proxy_t *s, int n, const void *p)
{
	return s->ops->write(s->ctx, n, p);
}
