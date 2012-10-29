#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "memorymapper.h"

class MMANMap : public MemoryMapper::Map {
public:
	MMANMap(int fd, void *data, int len)
	 : MemoryMapper::Map(data, len), m_fd(fd)
	{ }

	int getFD(void)
	{
		return m_fd;
	}
private:
	int m_fd;
};
MemoryMapper::Map *MemoryMapper::map(const char *fname)
{
	struct stat st;
	void *pBuf;
	int fd;

	if (stat(fname, &st))
		return NULL;

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
	munmap(map->getData(), map->getLength());
	close(static_cast<MMANMap *>(map)->getFD());
	delete static_cast<MMANMap *>(map);
}
