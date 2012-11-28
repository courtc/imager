#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imageloader.h"
#include "memorymapper.h"
#include "mime.h"

enum ImageFormat {
  ImageFormat_Invalid = 0,
  ImageFormat_PNG,
  ImageFormat_TGA,
  ImageFormat_JPEG,
  ImageFormat_ARGB8888,
};

extern "C" {
int LoadPNG(void *pRaw, int rawlen, unsigned int *puWidth, unsigned int *puHeight, void **ppData);
int LoadTGA(void *pRaw, int rawlen, unsigned int *puWidth, unsigned int *puHeight, void **ppData);
int LoadJPEG(void *pRaw, int rawlen, unsigned int *puWidth, unsigned int *puHeight, void **ppData);
}

Image *Image_CreateFromMemory(const void *pMem, ImageFormat fmt, unsigned int len)
{
	unsigned int uWidth, uHeight;
	void *pData;

	if (pMem == NULL)
		return NULL;

	switch (fmt) {
	case ImageFormat_PNG:
		if (LoadPNG((void *)pMem, len, &uWidth, &uHeight, &pData))
			return NULL;
		break;
	case ImageFormat_JPEG:
		if (LoadJPEG((void *)pMem, len, &uWidth, &uHeight, &pData))
			return NULL;
		break;
	case ImageFormat_TGA:
		if (LoadTGA((void *)pMem, len, &uWidth, &uHeight, &pData))
			return NULL;
		break;
	case ImageFormat_Invalid:
		if (!memcmp(pMem, "\x89PNG", 4))
			return Image_CreateFromMemory(pMem, ImageFormat_PNG, len);
		else if (!memcmp(pMem, "\xff\xd8", 2))
			return Image_CreateFromMemory(pMem, ImageFormat_JPEG, len);
		else
			return Image_CreateFromMemory(pMem, ImageFormat_TGA, len);
		break;
	default:
		return NULL;
		break;
	}

	return new Image(pData, GRE::Dimensions(uWidth, uHeight));
}

Image *ImageLoader::loadImage(const char *path)
{
	char mimetype[128];
	Image *pImage;

	if (!mime_file(path, mimetype, sizeof(mimetype))) {
		if (!strcmp(mimetype, "image/png"))
			;
		else if (!strcmp(mimetype, "image/jpeg"))
			;
		else
			return NULL;
	} else if (!strncmp(path, "http://", 7))
		;

	MemoryMapper::Map *map = MemoryMapper::map(path);
	if (map == NULL)
		return NULL;

	pImage = Image_CreateFromMemory(map->getData(), ImageFormat_Invalid, map->getLength());

	MemoryMapper::unmap(map);

	return pImage;
}

void ImageLoader::unloadImage(Image *image)
{
	free((void *)image->getData());
	delete image;
}
