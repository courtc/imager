/*
 * Copyright (c) 2010-2012, Courtney Cavin
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ringbuffer.h"
#include "simpletcp.h"
#include "sbuffer.h"
#include "httpstrm.h"

#define LOG printf

struct stream {
	stcp_t *sock;
	char mimetype[256];
	uint64 pos;
	uint64 end;
	char host[256];
	char get[1024];
	int port;
	sbuffer_t *buffer;
	int live;
};

#define BUFFER_DEFAULT_SIZE 1024*128

stream_t *httpstrm_open(const char *uri)
{
	const char *p, *e;
	char *method = "HEAD";
	int relocated = 0;
	stream_t *ret;
	char header[4096];
	char host[256];
	char get[1024] = "/";
	int port = 80;
	int rc;

	if (strncmp(uri, "http://", 7))
		return NULL;

	p = uri + 7;
	e = p;

	while (*e && *e != '/' && *e != ':') e++;
	if (e - p == 0) /* zero length hostname */
		return NULL;
	if (e - p > sizeof(host) - 1) /* too long hostname */
		return NULL;

	strncpy(host, p, e - p);
	host[e - p] = 0;

	p = e;
	if (*p == ':') {
		char *me;
		p = p + 1;
		port = strtol(p, &me, 10);
		p = (const char *)me;
	}
	if (*p == '/') {
		e = p;
		while (*e) ++e;
		if (e - p != 0 && e - p < sizeof(get)) {
			strncpy(get, p, e - p);
			get[e - p] = 0;
		}
	}

	ret = (stream_t *)malloc(sizeof(*ret));
	if (ret == NULL)
		return NULL;

	ret->buffer = sbuffer_create(BUFFER_DEFAULT_SIZE, 1);

	ret->live = 0;
	ret->mimetype[0] = 0;
	ret->pos = 0;
	ret->end = (uint64)-1;
	strcpy(ret->host, host);
	strcpy(ret->get, get);
	ret->port = port;

	for (;;) {
		//LOG("Connecting to %s:%d\n", host, port);
		ret->sock = stcp_connect(host, port);
		if (ret->sock == NULL) {
			LOG("Connect failed\n");
			free(ret);
			return NULL;
		}
		//LOG("Connected\n");

		sbuffer_set_socket(ret->buffer, ret->sock);
		sbuffer_printf(ret->buffer, "%s %s HTTP/1.1\r\n", method, get);
		sbuffer_printf(ret->buffer, "Host: %s:%d\r\n", host, port);
		sbuffer_printf(ret->buffer, "User-Agent: none\r\n");
		sbuffer_printf(ret->buffer, "Accept: */*\r\n");
		sbuffer_printf(ret->buffer, "Connection: Close\r\n");
		sbuffer_printf(ret->buffer, "\r\n");

		if (sbuffer_readline(ret->buffer, header, sizeof(header))) {
			//char *p = strtok(header, "\r\n");
			//if (p != NULL) LOG("* %s\n", p);
			if (!strncmp(header, "HTTP/1.1 302", 12) ||
					!strncmp(header, "HTTP/1.1 301", 12)) {
					relocated = 1;
			} else if (!strncmp(header, "HTTP/1.0 200 OK", 15)) {
			} else if (!strncmp(header, "ICY 200 OK", 10)) {
				ret->live = 1;
			} else if (!strncmp(header, "HTTP/1.1 200 OK", 15)) {
			} else if (!strcmp(method, "HEAD")) {
				LOG("HEAD failed, attempting GET\n");
				method = "GET";
				stcp_close(ret->sock);
				continue;
			} else {
				LOG("Fetch failure: %s\n", header);
				stcp_close(ret->sock);
				sbuffer_destroy(ret->buffer);
				free(ret);
				return NULL;
			}
		} else if (!strcmp(method, "HEAD")) {
			LOG("HEAD failed, attempting GET\n");
			method = "GET";
			stcp_close(ret->sock);
			continue;
		}
		break;
	}

	rc = -1;
	while (sbuffer_readline(ret->buffer, header, sizeof(header))) {
		char *p;
		char *tok;
		char *name;
		char *value;

		/* final CRLF */
		if (header[0] == 0 || header[0] == '\r' || header[0] == '\n') {
			rc = 0;
			break;
		}

		/* field-name */
		p = strtok_r(header, " :\r\n", &tok);
		if (p == NULL) {
			rc = -1;
			break;
		}
		name = p;

		/* field-value */
		p = strtok_r(NULL, "\r\n", &tok);
		while (*p == ' ') ++p;
		value = p ? p : "";

		if (!strcasecmp("content-type", name)) {
			strncpy(ret->mimetype, value, sizeof(ret->mimetype));
			ret->mimetype[sizeof(ret->mimetype) - 1] = 0;
			if ((p = strchr(ret->mimetype, ';')) != NULL)
				*p = 0;
			if ((p = strchr(ret->mimetype, ',')) != NULL)
				*p = 0;
			//LOG("found content-type: %s\n", ret->mimetype);
		} else if (!strcasecmp("content-length", name)) {
			ret->end = strtoll(value, 0, 0);
			//LOG("found content-length: %llu\n", ret->end);
		} else if (relocated && !strcasecmp("location", name)) {
			stcp_close(ret->sock);
			sbuffer_destroy(ret->buffer);
			free(ret);

			return httpstrm_open(value);
		}
	}

	stcp_close(ret->sock);
	ret->sock = NULL;
	if (rc || relocated) {
		LOG("Header invalid\n");
		sbuffer_destroy(ret->buffer);
		free(ret);
		return NULL;
	}

	rc = httpstrm_seek(ret, 0, SEEK_SET);
	if (rc) {
		sbuffer_destroy(ret->buffer);
		free(ret);
		return NULL;
	}

	return ret;
}

