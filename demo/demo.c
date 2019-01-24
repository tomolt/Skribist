/*

---- GLOSSARY ----

I am of firm belief that adequate imaginative nomenclature is the single
most important key to the expressing and understanding of complex concepts.

"bezier": a quadratic bezier curve.
"line": a simple, straight line.
"dot": a short line that only spans one pixel (think 'dotted line').
"point": a point in 2d space, where one unit equals one pixel.
"cover": the height that a particular dot span within one pixel.
this term was inherited from Anti-Grain Geometry and cl-vectors.
"area": the signed area between a line and the left edge of the raster.
this term was inherited from Anti-Grain Geometry and cl-vectors.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define sign(x) ((x) >= 0.0 ? 1.0 : -1.0)

#define WIDTH 256
#define HEIGHT 256

// Please update glossary when messing with units.
typedef struct {
	double x, y;
} point_t;

typedef struct {
	double ox; // x of origin point
	double oy; // y of origin point
	double dx; // difference along x
	double dy; // difference along y
} line_t;

typedef struct {
	int px, py;
	point_t pts[2];
} dot_t;

static int16_t accum[WIDTH * HEIGHT];
static uint8_t image[WIDTH * HEIGHT];

static void raster_dot(point_t beg, point_t end)
{
	// int winding = ? 1 : -1;
	// accum[WIDTH * py + px] = winding * 255;
	printf("(%f, %f) -> (%f, %f)\n", beg.x, beg.y, end.x, end.y);
}

/*

raster_line() is intended to take in a single line and output a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.

*/

static void raster_line(line_t line)
{
	assert(line.dx != 0.0);
	assert(line.dy != 0.0);

	double prev_t = 0.0;
	point_t prev_pt = { line.ox, line.oy };

	double sx = fabs(1.0 / line.dx); // step size along x
	double sy = fabs(1.0 / line.dy); // step size along y
	double xt = sx * (line.dx >= 0.0 ?
		ceil(line.ox) - line.ox : line.ox - floor(line.ox)); // t of next vertical intersection
	double yt = sy * (line.dy >= 0.0 ?
		ceil(line.oy) - line.oy : line.oy - floor(line.oy)); // t of next horizontal intersection

	while (xt <= 1.0 || yt <= 1.0) {
		double t;
		if (xt < yt) {
			t = xt;
			xt += sx;
		} else {
			t = yt;
			yt += sy;
		}
		if (t == prev_t) continue;
		point_t pt = { line.ox + t * line.dx, line.oy + t * line.dy };
		raster_dot(prev_pt, pt);
		prev_t = t;
		prev_pt = pt;
	}

	point_t last_pt = { line.ox + line.dx, line.oy + line.dy };
	raster_dot(prev_pt, last_pt);
}

static void fmt_le_dword(char *buf, uint32_t v)
{
	buf[0] = v & 0xFF;
	buf[1] = v >> 8 & 0xFF;
	buf[2] = v >> 16 & 0xFF;
	buf[3] = v >> 24 & 0xFF;
}

static void write_bmp(void)
{
	char hdr[54] = { 0 };
	// Header
	hdr[0] = 'B';
	hdr[1] = 'M';
	fmt_le_dword(&hdr[2], 54 + 3 * WIDTH * HEIGHT); // size of file
	hdr[10] = 54; // offset to image data
	// InfoHeader
	hdr[14] = 40; // size of InfoHeader
	fmt_le_dword(&hdr[18], WIDTH);
	fmt_le_dword(&hdr[22], HEIGHT);
	hdr[26] = 1; // color planes
	hdr[28] = 24; // bpp
	fwrite(hdr, 1, 54, stdout);
	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) {
			unsigned char c = image[WIDTH * y + x];
			fputc(c, stdout); // r
			fputc(c, stdout); // g
			fputc(c, stdout); // b
		}
	}
}

int main(int argc, char const *argv[])
{
	line_t line = { 0.0, HEIGHT - 1, 177.2, -99.8 };
	raster_line(line);
	// write_bmp();
	return EXIT_SUCCESS;
}
