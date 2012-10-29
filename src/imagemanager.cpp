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
	if (value < 0)
		return size + value;
	if (value >= size)
		return value - size;
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
	m_count  = 0;
	m_size   = 256;
	m_images = new String*[256];
	m_index  = 0;
	m_loadcount = 0;
}

ImageManager::~ImageManager()
{
	if (m_texture != NULL)
		m_gre.unloadTexture(m_texture);

	m_sem.post();
	m_thread.join();

	for (int i = 0; i < m_count; ++i)
		delete m_images[i];

	delete[] m_images;
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

void ImageManager::append(const char *image)
{
	if (m_count == m_size) {
		String **n = new String*[m_size << 1];
		for (int i = 0; i < m_count; ++i)
			n[i] = m_images[i];
		delete[] m_images;
		m_images = n;
		m_size <<= 1;
	}
	m_images[m_count++] = new String(image);
}

GRE::Texture *ImageManager::next(void)
{
	Image *image = cacheDir(1);

	if (m_texture != NULL)
		m_gre.unloadTexture(m_texture);
	m_texture = m_gre.loadTexture(image->getData(), image->getDimensions());

	return m_texture;
}

GRE::Texture *ImageManager::reload(void)
{
	Image *image = cacheDir(0);
	if (image == NULL)
		return NULL;

	if (m_texture != NULL)
		m_gre.unloadTexture(m_texture);
	m_texture = m_gre.loadTexture(image->getData(), image->getDimensions());

	return m_texture;
}


GRE::Texture *ImageManager::prev(void)
{
	Image *image = cacheDir(-1);

	if (m_texture != NULL)
		m_gre.unloadTexture(m_texture);
	m_texture = m_gre.loadTexture(image->getData(), image->getDimensions());

	return m_texture;
}

int ImageManager::imageCount(void) const
{
	return m_count;
}

int ImageManager::getLoadCount(void) const
{
	return m_loadcount;
}


void ImageManager::run(void)
{
	unsigned int waittime = 0;

	while (m_sem.wait(waittime) != 0) {
		waittime = 100;
		m_lock.lock();
		if (m_current == NULL && m_count > 0) {
			m_current = ImageLoader::loadImage(m_images[0]->getText());
			m_loadcount += (m_current != NULL);
		}
		m_lock.unlock();

		for (int i = 0; i < 2; ++i) {
			m_lock.lock();
			if (m_loadcount == m_count) {
				m_lock.unlock();
				continue;
			}
			int n = m_cache[i].count();
			int s = i == 0 ? -1 : 1;
			if (n > 2) {
				ImageLoader::unloadImage(m_cache[i].popBack());
				m_loadcount--;
			} else if (n < 2) {
				Image *image;
				int index = wrap(m_index + (n + 1) * s, m_count);
				//printf("Loading (%d + %d = %d), \"%s\"\n", m_index, (n + 1) * s, index, m_images[index]->getText());
				image = ImageLoader::loadImage(m_images[index]->getText());
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
					waittime = 0;
				}
			}
			m_lock.unlock();
		}
	}
}

Image *ImageManager::cacheDir(int dir)
{
	Image *ret;
	int idx = (dir + 1) >> 1;

	if (dir == 0)
		return m_current;

	ret = m_cache[idx].popFront();

	m_lock.lock();
	if (m_current != NULL)
		m_cache[!idx].pushFront(m_current);
	m_current = ret;
	m_index = wrap(m_index + dir, m_count);
	m_lock.unlock();

	return ret;
}