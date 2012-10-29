/*
 * Copyright (c) 2011, Courtney Cavin
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
#include <stdio.h>

struct magic {
	const char *magic;
	const char *mask;
	unsigned int len;
	unsigned int off;
};

static int mime_matches_magicx(char *text, int or0_and1,
		const struct magic *m, int count)
{
	char buf[32];
	int match = or0_and1;
	int i;

	for (i = 0; i < count; ++i) {
		int res;

		memcpy(buf, text + m[i].off, m[i].len);

		if (m[i].mask != NULL) {
			unsigned int j;

			for (j = 0; j < m[i].len; ++j)
				buf[j] &= m[i].mask[j];
		}

		res = !memcmp(buf, m[i].magic, m[i].len);

		if (or0_and1)
			match &= res;
		else
			match |= res;

		if (match != or0_and1)
			break;
	}

	return match;
}

static const struct magic mkv_magic[] = { { "\x1a\x45\xdf\xa3",  NULL, 4, 0 } };
static const struct magic wav_magic[] = { { "WAV",               NULL, 3, 8 } };
static const struct magic jpg_magic[] = { { "\xff\xd8",          NULL, 2, 0 } };
static const struct magic png_magic[] = { { "\x89PNG",           NULL, 4, 0 } };
static const struct magic m4a_magic[] = { { "ftypM4A",           NULL, 7, 4 } };

static const struct magic asf_magic[] = {
	{ "0&\xb2u",     NULL,  4, 0 },
	{ "[Reference]", NULL, 11, 0 },
};

static const struct magic avi_magic[] = {
	{ "RIFF", NULL, 4, 0 },
	{ "AVI",  NULL, 3, 8 },
};

static const struct magic mp4_magic[] = {
	{ "ftypmp41", NULL, 8, 4 },
	{ "ftypmp42", NULL, 8, 4 },
	{ "ftypisom", NULL, 8, 4 },
	{ "ftypMSNV", NULL, 8, 4 },
	{ "ftypM4V",  NULL, 7, 4 },
	{ "ftypf4v",  NULL, 7, 4 },
};

static const struct magic mp3_magic[] = {
	{ "\xff\xfb", NULL, 2, 0 },
	{ "\xff\xfa", NULL, 2, 0 },
	{ "ID3"     , NULL, 3, 0 },
};

static const struct magic g3p_magic[] = {
	{ "ftyp3ge", NULL, 7, 4 },
	{ "ftyp3gg", NULL, 7, 4 },
	{ "ftyp3gp", NULL, 7, 4 },
	{ "ftyp3gs", NULL, 7, 4 },
};

static const struct magic g32_magic[] = { { "ftyp3g2", NULL, 7, 4 }, };

static const struct magic mpg_magic[] = {
	{ "\x47\x3f\xff\x10", NULL, 4, 0 },
	{ "\x00\x00\x01\xb3", NULL, 4, 0 },
	{ "\x00\x00\x01\xba", NULL, 4, 0 },
};

static const struct magic mov_magic[] = {
	{ "mdat",   NULL, 4, 12 },
	{ "mdat",   NULL, 4,  4 },
	{ "moov",   NULL, 4,  4 },
	{ "ftypqt", NULL, 6,  4 },
};

static const struct magic flv_magic[] = {
	{ "FLV\x01",   NULL, 4, 0 },
};

static const struct magic m2t_magic[] = {
	{ "\x47\x40\x00\x10", "\xff\x40\x00\xdf", 4, 0 },
};

static const struct magic_assoc {
	const char *mime;
	int or0_and1;
	const struct magic *magic;
	int n;
} magic_map[] = {
	{ "video/x-msvideo", 1, avi_magic, 2 },
	{ "audio/mp4",       0, m4a_magic, 1 },
	{ "audio/mpeg",      0, mp3_magic, 3 },
	{ "video/mpeg",      0, mpg_magic, 3 },
	{ "video/mp4",       0, mp4_magic, 6 },
	{ "video/x-matroska",0, mkv_magic, 1 },
	{ "video/x-ms-asf",  0, asf_magic, 2 },
	{ "video/mp4",       0, mov_magic, 4 },
	{ "video/3gpp2",     0, g32_magic, 1 },
	{ "video/3gpp",      0, g3p_magic, 4 },
	{ "video/x-flv",     0, flv_magic, 1 },
	{ "video/mp2t",      0, m2t_magic, 1 },
	{ "audio/x-wav",     0, wav_magic, 1 },
	{ "image/png",       0, png_magic, 1 },
	{ "image/jpeg",      0, jpg_magic, 1 },
};

int mime_file(const char *fname, char *mime, int len)
{
	char buf[32];
	FILE *fp;
	int cnt;
	int i;

	cnt = sizeof(magic_map)/sizeof(magic_map[0]);

	fp = fopen(fname, "rb");
	if (fp == NULL)
		return -1;

	memset(buf, 0, 32);
	fread(buf, 1, 32, fp);

	fclose(fp);

	for (i = 0; i < cnt; ++i) {
		const struct magic_assoc *a = &magic_map[i];
		if (mime_matches_magicx(buf, a->or0_and1, a->magic, a->n)) {
			strncpy(mime, a->mime, len);
			mime[len - 1] = 0;
			return 0;
		}

	}

	return -1;
}
