#include <math.h>
#include <stdio.h>
#include "gui.h"

class GUIAnimation : public Animation {
public:
	GUIAnimation(GUI &gui, GRE::Texture *p[2], Timestamp ms)
	 : Animation(0.0, 1.0, ms), m_gui(gui), m_finished(false)
	{
		m_textures[0] = p[0];
		m_textures[1] = p[1];
	}

	void update(double val)
	{
		if (m_finished)
			return;
		if (val > 1.0)
			val = 1.0;
		if (m_textures[1] != NULL)
			m_textures[1]->setAlpha(1.0 - val);
		else
			val = 1.0;
		if ((1.0 - val) < 0.00001)
			m_finished = true;
		if (m_textures[0] != NULL)
			m_textures[0]->setAlpha(val);

		m_gui.setDirty();
	}

private:
	GUI          &m_gui;
	bool          m_finished;
	GRE::Texture *m_textures[2];
};

class GUISimpleAnim : public Animation {
public:
	GUISimpleAnim(GUI &gui, GRE::Texture *texture, bool on)
	 : Animation(texture->getAlpha(), on ? 1.0 : 0.0, 250), m_gui(gui),
	 	m_texture(texture),  m_target(on ? 1.0 : 0.0), m_finished(false)
	{
	}

	void update(double val)
	{
		if (m_finished)
			return;
		if (val > 1.0) val = 1.0;
		if (val < 0.0) val = 0.0;
		if (fabs(m_target - val) < 0.00001) {
			m_finished = true;
			val = m_target;
		}
		m_texture->setAlpha(val);
		m_gui.setDirty();
	}

	bool finished(void) const { return m_finished; }

private:
	GUI          &m_gui;
	GRE::Texture *m_texture;
	bool          m_target;
	bool          m_finished;
};

GUI::GUI(const GRE::Dimensions &dims, bool fullscreen)
 : m_gre(dims, fullscreen), m_im(m_gre), m_first(false), m_started(false), m_dirty(true), m_text(false)
{
	m_infoshown = false;
	m_infoupdated = false;
	m_textupdated = false;
	m_spinning = true;
	m_textures[0] = NULL;
	m_textures[1] = NULL;
	m_animation = NULL;
	m_spinner = NULL;
	m_infoanim = NULL;
	m_duration = 0;

	m_string = new StringDrawable("Loading...", 0xffdddddd, 0x0);
	m_stringtex = m_gre.loadTexture(m_string->getData(), m_string->getDimensions());
	m_stringtex->setPosition(GRE::Position(18, 0));

	m_info = new StringDrawable(" ", 0xffdddd00, 0x0);
	m_infotex = m_gre.loadTexture(m_info->getData(), m_info->getDimensions());
	m_infotex->setPosition(GRE::Position(18, 18));
	m_gre.addTextureOverlay(m_infotex);
}

GUI::~GUI()
{
	if (m_animation != NULL) {
		m_anim.remove(m_animation);
		delete m_animation;
	}
	if (m_spinner != NULL) {
		m_anim.remove(m_spinner);
		delete m_spinner;
	}
	if (m_infoanim != NULL) {
		m_anim.remove(m_infoanim);
		delete m_infoanim;
	}

	m_gre.unloadTexture(m_stringtex);
	delete m_string;

	m_gre.unloadTexture(m_infotex);
	delete m_info;
}

void GUI::setVideoMode(const GRE::Dimensions &dims, bool fullscreen)
{
	m_gre.setVideoMode(dims, fullscreen);
	m_textures[1] = NULL;
	m_textures[0] = m_im.reload();
	m_gre.clearTexturePasses();
	if (m_textures[0] != NULL) {
		m_gre.addTexturePass(m_textures[0]);
		m_dirty = true;
	}
}


void GUI::updateText(void)
{
	if (!m_started || !m_text)
		return;

	m_textupdated = true;
}

void GUI::addImage(const char *str)
{
	m_im.append(str);
	updateText();
}

void GUI::restartAnimation(void)
{
	if (m_animation != NULL) {
		m_anim.remove(m_animation);
		delete m_animation;
		m_animation = NULL;
	}
	if (m_duration != 0) {
		m_animation = new GUIAnimation(*this, m_textures, m_duration);
		m_anim.add(m_animation);
	} else {
		m_gre.remTexturePass(m_textures[1]);
		m_textures[1] = NULL;
		m_dirty = true;
	}
}

void GUI::enableText(bool enabled)
{
	if (enabled == m_text)
		return;

	m_text = enabled;
	if (m_text) {
		m_gre.addTextureOverlay(m_stringtex);
		updateText();
	} else {
		m_gre.remTextureOverlay(m_stringtex);
	}
	m_dirty = true;
}

