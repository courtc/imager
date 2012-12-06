#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
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
		update(pData, dims);
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

	void update(const void *pData, const GRE::Dimensions &dims)
	{
		m_dims = dims;
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				m_dims.w, m_dims.h, 0,
				GL_RGBA, GL_UNSIGNED_BYTE,
				(void *)pData);
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
		m_rotation = 0.0f;
		update(m_data, m_dims);
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
	void update(const void *pData, const GRE::Dimensions &dims)
	{
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				m_dims.w, m_dims.h, 0,
				GL_RGBA, GL_UNSIGNED_BYTE,
				(void *)pData);
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

const char *bicubic_fragment =
"uniform sampler2D u_Texture;\n"
"uniform vec2 u_Scale;\n"

"vec4 cubic(float s)\n"
"{\n"
"	vec4 c0 = vec4(-0.5,    0.1666, 0.3333, -0.3333);\n"
"	vec4 c1 = vec4( 1.0,    0.0,   -0.5,     0.5);\n"
"	vec4 c2 = vec4( 0.0,    0.0,   -0.5,     0.5);\n"
"	vec4 c3 = vec4(-0.6666, 0.0,    0.8333,  0.1666);\n"
"	vec4 t = ((c0 * s + c1) * s + c2) * s + c3;\n"
"	vec2 a = vec2(1.0 / t.z, 1.0 / t.w);\n"
"	t.xy = t.xy * a + vec2(1.0 + s, 1.0 - s);\n"
"	return t;\n"
"}\n"

"vec4 filter(sampler2D tex, vec2 texsize, vec2 texcoord)\n"
"{\n"
"	vec4 off;\n"
"	vec2 pt = 1.0 / texsize;\n"
"	vec2 fcoord = fract(texcoord * texsize + vec2(0.5, 0.5));\n"
"	vec4 cx = cubic(fcoord.x);\n"
"	vec4 cy = cubic(fcoord.y);\n"
"	off.xz = cx.xy * vec2(-pt.x, pt.x);\n"
"	off.yw = cy.xy * vec2(-pt.y, pt.y);\n"
"	vec4 xy = texture2D(tex, texcoord + off.xy);\n"
"	vec4 xw = texture2D(tex, texcoord + off.xw);\n"
"	vec4 zy = texture2D(tex, texcoord + off.zy);\n"
"	vec4 zw = texture2D(tex, texcoord + off.zw);\n"
"	return mix(mix(zw, zy, cy.z), mix(xw, xy, cy.z), cx.z);\n"
"}\n"

"void main()\n"
"{\n"
"	vec4 color = filter(u_Texture, u_Scale, gl_TexCoord[0].st);\n"
"	color.a = gl_Color.a;\n"
"	gl_FragColor = color;\n"
"}\n";

const char *bicubic_vertex =
"void main()\n"
"{\n"
"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"	gl_FrontColor = gl_Color;\n"
"	gl_Position = ftransform();\n"
"}\n";

#if 1
#define dbg_uniform(name)
#else
#define dbg_uniform(name) \
	fprintf(stderr, "Uniform \"%s\" not found\n", name)
#endif

class GLShader {
public:
	GLShader();
	~GLShader();

	void loadVertexText(const char *);
	void loadFragmentText(const char *);
	void compile(void);
	void bind(void);

	void setUniform(const char *name, float);
	void setUniform(const char *name, float, float);
	void setUniform(const char *name, float, float, float);
	void setUniform(const char *name, float, float, float, float);
	void setUniform(const char *name, float *, int count);

	void setUniform(const char *name, int);
	void setUniform(const char *name, int, int);
	void setUniform(const char *name, int, int, int);
	void setUniform(const char *name, int, int, int, int);
	void setUniform(const char *name, int *, int count);


private:
	GLuint loadText(const char *, GLuint type);

	GLuint m_program;
	GLuint m_vertex;
	GLuint m_fragment;
};

GLShader::GLShader()
 : m_program(0), m_vertex(-1), m_fragment(-1)
{ }

GLShader::~GLShader()
{
	if (m_program != 0)
		glDeleteProgram(m_program);
	if (m_vertex != (GLuint)-1)
		glDeleteShader(m_vertex);
	if (m_fragment != (GLuint)-1)
		glDeleteShader(m_fragment);
}

