#pragma once
#include "gre.h"

class StringDrawable {
public:
	StringDrawable(const char *text, unsigned int fg, unsigned int bg);
	~StringDrawable();

	void setText(const char *text);

	const void *getData(void) const;
	const GRE::Dimensions &getDimensions(void) const;

private:
	GRE::Dimensions m_dims;
	unsigned int m_foreground;
	unsigned int m_background;
	unsigned int *m_data;
};
