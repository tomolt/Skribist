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
	int px;
	int py;
	int bx;
	int by;
	int ex;
	int ey;
} dot_t;

static int16_t accum[WIDTH * HEIGHT];
static uint8_t image[WIDTH * HEIGHT];

static point_t cns_point(double x, double y)
{
	return (point_t) { 0.5 + WIDTH / 2.0 + x * WIDTH, 0.5 + HEIGHT / 2.0 + y * HEIGHT };
}

static line_t cns_line(point_t beg, point_t end)
{
	return (line_t) { beg.x, beg.y, end.x - beg.x, end.y - beg.y };
}

static dot_t cns_dot(point_t beg, point_t end)
{
	int px = min(beg.x, end.x) + 0.001; // TODO cleanup
	int py = min(beg.y, end.y) + 0.001; // TODO cleanup
	int bx = round((beg.x - px) * 255.0);
	int by = round((beg.y - py) * 255.0);
	int ex = round((end.x - px) * 255.0);
	int ey = round((end.y - py) * 255.0);
	return (dot_t) { px, py, bx, by, ex, ey };
}

static void raster_dot(dot_t dot)
{
	int winding = sign(dot.ey - dot.by); // FIXME more robust way?
	int cover = abs(dot.ey - dot.by); // in the range 0 - 255
	int total = winding * cover;
	int width = 510 + abs(dot.ex - dot.bx) - 2 * max(dot.bx, dot.ex); // in the range 0 - 510
	int value = total * width / 510;
	accum[WIDTH * dot.py + dot.px] += value;
	accum[WIDTH * dot.py + dot.px + 1] += total - value;
}

/*

raster_line() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.

*/

static void raster_line(line_t line)
{
	double sx, sy; // step size along each axis
	double xt, yt; // t of next vertical / horizontal intersection

	if (line.dx != 0.0) {
		sx = fabs(1.0 / line.dx);
		if (line.dx > 0.0) {
			xt = sx * (ceil(line.ox) - line.ox);
		} else {
			xt = sx * (line.ox - floor(line.ox));
		}
	} else {
		sx = 0.0;
		xt = 9.9;
	}

	if (line.dy != 0.0) {
		sy = fabs(1.0 / line.dy);
		if (line.dy > 0.0) {
			yt = sy * (ceil(line.oy) - line.oy);
		} else {
			yt = sy * (line.oy - floor(line.oy));
		}
	} else {
		sy = 0.0;
		yt = 9.9;
	}

	double prev_t = 0.0;
	point_t prev_pt = { line.ox, line.oy };
	point_t pt = { line.ox, line.oy };

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

		double td = t - prev_t;
		pt.x += td * line.dx;
		pt.y += td * line.dy;

		raster_dot(cns_dot(prev_pt, pt));

		prev_t = t;
		prev_pt = pt;
	}

	point_t last_pt = { line.ox + line.dx, line.oy + line.dy };
	raster_dot(cns_dot(prev_pt, last_pt));
}

static void gather(void)
{
	int32_t acc = 0;
	for (int i = 0; i < WIDTH * HEIGHT; ++i) {
		acc += accum[i];
		image[i] = min(abs(acc), 255);
	}
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
	point_t pt1 = cns_point(-0.25, -0.25);
	point_t pt2 = cns_point( 0.25, -0.25);
	point_t pt3 = cns_point( 0.0,  0.25);
	raster_line(cns_line(pt1, pt2));
	raster_line(cns_line(pt2, pt3));
	raster_line(cns_line(pt3, pt1));

	point_t pt4 = cns_point(-0.125, -0.125);
	point_t pt5 = cns_point( 0.0,  0.125);
	point_t pt6 = cns_point( 0.125, -0.125);
	raster_line(cns_line(pt4, pt5));
	raster_line(cns_line(pt5, pt6));
	raster_line(cns_line(pt6, pt4));

	gather();
	write_bmp();
	return EXIT_SUCCESS;
}
