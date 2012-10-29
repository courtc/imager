#pragma once

#include "gre.h"

class Image {
public:
	Image(const void *data, const GRE::Dimensions &dims)
	 : m_data(data), m_dims(dims)
	{ }
	~Image()
	{ }

	const void *getData(void) const
	{
		return m_data;
	}

	const GRE::Dimensions &getDimensions(void) const
	{
		return m_dims;
	}

private:
	const void *m_data;
	GRE::Dimensions m_dims;
};

class ImageLoader {
public:
	static Image *loadImage(const char *path);
	static void unloadImage(Image *);
};
