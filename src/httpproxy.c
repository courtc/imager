#include <stdlib.h>
#include <string.h>

#include "simpletcp.h"
#include "sbuffer.h"
#include "proxy.h"

struct proxy_context {
	stcp_t *stcp;
	sbuffer_t *buffer;
};

static const char const *envvars[] = {
	"http_proxy",
	"HTTP_PROXY",
};

proxy_context_t *httpproxy_connect(const char *host, int port)
{
	char *proxy, *phost, *pport;
	const char *proxyhost = NULL;
	int proxyport = 80;
	proxy_context_t *p;
	char buf[4096];
	stcp_t *stcp;
	int i = 0;
	int rc;

	while (proxyhost == NULL && i < sizeof(envvars) / sizeof(envvars[0]))
		proxyhost = getenv(envvars[i++]);

	if (proxyhost == NULL)
		return NULL;

	phost = proxy = strdup(proxyhost);
	if (!(strncasecmp(proxy, "http://", 7)))
		phost += 7;

	if ((pport = strrchr(phost, ':')) != NULL) {
		*pport = 0;
		pport += 1;
		proxyport = strtol(pport, 0, 0);
	}

	stcp = stcp_connect_np(phost, proxyport);
	if (stcp == NULL) {
		free(proxy);
		return NULL;
	}

	free(proxy);

	p = (proxy_context_t *)calloc(1, sizeof(proxy_context_t));
	if (p == NULL) {
		stcp_close(stcp);
		return NULL;
	}

	p->stcp = stcp;
	p->buffer = sbuffer_create(1024 * 64, 1);
	if (p->buffer == NULL) {
		stcp_close(stcp);
		free(p);
	}

	sbuffer_set_socket(p->buffer, p->stcp);
	sbuffer_printf(p->buffer, "CONNECT %s:%d HTTP/1.1\r\n", host, port);
	sbuffer_printf(p->buffer, "Host: %s:%d\r\n", host, port);
	sbuffer_printf(p->buffer, "\r\n");

	if (sbuffer_readline(p->buffer, buf, sizeof(buf))) {
		if (!strncmp(buf, "HTTP/1.0 200 OK", 15)) {
		} else if (!strncmp(buf, "HTTP/1.1 200 OK", 15)) {
		} else if (!strncmp(buf, "HTTP/1.1 2", 10)) {
			int resp = strtol(buf + 8, 0, 0);
			if (resp < 200 || resp >= 300)
				goto err;
		} else {
			goto err;
		}
	} else {
		goto err;
	}

	rc = -1;
	while (sbuffer_readline(p->buffer, buf, sizeof(buf))) {
		/* final CRLF */
		if (buf[0] == 0 || buf[0] == '\r' || buf[0] == '\n') {
			rc = 0;
			break;
		}
	}

	return p;
err:
	sbuffer_destroy(p->buffer);
	stcp_close(p->stcp);
	free(p);
	return NULL;
}

void httpproxy_close(proxy_context_t *s)
{
	sbuffer_destroy(s->buffer);
	stcp_close(s->stcp);
	free(s);
}

int  httpproxy_read(proxy_context_t *s, int n, void *p)
{
	return sbuffer_read(s->buffer, (char *)p, n);
}

int  httpproxy_wait(proxy_context_t *s, int ms)
{
	return sbuffer_wait(s->buffer, ms);
}

int  httpproxy_write(proxy_context_t *s, int n, const void *p)
{
	return sbuffer_write(s->buffer, p, n);
}

const struct proxy_ops httpproxy_ops = {
	.connect = httpproxy_connect,
	.close = httpproxy_close,
	.read = httpproxy_read,
	.write = httpproxy_write,
	.wait = httpproxy_wait,
};
