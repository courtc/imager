#include <stdlib.h>
#include <stdio.h>

#include "memorymapper.h"

MemoryMapper::Map *MemoryMapper::map(const char *fname)
{
	unsigned long len;
	void *pBuf;
	FILE *fp;

	fp = fopen(fname, "rb");
	if (fp == NULL)
		return NULL;

	fseek(fp, 0L, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	pBuf = malloc(len);
	if (pBuf == NULL) {
		fclose(fp);
		return NULL;
	}

	if (fread(pBuf, 1, len, fp) != len) {
		free(pBuf);
		fclose(fp);
		return NULL;
	}
	fclose(fp);

	return new Map(pBuf, len);
}

void MemoryMapper::unmap(MemoryMapper::Map *map)
{
	free(map->getData());
	delete map;
}