void        httpstrm_close(stream_t *v)
{
	if (v->sock != NULL)
		stcp_close(v->sock);
	sbuffer_destroy(v->buffer);
	free(v);
}

int         httpstrm_read(stream_t *v, void *data, unsigned int len)
{
	int rc;

	if (v->sock == NULL)
		return -1;

	rc = sbuffer_read(v->buffer, data, len);
	if (rc > 0)
		v->pos += rc;

	return rc;
}
int         httpstrm_mime(stream_t *v, char *mime, unsigned int len)
{
	if (!v->mimetype[0])
		return -1;

	strncpy(mime, v->mimetype, len);
	mime[len - 1] = 0;
	return 0;
}

int         httpstrm_seek(stream_t *v, uint64 pos, int whence)
{
	char header[4096];
	int rc;
	uint64 npos;

	switch (whence) {
	case SEEK_SET:
		npos = pos;
		break;
	case SEEK_CUR:
		npos = v->pos + pos;
		break;
	case SEEK_END:
		if (v->end == (uint64)-1)
			return -1;
		npos = v->end;
		break;
	}

	if (v->end != (uint64)-1 && npos > v->end)
		return -1;

	if (v->pos == npos && v->sock != NULL)
		return 0;

	//LOG("Seeking to: %llu\n", npos);
	if (v->sock != NULL)
		stcp_close(v->sock);

	v->sock = stcp_connect(v->host, v->port);
	if (v->sock == NULL) {
		LOG("Connect failed\n");
		return -1;
	}

	sbuffer_set_socket(v->buffer, v->sock);
	sbuffer_printf(v->buffer, "GET %s HTTP/1.1\r\n", v->get);
	sbuffer_printf(v->buffer, "Host: %s:%d\r\n", v->host, v->port);
	sbuffer_printf(v->buffer, "User-Agent: none\r\n");
	if (npos != 0)
		sbuffer_printf(v->buffer, "Range: bytes=%llu-\r\n", npos);
	sbuffer_printf(v->buffer, "Accept: */*\r\n");
	sbuffer_printf(v->buffer, "Connection: Keep-Alive\r\n");
	sbuffer_printf(v->buffer, "\r\n");

	if (sbuffer_readline(v->buffer, header, sizeof(header))) {
		//char *p = strtok(header, "\r\n");
		//if (p != NULL) LOG("* %s\n", p);
		if (!strncmp(header, "HTTP/1.1 206 Partial", 20)) {
		} else if (!strncmp(header, "HTTP/1.1 200 OK", 15)) {
		} else if (!strncmp(header, "ICY 200 OK", 10)) {
		} else {
			stcp_close(v->sock);
			LOG("Fetch failure: %s\n", header);
			v->sock = NULL;
			return -1;
		}
	}

	rc = -1;
	while (sbuffer_readline(v->buffer, header, sizeof(header))) {
		/* final CRLF */
		if (header[0] == 0 || header[0] == '\r' || header[0] == '\n') {
			rc = 0;
			break;
		}
	}

	if (rc != 0) {
		LOG("Header invalid\n");
		stcp_close(v->sock);
		v->sock = NULL;
		return -1;
	}

	v->pos = npos;
	return 0;
}

uint64     httpstrm_length(stream_t *v)
{
	return v->end;
}
