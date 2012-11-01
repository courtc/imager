#pragma once

#include "gre.h"
#include "imagemanager.h"
#include "animation.h"

class GUI {
public:
	GUI(const GRE::Dimensions &dims, bool fullscreen);
	~GUI();

	void setVideoMode(const GRE::Dimensions &dims, bool fullscreen);
	void addImage(const char *str);

	void next(void);
	void prev(void);
	void render(void);

	void setFadeDuration(Timestamp ms);
	void randomSort(void);
	void logicalSort(void);

	void setDirty(void);

	int imageCount(void) const;

	int pollEvent(GRE::Event &ev);

private:
	void restartAnimation(void);

	GRE           m_gre;
	ImageManager  m_im;
	Animator      m_anim;
	Animation    *m_animation;
	bool          m_started;
	bool          m_dirty;
	GRE::Texture *m_textures[2];
	Timestamp     m_duration;
};
