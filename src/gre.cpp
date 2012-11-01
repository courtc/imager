#include <stdlib.h>
#include <SDL/SDL.h>
#include <GL/glew.h>

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
 : m_dims(dims), m_fullscreen(fullscreen)
{
	SDL_Init(SDL_INIT_VIDEO);
	setVideoMode(dims, fullscreen);
}

GRE::~GRE()
{
	SDL_Quit();
}

void GRE::setVideoMode(const GRE::Dimensions &dims, bool fullscreen)
{
	if (m_render.size() != 0 && m_fullscreen != fullscreen) {
		SDL_Quit();
		SDL_Init(SDL_INIT_VIDEO);
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	SDL_SetVideoMode(fullscreen ? 0 : dims.w,
			fullscreen ? 0 : dims.h, 32,
			(SDL_OPENGL | SDL_HWSURFACE) |
			(fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE));

	if (fullscreen) {
		const SDL_VideoInfo* vinfo;
		vinfo = SDL_GetVideoInfo();
		m_dims.w = vinfo->current_w;
		m_dims.h = vinfo->current_h;
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		SDL_ShowCursor(SDL_ENABLE);
		m_dims.w = dims.w;
		m_dims.h = dims.h;
	}
	m_fullscreen = fullscreen;
	//SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_WM_SetCaption("imager", "imager");
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
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	glOrtho(0.0f,m_dims.w,m_dims.h,0.0f,1000.0f,-1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::list<const Texture *>::iterator it = m_render.begin();

	for (; it != m_render.end(); ++it) {
		renderTexture(*it);
	}

	SDL_GL_SwapBuffers();
	glFinish();
}

int GRE::pollEvent(GRE::Event &oev)
{
	SDL_Event ev;

	if (SDL_PollEvent(&ev) == 0)
		return -1;

	switch (ev.type) {
	case SDL_MOUSEBUTTONDOWN:
		switch (ev.button.button) {
		case 4:
			oev.type = GRE::Event::Prev;
			break;
		case 5:
			oev.type = GRE::Event::Next;
			break;
		default:
			return -1;
		}
		break;
	case SDL_KEYDOWN:
		switch (ev.key.keysym.sym) {
		case SDLK_LSHIFT:
			m_keystate[0][0] = true;
			return -1;
		case SDLK_LCTRL:
			m_keystate[0][1] = true;
			return -1;
		case SDLK_LMETA:
			m_keystate[0][2] = true;
			return -1;
		case SDLK_LALT:
			m_keystate[0][3] = true;
			return -1;
		case SDLK_RSHIFT:
			m_keystate[1][0] = true;
			return -1;
		case SDLK_RCTRL:
			m_keystate[1][1] = true;
			return -1;
		case SDLK_RMETA:
			m_keystate[1][2] = true;
			return -1;
		case SDLK_RALT:
			m_keystate[1][3] = true;
			return -1;
		case SDLK_ESCAPE:
			oev.type = GRE::Event::Quit;
			break;
		case SDLK_LEFT:
			oev.type = GRE::Event::Prev;
			break;
		case SDLK_RIGHT:
			oev.type = GRE::Event::Next;
			break;
		case SDLK_RETURN:
			if (m_keystate[0][3] || m_keystate[1][3])
				oev.type = GRE::Event::FullscreenToggle;
			else
				oev.type = GRE::Event::Next;
			break;
		case SDLK_SPACE:
			oev.type = GRE::Event::Next;
			break;
		case SDLK_h:
			oev.type = GRE::Event::HaltToggle;
			break;
		case SDLK_q:
			oev.type = GRE::Event::Quit;
			break;
		case SDLK_f:
			oev.type = GRE::Event::FullscreenToggle;
			break;
		case SDLK_TAB:
			if ((m_keystate[0][3] || m_keystate[1][3]) && m_fullscreen) {
				oev.type = GRE::Event::FullscreenToggle;
				break;
			}
			return -1;
		default:
			printf("key = %d\n", ev.key.keysym.sym);
			return -1;
		}
		break;
	case SDL_KEYUP:
		switch (ev.key.keysym.sym) {
		case SDLK_LSHIFT:
			m_keystate[0][0] = false;
			return -1;
		case SDLK_LCTRL:
			m_keystate[0][1] = false;
			return -1;
		case SDLK_LMETA:
			m_keystate[0][2] = false;
			return -1;
		case SDLK_LALT:
			m_keystate[0][3] = false;
			return -1;
		case SDLK_RSHIFT:
			m_keystate[1][0] = false;
			return -1;
		case SDLK_RCTRL:
			m_keystate[1][1] = false;
			return -1;
		case SDLK_RMETA:
			m_keystate[1][2] = false;
			return -1;
		case SDLK_RALT:
			m_keystate[1][3] = false;
			return -1;
		default:
			return -1;
		}
		break;
	case SDL_QUIT:
		oev.type = GRE::Event::Quit;
		break;
	case SDL_VIDEORESIZE:
		setVideoMode(GRE::Dimensions(ev.resize.w, ev.resize.h), m_fullscreen);
		return -1;
	default:
		return -1;
	}

	return 0;
}
