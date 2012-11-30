#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "server.h"
#include "simpletcp.h"
#include "ringbuffer.h"

struct linefeed {
	char line[4096];
	struct linefeed *next;
};

struct server_conn {
	stcp_t *stcp;
	ringbuffer_t ring;
	struct server_conn *next;

	struct linefeed *head;
	struct linefeed *tail;
	int error;
};

struct server {
	stcp_t *stcp;
	struct server_conn *connections;
};

#define MIN(x,y) (((x)<(y))?(x):(y))

server_t *server_create(int port)
{
	server_t *srv;

	srv = (server_t *)calloc(1, sizeof(*srv));
	if (srv == NULL)
		return NULL;

	srv->stcp = stcp_listen(port);
	if (srv->stcp == NULL) {
		free(srv);
		return NULL;
	}

	return srv;
}

void      server_destroy(server_t *srv)
{
	struct server_conn *conn;

	conn = srv->connections;
	while (conn != NULL) {
		struct server_conn *p;

		p = conn;
		conn = conn->next;
		ringbuffer_destroy(&p->ring);
		stcp_close(p->stcp);
		free(p);
	}

	stcp_close(srv->stcp);
	free(srv);
}

static int server_conn_readline(struct server_conn *conn, char *buf, int len)
{
	struct linefeed *f;
	char fluff[4096];
	int rc;

	while (!conn->error && stcp_wait(conn->stcp, 0) == 0) {
		int off = 0;
		int i;

		rc = stcp_read(conn->stcp, sizeof(fluff), fluff);
		if (rc <= 0) {
			conn->error = 1;
			break;
		}

		for (;;) {
			for (i = 0; i < rc; ++i)
				if (fluff[i + off] == '\n') break;

			if (i == rc) {
				ringbuffer_write(&conn->ring, fluff, rc);
				break;
			} else {
				int v, q, c;

				f = (struct linefeed *)malloc(sizeof(*f));
				c = i + 1;
				v = ringbuffer_read(&conn->ring, f->line, sizeof(f->line));
				q = MIN((sizeof(f->line) - v) - 1, c);
				memcpy((char *)f->line + v, &fluff[off], q);
				f->line[v + q] = 0;
				f->next = NULL;
				if (conn->tail != NULL) {
					conn->tail->next = f;
					conn->tail = f;
				} else {
					conn->head = conn->tail = f;
				}

				if (c < rc) {
					off += c;
					rc -= c;
				} else {
					break;
				}
			}
		}
	}

	f = conn->head;
	if (f != NULL) {
		conn->head = conn->head->next;
		if (conn->head == NULL)
			conn->tail = NULL;
		memcpy(buf, f->line, MIN(len, 4096));
		free(f);
		return len;
	}

	return -conn->error;
}


int       server_readline(server_t *srv, char *buf, int len)
{
	struct server_conn *conn, *prev;
	stcp_t *n;

	if (stcp_accept(srv->stcp, &n, 0) == 0) {
		conn = (struct server_conn *)calloc(1, sizeof(*conn));
		if (conn != NULL) {
			conn->stcp = n;
			conn->next = srv->connections;
			ringbuffer_create(&conn->ring, 1024 * 16);
			srv->connections = conn;
			//printf("Connection accepted\n");
		} else {
			stcp_close(n);
		}
	}

	prev = NULL;
	conn = srv->connections;
	while (conn != NULL) {
		int rc;

		rc = server_conn_readline(conn, buf, len);
		if (rc < 0) {
			if (prev == NULL) {
				srv->connections = conn->next;
				prev = conn;
				conn = conn->next;
				ringbuffer_destroy(&prev->ring);
				stcp_close(prev->stcp);
				free(prev);
				prev = NULL;
			} else {
				prev->next = conn->next;
				ringbuffer_destroy(&conn->ring);
				stcp_close(conn->stcp);
				free(conn);
				conn = prev->next;
			}
			continue;
		} else if (rc > 0) {
			return 0;
		}
		prev = conn;
		conn = conn->next;
	}

	return -1;
}
