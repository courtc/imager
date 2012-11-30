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

GUI::GUI(const GRE::Dimensions &dims, bool fullscreen)
 : m_gre(dims, fullscreen), m_im(m_gre), m_first(false), m_started(false), m_dirty(true)
{
	m_textures[0] = NULL;
	m_textures[1] = NULL;
	m_animation = NULL;
	m_duration = 0;
}

GUI::~GUI()
{
	if (m_animation != NULL) {
		m_anim.remove(m_animation);
		delete m_animation;
	}
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

void GUI::addImage(const char *str)
{
	m_im.append(str);
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

void GUI::start(void)
{
	m_im.start();
	m_started = true;
}

void GUI::next(void)
{
	if (m_textures[1] != NULL)
		m_gre.remTexturePass(m_textures[1]);
	m_textures[1] = m_textures[0];
	m_textures[0] = m_im.next();
	if (m_textures[0] != NULL) {
		restartAnimation();
		m_gre.addTexturePass(m_textures[0]);
		m_dirty = true;
		m_first = m_started = true;
	}
}

void GUI::prev(void)
{
	if (m_textures[1] != NULL)
		m_gre.remTexturePass(m_textures[1]);
	m_textures[1] = m_textures[0];
	m_textures[0] = m_im.prev();
	if (m_textures[0] != NULL) {
		restartAnimation();
		m_gre.addTexturePass(m_textures[0]);
		m_dirty = true;
		m_first = m_started = true;
	}
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
			}
		}
	}

	if (m_dirty) {
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
