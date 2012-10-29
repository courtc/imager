#pragma once

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
	protected: Texture() { };
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

	void display(const Texture *texture);

	void render(void);

	int pollEvent(Event &ev);
private:
	Dimensions     m_dims;
	const Texture *m_current;
	bool           m_keystate[2][4];
	bool           m_fullscreen;
};
