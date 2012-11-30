#include <stdlib.h>
#include <GL/gl.h>
#include <stdio.h>
#include "zdl/zdl.h"

#include "gre.h"

#define GLASSERT() \
  do { \
    static int warned = 0; \
    GLint err = glGetError(); \
    if (!warned) { \
      if (err != 0) { \
        fprintf(stderr, "%s:%d: GL Error %d\n", __func__, __LINE__, err); \
        warned = 1; \
      } \
    } \
  } while (0)

static const unsigned char spinner[9 * 9] = {
	0x11, 0x11, 0x11, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11,
	0x11, 0x11, 0x10, 0x02, 0x03, 0x04, 0x10, 0x11, 0x11,
	0x11, 0x10, 0x01, 0x10, 0x10, 0x10, 0x05, 0x10, 0x11,
	0x10, 0x00, 0x10, 0x11, 0x11, 0x11, 0x10, 0x06, 0x10,
	0x10, 0x0f, 0x10, 0x11, 0x11, 0x11, 0x10, 0x07, 0x10,
	0x10, 0x0e, 0x10, 0x11, 0x11, 0x11, 0x10, 0x08, 0x10,
	0x11, 0x10, 0x0d, 0x10, 0x10, 0x10, 0x09, 0x10, 0x11,
	0x11, 0x11, 0x10, 0x0c, 0x0b, 0x0a, 0x10, 0x11, 0x11,
	0x11, 0x11, 0x11, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11,
};

static const unsigned int spinner_pallete[18] = {
	0xff000000, 0xff001000, 0xff002000, 0xff003000, 0xff004000,
	0xff005000, 0xff006000, 0xff007000, 0xff008000, 0xff009000,
	0xff00a000, 0xff00b000, 0xff00c000, 0xff00d000, 0xff00e000,
	0xff00f000, 0x80f9384c, 0x00000000,
};

class GLTexture : public GRE::Texture {
public:
	GLTexture(const void *pData, const GRE::Dimensions &dims)
	 : m_dims(dims)
	{
		glGenTextures(1, &m_tex);
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				m_dims.w, m_dims.h, 0,
				GL_RGBA, GL_UNSIGNED_BYTE,
				(void *)pData);
	}

	void bind(void) const
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_tex);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	}

	const GRE::Dimensions &getDimensions(void) const
	{
		return m_dims;
	}

	virtual ~GLTexture()
	{
		glDeleteTextures(1, &m_tex);
	}

	GRE::Dimensions m_dims;
	GLuint m_tex;
};

class GLSpinner : public GRE::Texture {
public:
	GLSpinner()
	: m_dims(32,32)
	{
		for (int i = 0; i < 16 * 16; ++i)
			m_data[i] = 0;
		for (int y = 0; y < 9; ++y) {
			for (int x = 0; x < 9; ++x) {
				unsigned int idx = spinner[y * 9 + x];
				unsigned int col = spinner_pallete[idx];

				m_data[(y*2 +  0) * 32 + (x*2 +  0)] = col;
				m_data[(y*2 +  1) * 32 + (x*2 +  0)] = col;
				m_data[(y*2 +  1) * 32 + (x*2 +  1)] = col;
				m_data[(y*2 +  0) * 32 + (x*2 +  1)] = col;
				m_data[(y*2 + 14) * 32 + (x*2 + 14)] = col;
				m_data[(y*2 + 15) * 32 + (x*2 + 14)] = col;
				m_data[(y*2 + 15) * 32 + (x*2 + 15)] = col;
				m_data[(y*2 + 14) * 32 + (x*2 + 15)] = col;
			}
		}

		glGenTextures(1, &m_tex);
		GLASSERT();
		bind();
		GLASSERT();

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				m_dims.w, m_dims.h, 0,
				GL_RGBA, GL_UNSIGNED_BYTE,
				(void *)m_data);
		m_rotation = 0.0f;
	}

	float advance(void)
	{
		m_rotation += 360.0f / 30.0f;
		return m_rotation;
	}

	void bind(void) const
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_tex);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	}

	const GRE::Dimensions &getDimensions(void) const
	{
		return m_dims;
	}

	~GLSpinner()
	{
		glDeleteTextures(1, &m_tex);
	}

private:
	unsigned int m_data[32 * 32];
	float m_rotation;
	GRE::Dimensions m_dims;
	GLuint m_tex;
};

GRE::GRE(const GRE::Dimensions &dims, bool fullscreen)
 : m_dims(dims), m_fullscreen(fullscreen), m_priv(0)
{
	setVideoMode(dims, fullscreen);
	m_spinner = new GLSpinner;
}

GRE::~GRE()
{
	ZDL::Window *m_window = static_cast<ZDL::Window *>(m_priv);
	if (m_window != NULL)
		delete m_window;
	delete m_spinner;
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

void GRE::setSpinnerAlpha(float val)
{
	m_spinner->setAlpha(val);
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

	GLSpinner *spinner = static_cast<GLSpinner *>(m_spinner);
	float rotation = spinner->advance();

	spinner->bind();
	glTranslatef(8.0, 8.0, 0.0f);
	glRotatef(rotation, 0.0f, 0.0f, 1.0f);

	glBegin(GL_QUADS);
		glColor4f(1.0f,1.0f,1.0f, spinner->getAlpha());

		glTexCoord2d(0.0,0.0); glVertex2f(-8.0f,-8.0f);
		glTexCoord2d(1.0,0.0); glVertex2f( 8.0f,-8.0f);
		glTexCoord2d(1.0,1.0); glVertex2f( 8.0f, 8.0f);
		glTexCoord2d(0.0,1.0); glVertex2f(-8.0f, 8.0f);

	glEnd();


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
