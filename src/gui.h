#pragma once

#include <stdarg.h>
#include "gre.h"
#include "imagemanager.h"
#include "animation.h"
#include "font.h"

class GUI {
public:
	GUI(const GRE::Dimensions &dims, bool fullscreen);
	~GUI();

	void setVideoMode(const GRE::Dimensions &dims, bool fullscreen);
	void addImage(const char *str);

	void start(void);

	int  next(void);
	int  prev(void);
	void render(void);

	void debugVPrintf(const char *fmt, va_list ap);
	void debugPrintf(const char *fmt, ...);

	void enableText(bool enabled);
	void enableSpinner(bool enabled);
	void enableFiltering(bool enabled);

	void setFadeDuration(Timestamp ms);
	void randomSort(void);
	void logicalSort(void);
	void directorySort(void);

	void randomOffset(void);

	void setDirty(void);

	int imageCount(void) const;

	int pollEvent(GRE::Event &ev);

private:
	void restartAnimation(void);
	void updateText(void);

	GRE             m_gre;
	ImageManager    m_im;
	Animator        m_anim;
	Animation      *m_animation;
	Animation      *m_spinner;
	Animation      *m_infoanim;
	bool            m_first;
	bool            m_started;
	bool            m_dirty;
	bool            m_spinning;
	bool            m_text;
	GRE::Texture   *m_textures[2];
	bool            m_textupdated;
	GRE::Texture   *m_stringtex;
	StringDrawable *m_string;
	bool            m_infoshown;
	bool            m_infoupdated;
	char            m_infotext[256];
	GRE::Texture   *m_infotex;
	StringDrawable *m_info;
	Timestamp       m_duration;
};