void GUI::debugVPrintf(const char *fmt, va_list ap)
{
	vsnprintf(m_infotext, sizeof(m_infotext), fmt, ap);
	m_infoupdated = true;

	if (m_infoanim != NULL) {
		m_anim.remove(m_infoanim);
		delete m_infoanim;
	}
	m_infoanim = new GUISimpleAnim(*this, m_infotex, true);
	m_anim.add(m_infoanim);
}

void GUI::debugPrintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	debugVPrintf(fmt, ap);
	va_end(ap);
}

void GUI::enableFiltering(bool enabled)
{
	debugPrintf("filtering: %s", enabled ? "on" : "off");
	m_gre.enableFiltering(enabled);
	m_dirty = true;
}

void GUI::enableSpinner(bool enabled)
{
	if (m_spinning == enabled)
		return;

	if (m_spinner != NULL) {
		m_anim.remove(m_spinner);
		delete m_spinner;
	}
	m_spinner = new GUISimpleAnim(*this, m_gre.getSpinnerTexture(), enabled);
	m_anim.add(m_spinner);
	m_spinning = enabled;
	m_dirty = true;
}

void GUI::start(void)
{
	m_im.start();
	m_started = true;
	updateText();
}

int GUI::next(void)
{
	GRE::Texture *tex = m_im.next();
	if (tex == NULL)
		return -1;
	if (m_textures[1] != NULL)
		m_gre.remTexturePass(m_textures[1]);
	m_textures[1] = m_textures[0];
	m_textures[0] = tex;
	restartAnimation();
	m_gre.addTexturePass(m_textures[0]);
	m_dirty = m_first = m_started = true;
	updateText();
	return 0;
}

int GUI::prev(void)
{
	GRE::Texture *tex = m_im.prev();
	if (tex == NULL)
		return -1;
	if (m_textures[1] != NULL)
		m_gre.remTexturePass(m_textures[1]);
	m_textures[1] = m_textures[0];
	m_textures[0] = tex;
	restartAnimation();
	m_gre.addTexturePass(m_textures[0]);
	m_dirty = m_first = m_started = true;
	updateText();
	return 0;
}

void GUI::render(void)
{
	if (!m_started)
		return;

	m_anim.step();
	if (m_first == false && m_im.getLoadCount() != 0) {
		GRE::Texture *tex = m_im.reload();
		if (tex != 0) {
			m_textures[1] = NULL;
			m_textures[0] = tex;
			m_gre.clearTexturePasses();
			if (m_textures[0] != NULL) {
				m_gre.addTexturePass(m_textures[0]);
				m_first = true;
				m_dirty = true;
				updateText();
			}
		}
	}

	if (m_textupdated) {
		char buf[512];
		char name[512];

		if (m_im.imageCount() == 0) {
			snprintf(buf, sizeof(buf), "No images loaded...");
		} else {
			m_im.currentImageName(name, sizeof(name));
			snprintf(buf, sizeof(buf), "(%4d/%4d) %s",
					m_im.currentImage(), m_im.imageCount(),
					name);
		}
		m_string->setText(buf);
		m_stringtex->update(m_string->getData(), m_string->getDimensions());
		m_textupdated = false;
		m_dirty = true;
	}

	if (m_infoupdated) {
		m_info->setText(m_infotext);
		m_infotex->update(m_info->getData(), m_info->getDimensions());
		m_infoupdated = false;
		m_infoshown = true;
		m_dirty = true;
	}

	if (m_infoanim != NULL && m_infoanim->finished()) {
		if (m_infoshown && Time::MS() - 2000 > m_infoanim->getEnd()) {
			m_anim.remove(m_infoanim);
			delete m_infoanim;
			m_infoanim = new GUISimpleAnim(*this, m_infotex, false);
			m_anim.add(m_infoanim);
			m_infoshown = false;
		} else if (!m_infoshown) {
			m_anim.remove(m_infoanim);
			delete m_infoanim;
			m_infoanim = NULL;
		}
	}

	if (m_dirty || m_spinning) {
		m_gre.render();
		m_dirty = false;
	}
}

void GUI::setDirty(void)
{
	m_dirty = true;
}

void GUI::randomSort(void)
{
	m_im.randomSort();
}

void GUI::logicalSort(void)
{
	m_im.logicalSort();
}

void GUI::directorySort(void)
{
	m_im.directorySort();
}

void GUI::randomOffset(void)
{
	m_im.randomOffset();
}

void GUI::setFadeDuration(Timestamp ms)
{
	m_duration = ms;
}

int GUI::imageCount(void) const
{
	return m_im.imageCount();
}

int GUI::pollEvent(GRE::Event &ev)
{
	return m_gre.pollEvent(ev);
}
