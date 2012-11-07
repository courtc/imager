#include <stdlib.h>
#include <GL/gl.h>
#include <stdio.h>
#include "zdl/zdl.h"

#include "gre.h"

class GLTexture : public GRE::Texture {
public:
	GLTexture(const void *pData, const GRE::Dimensions &dims)
	 : m_dims(dims)
	{
		glGenTextures(1, &tex);
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				dims.w, dims.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				(void *)pData);
	}

	void bind(void) const
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	}

	const GRE::Dimensions &getDimensions(void) const
	{
		return m_dims;
	}

	~GLTexture()
	{
		glDeleteTextures(1, &tex);
	}

	GRE::Dimensions m_dims;
	GLuint tex;
};

GRE::GRE(const GRE::Dimensions &dims, bool fullscreen)
 : m_dims(dims), m_fullscreen(fullscreen), m_priv(0)
{
	setVideoMode(dims, fullscreen);
}

GRE::~GRE()
{
	ZDL::Window *m_window = static_cast<ZDL::Window *>(m_priv);
	if (m_window != NULL)
		delete m_window;
}

void GRE::setVideoMode(const GRE::Dimensions &dims, bool fullscreen)
{
	zdl_flags_t flags = fullscreen ? ZDL_FLAG_FULLSCREEN : 0;
	ZDL::Window *m_window;

	if (m_priv == NULL) {
		m_window = new ZDL::Window(dims.w, dims.h, flags);
		m_priv = m_window;
	} else {
		m_window = static_cast<ZDL::Window *>(m_priv);
	}
	m_window->setFlags(flags);
	m_window->showCursor(!fullscreen);
	m_window->setSize(dims.w, dims.h);
	m_window->getSize(&m_dims.w, &m_dims.h);
	m_fullscreen = fullscreen;
	m_window->setTitle("imager", "imager");
	glViewport(0, 0, m_dims.w, m_dims.h);
}

GRE::Texture *GRE::loadTexture(const void *pData, const GRE::Dimensions &dims)
{
	return new GLTexture(pData, dims);
}

void GRE::unloadTexture(GRE::Texture *texture)
{
	delete static_cast<const GLTexture *>(texture);
}

void GRE::clearTexturePasses(void)
{
	m_render.clear();
}

void GRE::addTexturePass(const GRE::Texture *texture)
{
	m_render.push_back(texture);
}

void GRE::remTexturePass(const GRE::Texture *texture)
{
	m_render.remove(texture);
}

void GRE::renderTexture(const GRE::Texture *texture)
{
	const GLTexture *tex = static_cast<const GLTexture *>(texture);
	Dimensions image_dims = tex->getDimensions();
	float screen_r = (float)m_dims.w / m_dims.h;
	float image_r = (float)image_dims.w / image_dims.h;
	float x, y, w, h;
	if (screen_r > image_r) {
		h = m_dims.h;
		w = h * image_r;
	} else {
		w = m_dims.w;
		h = w / image_r;
	}
	x = ((float)m_dims.w - w) / 2;
	y = ((float)m_dims.h - h) / 2;

	tex->bind();
	glBegin(GL_QUADS);
		glColor4f(1.0f,1.0f,1.0f,tex->getAlpha());

		glTexCoord2d(0.0,0.0); glVertex2f(x+0.0,y+0.0);
		glTexCoord2d(1.0,0.0); glVertex2f(x+  w,y+0.0);
		glTexCoord2d(1.0,1.0); glVertex2f(x+  w,y+  h);
		glTexCoord2d(0.0,1.0); glVertex2f(x+0.0,y+  h);

	glEnd();
}

void GRE::render(void)
{
	ZDL::Window *m_window = static_cast<ZDL::Window *>(m_priv);
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f,m_dims.w,m_dims.h,0.0f,1.0f,-1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::list<const Texture *>::iterator it = m_render.begin();

	for (; it != m_render.end(); ++it) {
		renderTexture(*it);
	}

	m_window->swap();
}

int GRE::pollEvent(GRE::Event &oev)
{
	ZDL::Window *m_window = static_cast<ZDL::Window *>(m_priv);
	struct zdl_event ev;
	int rc;

	do {
		rc = 0;

		if (m_window->pollEvent(&ev) != 0)
			return -1;

		switch (ev.type) {
		case ZDL_EVENT_BUTTONPRESS:
			switch (ev.button.button) {
			case 4:
				oev.type = GRE::Event::Prev;
				break;
			case 5:
				oev.type = GRE::Event::Next;
				break;
			default:
				rc = -1;
				break;
			}
			break;
		case ZDL_EVENT_KEYPRESS:
			switch (ev.key.sym) {
			case ZDL_KEYSYM_ESCAPE:
				oev.type = GRE::Event::Quit;
				break;
			case ZDL_KEYSYM_LEFT:
				oev.type = GRE::Event::Prev;
				break;
			case ZDL_KEYSYM_RIGHT:
				oev.type = GRE::Event::Next;
				break;
			case ZDL_KEYSYM_RETURN:
				if (ev.key.modifiers & ZDL_KEYMOD_ALT)
					oev.type = GRE::Event::FullscreenToggle;
				else
					oev.type = GRE::Event::Next;
				break;
			case ZDL_KEYSYM_SPACE:
				oev.type = GRE::Event::Next;
				break;
			case ZDL_KEYSYM_H:
				oev.type = GRE::Event::HaltToggle;
				break;
			case ZDL_KEYSYM_Q:
				oev.type = GRE::Event::Quit;
				break;
			case ZDL_KEYSYM_F:
				oev.type = GRE::Event::FullscreenToggle;
				break;
			case ZDL_KEYSYM_TAB:
				if ((ev.key.modifiers & ZDL_KEYMOD_ALT) && m_fullscreen) {
					oev.type = GRE::Event::FullscreenToggle;
					break;
				}
				rc = -1;
				break;
			default:
				rc = -1;
				break;
			}
			break;
		case ZDL_EVENT_EXIT:
			oev.type = GRE::Event::Quit;
			break;
		case ZDL_EVENT_RECONFIGURE:
			m_dims.w = ev.reconfigure.width;
			m_dims.h = ev.reconfigure.height;
			glViewport(0, 0, m_dims.w, m_dims.h);
			rc = -1;
			break;
		case ZDL_EVENT_EXPOSE:
			render();
			rc = -1;
			break;
		default:
			rc = -1;
			break;
		}
	} while (rc != 0);

	return 0;
}
