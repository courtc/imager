#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#endif
#ifdef __linux__
#include <net/if.h>
#include <signal.h>
#endif

#include "simpletcp.h"
#include "proxy.h"

struct stcp {
	int fd;
	proxy_t *proxy;
};

stcp_t *stcp_listen(int port)
{
	stcp_t *s;
	struct sockaddr_in ma;
	int yes = 1;
	int rc;

	s = (stcp_t*)calloc(1,sizeof(stcp_t));

	s->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (s->fd == -1)
		return NULL;

	rc = setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(int));
	if (rc == -1)
		return NULL;

	ma.sin_family = AF_INET;
	ma.sin_port = htons(port);
	ma.sin_addr.s_addr = INADDR_ANY;
	memset(&ma.sin_zero, 0, 8);

	rc = bind(s->fd, (struct sockaddr *)&ma, sizeof(struct sockaddr));
	if (rc == -1)
		return NULL;

	rc = listen(s->fd, 10);
	if (rc == -1)
		return NULL;

	return s;
}

int     stcp_accept(stcp_t *s,stcp_t **nsock,int timeout)
{
	struct sockaddr_in ta;
	struct timeval *tvp = NULL;
	struct timeval tv;
	unsigned int insz = sizeof(ta);
	int rc;
	fd_set fds;

	if (timeout != -1) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		tvp = &tv;
	}

	FD_ZERO(&fds);
	FD_SET(s->fd, &fds);

	rc = select(s->fd + 1, &fds, NULL, NULL, tvp);
	if (rc == 0)
		return -EAGAIN;
	else if (rc < 0)
		return -errno;

	*nsock = (stcp_t*)calloc(1,sizeof(stcp_t));

	(*nsock)->fd = accept(s->fd, (struct sockaddr *)&ta, &insz);
	if( (*nsock)->fd < 0)
		return -errno;

#ifdef __linux__
	/* Handle SIGPIPE in a non-terminaty manner */
	signal(SIGPIPE,SIG_IGN);
#endif
#ifdef _WIN32
	{
		u_long iMode=1;
		ioctlsocket((*nsock)->fd,FIONBIO,&iMode);
	}
#endif

#if 0
	rc = setsockopt(nsock->fd, SOL_TCP, TCP_NODELAY, &yes, sizeof(int));
	if (rc == -1)
		return -errno;
#endif

	return 0;
}

stcp_t *stcp_connect_np(const char *host, int port)
{
	int rc;
	struct sockaddr_in addr;
	struct hostent *he;
	stcp_t *ret;
	int yes = 1;

	ret = (stcp_t*)calloc(1,sizeof(stcp_t));
	if (ret == NULL)
		return NULL;

	he = gethostbyname(host);
	if (he == NULL)
		return NULL;

	ret->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (ret->fd == -1)
		return NULL;

	addr.sin_family = AF_INET;
	addr.sin_addr = *((struct in_addr *)he->h_addr);
	addr.sin_port = htons(port);
	memset(&addr.sin_zero, 0, 8);

	rc = connect(ret->fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	if (rc == -1)
		return NULL;

	rc = setsockopt(ret->fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&yes, sizeof(int));
	if (rc == -1)
		return NULL;
#if 1
	{
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 500000;
		rc = setsockopt(ret->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
	}
#endif

	return ret;
}

stcp_t *stcp_connect(const char *host, int port)
{
	stcp_t *ret;
	proxy_t *proxy;

	proxy = proxy_connect(host, port);
	if (proxy == NULL)
		return stcp_connect_np(host, port);

	ret = (stcp_t*)calloc(1,sizeof(stcp_t));
	if (ret == NULL)
		return NULL;

	ret->proxy = proxy;

	return ret;
}

void    stcp_close(stcp_t *s)
{
	if (s->proxy == NULL) {
#ifdef _WIN32
		closesocket(s->fd);
#else
		close(s->fd);
#endif
	} else {
		proxy_close(s->proxy);
	}

	free(s);
}

int     stcp_read(stcp_t *s,int n,void *p)
{
	if (s->proxy == NULL)
		return recv(s->fd, p, n, 0);
	else
		return proxy_read(s->proxy, n, p);
}

int     stcp_wait(stcp_t *s, int ms)
{
	if (s->proxy == NULL) {
		struct timeval tv;
		fd_set fds;
		int rc;

		FD_ZERO(&fds);
		FD_SET(s->fd, &fds);
		tv.tv_sec = ms / 1000;
		tv.tv_usec = (ms % 1000) * 1000;
		rc = select(s->fd + 1, &fds, NULL, NULL, &tv);
		if (rc > 0)
			return 0;
	} else {
		return proxy_wait(s->proxy, ms);
	}

	return -1;
}

int     stcp_write(stcp_t *s, int n, const void *p)
{
	if (s->proxy == NULL)
		return send(s->fd, (const char *)p, n, 0);
	else
		return proxy_write(s->proxy, n, p);
}
