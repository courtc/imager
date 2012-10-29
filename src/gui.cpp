#include "gui.h"

GUI::GUI(const GRE::Dimensions &dims, bool fullscreen)
 : m_gre(dims, fullscreen), m_im(m_gre), m_started(false)
{
}
GUI::~GUI() { }

void GUI::setVideoMode(const GRE::Dimensions &dims, bool fullscreen)
{
	m_gre.setVideoMode(dims, fullscreen);
	m_gre.display(m_im.reload());
}

void GUI::addImage(const char *str)
{
	m_im.append(str);
}

void GUI::next(void)
{
	m_gre.display(m_im.next());
}

void GUI::prev(void)
{
	m_gre.display(m_im.prev());
}

void GUI::render(void)
{
	if (m_started == false && m_im.getLoadCount() != 0) {
		GRE::Texture *tex = m_im.reload();
		if (tex != 0) {
			m_gre.display(tex);
			m_started = true;
		}
	}
	m_gre.render();
}

void GUI::randomSort(void)
{
	m_im.randomSort();
}

void GUI::logicalSort(void)
{
	m_im.logicalSort();
}

int GUI::imageCount(void) const
{
	return m_im.imageCount();
}

int GUI::pollEvent(GRE::Event &ev)
{
	return m_gre.pollEvent(ev);
}
