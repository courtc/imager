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
			FilterToggle,
			TextToggle,
		};

		Type type;
	};

	struct Dimensions {
		Dimensions(int _w, int _h)
		 : w(_w), h(_h)
		{ }
		int w, h;
	};

	struct Position {
		Position(int _x, int _y)
		 : x(_x), y(_y)
		{ }
		int x, y;
	};

	class Texture {
	protected:
		Texture()
		 : m_alpha(1.0f), m_rotation(0.0f), m_position(0,0)
		{ }

		float m_alpha;
		float m_rotation;
		Position m_position;
	public:
		virtual void update(const void *pData, const GRE::Dimensions &) = 0;
		void setPosition(const Position &pos)
		{ m_position = pos; };
		const Position &getPosition(void) const
		{ return m_position; };
		void setRotation(float v)
		{ m_rotation = v; }
		float getRotation(void) const
		{ return m_rotation; }

		void setAlpha(float v)
		{ m_alpha = v; }
		float getAlpha(void) const
		{ return m_alpha; }
		virtual ~Texture() { }
	};

	GRE(const Dimensions &dims, bool fullscreen);
	~GRE();

	void setVideoMode(const Dimensions &dims, bool fullscreen);

	Texture *loadTexture(const void *pData, const Dimensions &);
	void unloadTexture(Texture *texture);

	Texture *getSpinnerTexture(void);
	void enableFiltering(bool v);

	void clearTexturePasses(void);
	void addTexturePass(const Texture *texture);
	void remTexturePass(const Texture *texture);

	void clearTextureOverlays(void);
	void addTextureOverlay(const Texture *texture);
	void remTextureOverlay(const Texture *texture);

	void render(void);

	int pollEvent(Event &ev);
private:

	void renderTexture(const Texture *texture);
	void renderTextureOverlay(const Texture *texture);

	std::list<const Texture *> m_render;
	std::list<const Texture *> m_overlay;
	Texture       *m_spinner;
	Dimensions     m_dims;
	bool           m_keystate[2][4];
	bool           m_fullscreen;
	bool           m_filtering;
	void          *m_priv;
};
