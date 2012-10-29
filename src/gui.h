#pragma once

#include "gre.h"
#include "imagemanager.h"

class GUI {
public:
	GUI(const GRE::Dimensions &dims, bool fullscreen);
	~GUI();

	void setVideoMode(const GRE::Dimensions &dims, bool fullscreen);
	void addImage(const char *str);

	void next(void);
	void prev(void);
	void render(void);

	void randomSort(void);
	void logicalSort(void);

	int imageCount(void) const;

	int pollEvent(GRE::Event &ev);

private:
	GRE          m_gre;
	ImageManager m_im;
	bool         m_started;
};