GLuint GLShader::loadText(const char *text, GLuint type)
{
	GLuint ret;

	ret = glCreateShader(type);

	glShaderSource(ret, 1, &text, NULL);
	glCompileShader(ret);

	GLint compiled;
	// Check the compile status
	glGetShaderiv(ret, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		fprintf(stderr, "Failed to compile\n");
		GLint infoLen = 0;
		glGetShaderiv(ret, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = (char *)malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(ret, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling program:\n%s\n", infoLog);
			free(infoLog);
		}
	}

	return ret;
}

void GLShader::loadVertexText(const char *txt)
{
	m_vertex = loadText(txt, GL_VERTEX_SHADER);
}

void GLShader::loadFragmentText(const char *txt)
{
	m_fragment = loadText(txt, GL_FRAGMENT_SHADER);
}

void GLShader::compile(void)
{
	GLuint ret;
	GLint  linked;

	if (m_program != 0)
		return;

	if ((m_vertex == (GLuint)-1) || (m_fragment == (GLuint)-1)) {
		fprintf(stderr, "Failed to load shaders - "
				"reverting to defaults\n");
		m_program = 0;
		return;
	}

	ret = glCreateProgram();
	glAttachShader(ret, m_vertex);
	glAttachShader(ret, m_fragment);

	glLinkProgram(ret);

	// Check the link status
	glGetProgramiv(ret, GL_LINK_STATUS, &linked);
	if (!linked) {
		fprintf(stderr, "Linking shader failed\n");
		GLint infoLen = 0;
		glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = (char*) malloc(sizeof(char) * infoLen);
			glGetProgramInfoLog(ret, infoLen, NULL, infoLog);
			fprintf(stderr, "Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		m_program = 0;
		glDeleteProgram(ret);
		return;
	}

	m_program = ret;
}

void GLShader::bind(void)
{
	glUseProgram(m_program);
	GLASSERT();
}

void GLShader::setUniform(const char *name, float a)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform1f(u, a);
	GLASSERT();
}
void GLShader::setUniform(const char *name, float a, float b)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform2f(u, a, b);
	GLASSERT();
}
void GLShader::setUniform(const char *name, float a, float b, float c)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform3f(u, a, b, c);
	GLASSERT();
}
void GLShader::setUniform(const char *name, float a, float b, float c, float d)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform4f(u, a, b, c, d);
	GLASSERT();
}
void GLShader::setUniform(const char *name, float *v, int count)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform1fv(u, count, v);
	GLASSERT();
}

void GLShader::setUniform(const char *name, int a)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform1i(u, a);
	GLASSERT();
}
void GLShader::setUniform(const char *name, int a, int b)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform2i(u, a, b);
	GLASSERT();
}
void GLShader::setUniform(const char *name, int a, int b, int c)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform3i(u, a, b, c);
	GLASSERT();
}
void GLShader::setUniform(const char *name, int a, int b, int c, int d)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform4i(u, a, b, c, d);
	GLASSERT();
}
void GLShader::setUniform(const char *name, int *v, int count)
{
	GLint u = glGetUniformLocation(m_program, name);
	if (u == -1)
		dbg_uniform(name);
	glUniform1iv(u, count, v);
	GLASSERT();
}

static GLShader g_shader;

GRE::GRE(const GRE::Dimensions &dims, bool fullscreen)
 : m_dims(dims), m_fullscreen(fullscreen), m_filtering(true), m_priv(0)
{
	setVideoMode(dims, fullscreen);
	m_spinner = new GLSpinner;
	g_shader.loadVertexText(bicubic_vertex);
	g_shader.loadFragmentText(bicubic_fragment);
	g_shader.compile();
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

GRE::Texture *GRE::getSpinnerTexture(void)
{
	return m_spinner;
}

void GRE::enableFiltering(bool v)
{
	m_filtering = v;
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

void GRE::clearTextureOverlays(void)
{
	m_overlay.clear();
}

void GRE::addTextureOverlay(const GRE::Texture *texture)
{
	m_overlay.push_back(texture);
}

void GRE::remTextureOverlay(const GRE::Texture *texture)
{
	m_overlay.remove(texture);
}

static float scale(float rs, float cs)
{
	return 2.0f * cs;
}

void GRE::renderTextureOverlay(const GRE::Texture *overlay)
{
	const GLTexture *tex = static_cast<const GLTexture *>(overlay);
	float x, y, w, h;

	x = tex->getPosition().x;
	y = tex->getPosition().y;
	w = tex->getDimensions().w;
	h = tex->getDimensions().h;

	tex->bind();
	glBegin(GL_QUADS);
		glColor4f(1.0f,1.0f,1.0f, tex->getAlpha());

		glTexCoord2d(0.0,0.0); glVertex2f(x+0.0,y+0.0);
		glTexCoord2d(1.0,0.0); glVertex2f(x+  w,y+0.0);
		glTexCoord2d(1.0,1.0); glVertex2f(x+  w,y+  h);
		glTexCoord2d(0.0,1.0); glVertex2f(x+0.0,y+  h);

	glEnd();
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

	if (m_filtering) {
		g_shader.setUniform("u_Texture", 0);
		g_shader.setUniform("u_Scale",
			scale(image_dims.w, w),
			scale(image_dims.h, h));
	}

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
	if (m_window->getFlags() & ZDL_FLAG_FLIP_Y)
		glOrtho(0.0f,m_dims.w,0.0f,m_dims.h,1.0f,-1.0f);
	else
		glOrtho(0.0f,m_dims.w,m_dims.h,0.0f,1.0f,-1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::list<const Texture *>::iterator it = m_render.begin();

	if (m_filtering)
		g_shader.bind();
	else
		glUseProgram(0);

	for (; it != m_render.end(); ++it)
		renderTexture(*it);

	glUseProgram(0);

	for (it = m_overlay.begin(); it != m_overlay.end(); ++it)
		renderTextureOverlay(*it);


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
			case ZDL_KEYSYM_D:
				oev.type = GRE::Event::TextToggle;
				break;
			case ZDL_KEYSYM_H:
				oev.type = GRE::Event::HaltToggle;
				break;
			case ZDL_KEYSYM_Q:
				oev.type = GRE::Event::Quit;
				break;
			case ZDL_KEYSYM_F:
				if (ev.key.modifiers & ZDL_KEYMOD_SHIFT)
					oev.type = GRE::Event::FilterToggle;
				else
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
