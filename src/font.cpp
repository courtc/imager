#include <string.h>

#include "font.h"

static const unsigned int font_characters[] = {
	0x00000000,  // SPACE
	0x44444044,  // !
	0x55000000,  // "
	0x055f5faa,  // # ... or some resemblance
	0x227263e2,  // $
	0x00992499,  // %
	0x04aa4ba7,  // &
	0x44800000,  // '
	0x12444421,  // (
	0x84222248,  // )
	0x096f6900,  // *
	0x0004e400,  // +
	0x00000224,  // ,
	0x0000f000,  // -
	0x00000022,  // .
	0x11224488,  // /
	0x699bd996,  // 0
	0x26222222,  // 1
	0x6912488f,  // 2
	0x69161196,  // 3
	0x9999f111,  // 4
	0xf88e1196,  // 5
	0x688e9996,  // 6
	0xf1172222,  // 7
	0x69969996,  // 8
	0x69997111,  // 9
	0x00044044,  // :
	0x00044048,  // ;
	0x01248421,  // <
	0x000f0f00,  // =
	0x08421248,  // >
	0x0e116404,  // ?
	0x699dd196,  // @
	0x00617997,  // a
	0x88e9999e,  // b
	0x00788887,  // c
	0x11799997,  // d
	0x00699f87,  // e
	0x344e4444,  // f
	0x0799971e,  // g
	0x888e9999,  // h
	0x22022222,  // i
	0x2202222c,  // j
	0x889aca99,  // k
	0x62222227,  // l
	0x009fb999,  // m
	0x00e99999,  // n
	0x00699996,  // o
	0x0e999e88,  // p
	0x07999711,  // q
	0x00bc8888,  // r
	0x0078611e,  // s
	0x444e4442,  // t
	0x00999996,  // u
	0x00999ac8,  // v
	0x00999bf9,  // w
	0x00996999,  // x
	0x0999971e,  // y
	0x00f1248f,  // z
	0xf888888f,  // [
	0x88442211,  // \_
	0xf111111f,  // ]
	0x4a000000,  // ^
	0x0000000f,  // _
	0x08840000,  // `
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,
	0x34488443,  // {
	0x22222222,  // |
	0xc221122c,  // }
	0x5a000000   // ~
};

#define CHAR_WIDTH   4
#define CHAR_HEIGHT  8
#define CHAR_PADDING 1
#define CHAR(x) font_characters[(x > 96 && x < 123) ? x - 64 : x - 32]

#define SWAP(x,y) do { (x) ^= (y); (y) ^= (x); (x) ^= (y); } while (0)
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

static void pix_fillrect(unsigned int *m_pixels, const GRE::Dimensions &dims,
		int x1, int y1, int x2, int y2, unsigned int col)
{
	int xi;
	unsigned int *p;

	if (x1 == x2 || y1 == y2) return;
	if (x1 > x2) SWAP(x1, x2);
	if (y1 > y2) SWAP(y1, y2);

	y2 = MIN(dims.h, y2);
	x2 = MIN(dims.w, x2);

	x1 = MAX(0, x1);
	y1 = MAX(0, y1);

	if (x2 <= x1 || y2 <= y1) return;

	for (; y1 < y2; ++y1) {
		p = m_pixels + dims.w * y1;
		for (xi = x1; xi < x2; ++xi)
			*(p + xi) = col;
	}
}

void pix_drawchar(unsigned int *m_pixels, const GRE::Dimensions &dims,
		int x, int y, char ch, unsigned int col)
{
	unsigned int v = CHAR(ch);
	int p = 32, xi = 0;

	while (p--) {
		if (v & (1 << p))
			*(m_pixels + y * dims.w + x + xi) = col;
		if (++xi == CHAR_WIDTH) ++y, xi = 0;
	}
}


StringDrawable::StringDrawable(const char *text, unsigned int fg, unsigned int bg)
 : m_dims(0,0), m_foreground(fg), m_background(bg), m_data(NULL)
{
	setText(text);

}

StringDrawable::~StringDrawable()
{
	delete[] m_data;
}

void StringDrawable::setText(const char *text)
{
	GRE::Dimensions dims(0,0);
	int len = strlen(text);
	int nlines = 1;
	const char *p;
	int x, y;

	p = text;
	while ((p = strchr(p, '\n')) != NULL) {
		nlines++;
		p = p + 1;
	}

	dims.w = len * (CHAR_WIDTH + CHAR_PADDING) + 10;
	dims.h = nlines * (CHAR_HEIGHT + CHAR_PADDING) + 10;

	if (dims.w != m_dims.w || dims.h != m_dims.h) {
		m_dims = dims;
		if (m_data != NULL) {
			delete[] m_data;
		}
		m_data = new unsigned int[m_dims.w * m_dims.h];
	}

	pix_fillrect(m_data, m_dims, 0, 0, m_dims.w, m_dims.h, m_background);
	x = y = 5;
	for (p = text; *p; ++p) {
		if (*p == '\n') {
			y += (CHAR_HEIGHT + CHAR_PADDING);
			x = 5;
		} else {
			pix_drawchar(m_data, m_dims, x + 1, y + 1, *p, 0x80000000);
			pix_drawchar(m_data, m_dims, x, y, *p, m_foreground);
			x += (CHAR_WIDTH + CHAR_PADDING);
		}
	}
}

const void *StringDrawable::getData(void) const
{
	return (void *)m_data;
}

const GRE::Dimensions &StringDrawable::getDimensions(void) const
{
	return m_dims;
}
