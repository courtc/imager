#pragma once

#include "gre.h"
#include "thread.h"
#include "imageloader.h"

class ImageManager : public Runnable {
public:
	ImageManager(GRE &gre);
	~ImageManager();

	void start(void);
	void randomOffset(void);
	void randomSort(void);
	void logicalSort(void);
	void directorySort(void);
	void append(const char *image);
	int imageCount(void) const;

	GRE::Texture *reload(void);
	GRE::Texture *next(void);
	GRE::Texture *prev(void);

	int getLoadCount(void) const;

	class String {
	public:
		String(const String &str);
		String(const char *str);
		String();
		~String();
		char *getText(void);
		const char *getText(void) const;

	private:
		char *m_text;
	};
private:
	GRE::Texture *index(int dir);
	void run(void);

	Image *cacheDir(int dir);

	Semaphore      m_sem;
	Mutex          m_lock;
	WaitQ<Image *> m_cache[2];
	GRE::Texture  *m_texture;
	GRE::Texture  *m_previous;
	Image         *m_current;

	GRE      &m_gre;
	String  **m_images;
	int       m_size;
	int       m_count;
	int       m_index;
	Thread    m_thread;
	int       m_loadcount;
	bool      m_started;
};
