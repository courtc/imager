#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "imagemanager.h"

ImageManager::String::String(const ImageManager::String &str)
{
	if (str.m_text != NULL)
		m_text = strdup(str.m_text);
	else
		m_text = NULL;
}
ImageManager::String::String(const char *str)
{
	m_text = strdup(str);
}
ImageManager::String::String()
{
	m_text = NULL;
}
ImageManager::String::~String()
{
	if (m_text != NULL)
		free(m_text);
}
char *ImageManager::String::getText(void)
{
	return m_text;
}
const char *ImageManager::String::getText(void) const
{
	return m_text;
}

static inline int wrap(int value, int size)
{
	value = value % size;
	if (value < 0)
		return size + value;
	return value;
}

static int xstrrandcmp(const void *, const void *)
{
	return (RAND_MAX / 2) - (int)rand();
}

static int xstrverscmp(const void *p1, const void *p2)
{
	return strverscmp((*(const ImageManager::String **)p1)->getText(),
			  (*(const ImageManager::String **)p2)->getText());
}

ImageManager::ImageManager(GRE &gre)
 : m_sem(0), m_gre(gre), m_thread(*this)
{
	m_texture = NULL;
	m_current = NULL;
	m_previous = NULL;
	m_count  = 0;
	m_size   = 256;
	m_images = new String*[256];
	m_index  = 0;
	m_loadcount = 0;
	m_started = false;
}

ImageManager::~ImageManager()
{
	if (m_previous != NULL)
		m_gre.unloadTexture(m_previous);
	if (m_texture != NULL)
		m_gre.unloadTexture(m_texture);

	m_sem.post();
	m_thread.join();

	for (int i = 0; i < m_count; ++i)
		delete m_images[i];

	delete[] m_images;
}

void ImageManager::start(void)
{
	if (!m_started) {
		m_started = true;
		m_sem.post();
	}
}

void ImageManager::randomSort(void)
{
	m_lock.lock();
	qsort(m_images, m_count, sizeof(m_images[0]), xstrrandcmp);
	m_lock.unlock();
}

void ImageManager::logicalSort(void)
{
	m_lock.lock();
	qsort(m_images, m_count, sizeof(m_images[0]), xstrverscmp);
	m_lock.unlock();
}

void ImageManager::directorySort(void)
{
	m_lock.lock();
	if (m_count <= 1) {
		m_lock.unlock();
		return;
	}
	struct dirid {
		int index;
		int count;
	} *dirid = new struct dirid[m_count];
	const char *l, *r, *le, *re;
	int di = 0;
	qsort(m_images, m_count, sizeof(m_images[0]), xstrverscmp);
	l = m_images[0]->getText();
	le = strrchr(l, '/');
	dirid[0].index = 0;
	dirid[0].count = 1;
	for (int i = 1; i < m_count; ++i) {
		r = m_images[i]->getText();
		re = strrchr(r, '/');
		if (le == NULL || re == NULL) {
			if (le == re)
				dirid[di].count++;
			else {
				dirid[++di].index = i;
				dirid[di].count = 1;
			}
		} else {
			if (!strncmp(l, r, le - l))
				dirid[di].count++;
			else {
				dirid[++di].index = i;
				dirid[di].count = 1;
			}
		}
		le = re;
		l = r;
	}
	if (di++ != 0) {
		qsort(dirid, di, sizeof(dirid[0]), xstrrandcmp);
		String **n = new String*[m_size];
		int idx = 0;
		for (int i = 0; i < di; ++i) {
			for (int j = 0; j < dirid[i].count; ++j)
				n[idx++] = m_images[dirid[i].index + j];
		}
		delete[] m_images;
		m_images = n;
	}

	delete[] dirid;
	m_lock.unlock();
}

void ImageManager::randomOffset(void)
{
	m_lock.lock();
	m_index = wrap(m_index + rand(), m_count);
	m_lock.unlock();
}

