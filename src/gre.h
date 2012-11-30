#pragma once

#include <list>

class GRE {
public:
	struct Event {
		enum Type {
			Next,
			Prev,
			Quit,
			HaltToggle,
			FullscreenToggle,
		};

		Type type;
	};
	class Texture {
	protected:
		Texture() : m_alpha(1.0f) { };
		float m_alpha;
	public:
		void setAlpha(float v)
		{ m_alpha = v; }
		float getAlpha(void) const
		{ return m_alpha; }
	};

	struct Dimensions {
		Dimensions(int _w, int _h)
		 : w(_w), h(_h)
		{ }
		int w, h;
	};

	GRE(const Dimensions &dims, bool fullscreen);
	~GRE();

	void setVideoMode(const Dimensions &dims, bool fullscreen);

	Texture *loadTexture(const void *pData, const Dimensions &);
	void unloadTexture(Texture *texture);

	void setSpinnerAlpha(float val);

	void clearTexturePasses(void);
	void addTexturePass(const Texture *texture);
	void remTexturePass(const Texture *texture);

	void render(void);

	int pollEvent(Event &ev);
private:
	void renderTexture(const Texture *texture);

	std::list<const Texture *> m_render;
	Texture       *m_spinner;
	Dimensions     m_dims;
	bool           m_keystate[2][4];
	bool           m_fullscreen;
	void          *m_priv;
};
