#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>

#include "memorymapper.h"
extern "C" {
#include "httpstrm.h"
}

class MMANMap : public MemoryMapper::Map {
public:
	MMANMap(int fd, void *data, int len)
	 : MemoryMapper::Map(data, len), m_fd(fd)
	{ }

	~MMANMap()
	{
		munmap(getData(), getLength());
		close(m_fd);
	}

private:
	int m_fd;
};

class StreamMap : public MemoryMapper::Map {
public:
	StreamMap(void *data, int len)
	 : MemoryMapper::Map(data, len)
	{ }

	~StreamMap()
	{
		free(getData());
	}
};


MemoryMapper::Map *MemoryMapper::map(const char *fname)
{
	struct stat st;
	void *pBuf;
	int fd;

	if (stat(fname, &st)) {
		stream_t *strm;
		uint64 len;
		int rc;
		if (strncmp(fname, "http://", 7))
			return NULL;

		strm = httpstrm_open(fname);
		if (strm == NULL)
			return NULL;

		len = httpstrm_length(strm);
		if (len > 16 * 1024 * 1024)
			len = 16 * 1024 * 1024;
		pBuf = malloc(len);
		if (pBuf == NULL) {
			httpstrm_close(strm);
			return NULL;
		}

		rc = httpstrm_read(strm, pBuf, len);
		if (rc < 0) {
			httpstrm_close(strm);
			return NULL;
		}
		httpstrm_close(strm);
		return new StreamMap(pBuf, rc);

	}

	fd = open(fname, O_RDONLY);
	if (fd == -1)
		return NULL;

	pBuf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (pBuf == MAP_FAILED) {
		close(fd);
		return NULL;
	}

	return new MMANMap(fd, pBuf, st.st_size);
}

void MemoryMapper::unmap(MemoryMapper::Map *map)
{
	delete map;
}