void ImageManager::append(const char *image)
{
	m_lock.lock();
	if (m_count == m_size) {
		String **n = new String*[m_size << 1];
		for (int i = 0; i < m_count; ++i)
			n[i] = m_images[i];
		delete[] m_images;
		m_images = n;
		m_size <<= 1;
	}
	m_images[m_count++] = new String(image);
	m_lock.unlock();
}

GRE::Texture *ImageManager::index(int dir)
{
	Image *image = cacheDir(dir);
	if (image == NULL)
		return NULL;

	if (m_previous != NULL)
		m_gre.unloadTexture(m_previous);
	m_previous = m_texture;
	m_texture = m_gre.loadTexture(image->getData(), image->getDimensions());

	return m_texture;
}

GRE::Texture *ImageManager::next(void)
{
	return index(1);
}

GRE::Texture *ImageManager::reload(void)
{
	return index(0);
}

GRE::Texture *ImageManager::prev(void)
{
	return index(-1);
}

int ImageManager::currentImage(void) const
{
	return m_index + 1;
}

int ImageManager::imageCount(void) const
{
	return m_count;
}

void ImageManager::currentImageName(char *buf, int len)
{
	m_lock.lock();
	strncpy(buf, m_images[m_index]->getText(), len);
	buf[len-1]=0;
	m_lock.unlock();
}

int ImageManager::getLoadCount(void) const
{
	return m_loadcount;
}

void ImageManager::run(void)
{
	unsigned int waittime = 10;

	while (m_sem.wait(waittime) != 0);

	waittime = 0;
	while (m_sem.wait(waittime) != 0) {
		waittime = 100;
		m_lock.lock();
		if (m_count == 0) {
			m_lock.unlock();
			continue;
		}
		if (m_current == NULL && m_count > 0) {
			m_current = m_loader.loadImage(m_images[m_index]->getText());
			waittime = 0;
			m_loadcount += (m_current != NULL);
		}
		m_lock.unlock();

		for (int i = 0; i < 2; ++i) {
			m_lock.lock();
			if (m_count == 0) {
				m_lock.unlock();
				continue;
			}
			int n = m_cache[i].count();
			int s = i == 0 ? -1 : 1;
			if (n > 2) {
				m_loader.unloadImage(m_cache[i].popBack());
				waittime = 0;
				m_loadcount--;
			} else if (n < 2) {
				const char *name;
				Image *image;
				int index;

				index = wrap(m_index + (n + 1) * s, m_count);
				//printf("Loading (%d + %d = %d), \"%s\"...", m_index, (n + 1) * s, index, m_images[index]->getText());
				//fflush(stdout);
				name = m_images[index]->getText();
				m_lock.unlock();
				image = m_loader.loadImage(name);
				m_lock.lock();
				//printf(" done\n");
				waittime = 0;
				if (image != NULL) {
					m_cache[i].pushBack(image);
					m_loadcount++;
				} else {
					fprintf(stderr, "Removing \"%s\", as it is unloadable\n",
						m_images[index]->getText());
					delete m_images[index];
					memmove(&m_images[index],
						&m_images[index + 1],
						(m_count - (index + 1)) * sizeof(m_images[0]));
					m_count--;
				}
			}
			m_lock.unlock();
		}
	}

	while (m_cache[0].count() > 0)
		m_loader.unloadImage(m_cache[0].popBack());
	while (m_cache[1].count() > 0)
		m_loader.unloadImage(m_cache[1].popBack());
	if (m_current != NULL)
		m_loader.unloadImage(m_current);
}

Image *ImageManager::cacheDir(int dir)
{
	Image *ret;
	int idx = (dir + 1) >> 1;

	if (m_count == 0)
		return NULL;
	if (dir == 0)
		return m_current;

	ret = m_cache[idx].popFront(0);
	if (ret == NULL)
		return NULL;

	m_lock.lock();
	if (m_current != NULL)
		m_cache[!idx].pushFront(m_current);
	m_current = ret;
	m_index = wrap(m_index + dir, m_count);
	m_lock.unlock();

	return ret;
}
