/*

Copyright 2019 Thomas Oltmann

---- LICENSE ----

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/*

---- REFERENCES ----

Sean Barrett, "How the stb_truetype Anti-Aliased Software Rasterizer v2 Works",
	https://nothings.org/gamedev/rasterize/
CL-VECTORS documentation, "9. The cl-aa algorithm",
	http://projects.tuxee.net/cl-vectors/section-the-cl-aa-algorithm
Ralph Levien, "Inside the fastest font renderer in the world",
	https://medium.com/@raphlinus/inside-the-fastest-font-renderer-in-the-world-75ae5270c445
Maxim Shemanarev, "Anti-Grain Geometry - Interpolation with Bezier Curves",
	http://www.antigrain.com/research/bezier_interpolation/
Maxim Shemanarev, "Anti-Grain Geometry - Gamma Correction",
	http://www.antigrain.com/research/gamma_correction/
Maxim Shemanarev, "Anti-Grain Geometry - Adaptive Subdivision of Bezier Curves",
	http://www.antigrain.com/research/adaptive_bezier/
Maxim Shemanarev, "Anti-Grain Geometry - Texts Rasterization Exposures",
	http://www.antigrain.com/research/font_rasterization/

*/

/*

---- GLOSSARY ----

I am of firm belief that adequate imaginative nomenclature is the single
most important key to expressing and understanding complex concepts.

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

#include "rational.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define sign(x) ((x) >= 0.0 ? 1.0 : -1.0)

#define WIDTH 256
#define HEIGHT 256

// Please update glossary when messing with units.
typedef struct {
	Rational x, y;
} Point;

typedef struct {
	Point beg;
	Point diff;
} Line;

typedef struct {
	int px;
	int py;
	int bx;
	int by;
	int ex;
	int ey;
} Dot;

typedef struct {
	Point beg;
	Point ctrl;
	Point end;
} Bezier;

static int16_t olt_GLOBAL_raster[WIDTH * HEIGHT];
static uint8_t olt_GLOBAL_image[WIDTH * HEIGHT];

static Rational cns_coord(Rational v, int span)
{
	Rational const one = R(1, 1), half = R(1, 2);
	return mulr(half, addr(one, mulr( addr(one, v), R(span, 1))));
}

static Point cns_point(Rational x, Rational y)
{
	return (Point) { cns_coord(x, WIDTH), cns_coord(y, HEIGHT) };
}

static Point add_point(Point a, Point b)
{
	return (Point) { addr(a.x, b.x), addr(a.y, b.y) };
}

static Point sub_point(Point a, Point b)
{
	return (Point) { subr(a.x, b.x), subr(a.y, b.y) };
}

static Line cns_line(Point beg, Point end)
{
	return (Line) { beg, sub_point(end, beg) };
}

static Dot cns_dot(Point beg, Point end)
{
	// TODO cleanup
	Rational const hacky_epsilon = R(1, 1000);
	int px = min(
		floorr(addr(beg.x, hacky_epsilon)),
		floorr(addr(end.x, hacky_epsilon)));
	int py = min(
		floorr(addr(beg.y, hacky_epsilon)),
		floorr(addr(end.y, hacky_epsilon)));
	int bx = roundr(mulr(subr(beg.x, R(px, 1)), R(255, 1)));
	int by = roundr(mulr(subr(beg.y, R(py, 1)), R(255, 1)));
	int ex = roundr(mulr(subr(end.x, R(px, 1)), R(255, 1)));
	int ey = roundr(mulr(subr(end.y, R(py, 1)), R(255, 1)));
	return (Dot) { px, py, bx, by, ex, ey };
}

static void raster_dot(Dot dot)
{
	int winding = sign(dot.ey - dot.by); // FIXME more robust way?
	int cover = abs(dot.ey - dot.by); // in the range 0 - 255
	int total = winding * cover;
	int width = 510 + abs(dot.ex - dot.bx) - 2 * max(dot.bx, dot.ex); // in the range 0 - 510
	int value = total * width / 510;
	olt_GLOBAL_raster[WIDTH * dot.py + dot.px] += value;
	olt_GLOBAL_raster[WIDTH * dot.py + dot.px + 1] += total - value;
}

/*

raster_line() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.

*/

/*

At some point, raster_line() needs to be redone - there are *many* ways in which
it could be drastically simplified still. Most importantly, it should track the
integer pixel coordinates by itself without having to rely on the hacky cns_dot() thing.

*/

static void raster_line(Line line)
{
	Rational sx, sy; // step size along each axis
	Rational xt, yt; // t of next vertical / horizontal intersection

	if (line.diff.x.numer != 0) {
		sx = absr(invr(line.diff.x));
		if (line.diff.x.numer > 0) {
			xt = mulr(sx, subr(ceilr(line.beg.x), line.beg.x));
		} else {
			xt = mulr(sx, subr(line.beg.x, floorr(line.beg.x)));
		}
	} else {
		sx = R(0, 1);
		xt = R(2, 1); // Any number larger than 1 works
	}

	if (line.diff.y.numer != 0) {
		sy = absr(invr(line.diff.y));
		if (line.diff.y.numer > 0) {
			yt = sy * (ceil(line.beg.y) - line.beg.y);
		} else {
			yt = sy * (line.beg.y - floor(line.beg.y));
		}
	} else {
		sy = R(0, 1);
		yt = R(2, 1); // Any number larger than 1 works
	}

	Rational prev_t = R(0, 1);
	Point prev_pt = line.beg, pt = prev_pt;

	while (xt <= 1.0 || yt <= 1.0) {
		Rational t;

		if (xt < yt) {
			t = xt;
			xt += sx;
		} else {
			t = yt;
			yt += sy;
		}

		if (t.numer == prev_t.numer && t.denom == prev_t.denom) continue;

		Rational td = t - prev_t;
		pt.x = addr(pt.x, mulr(td, line.diff.x));
		pt.y = addr(pt.y, mulr(td, line.diff.y));

		raster_dot(cns_dot(prev_pt, pt));

		prev_t = t;
		prev_pt = pt;
	}

	Point last_pt = { line.beg.x + line.diff.x, line.beg.y + line.diff.y };
	raster_dot(cns_dot(prev_pt, last_pt));
}

/*

Once the points of bezier curves are represented in relative coordinates
to each other, interp_points() and manhattan_distance() should just operate
on lines for simplicity.

*/

static Point interp_bezier(Bezier bezier)
{
	double ax = bezier.beg.x - 2.0 * bezier.ctrl.x + bezier.end.x;
	double ay = bezier.beg.y - 2.0 * bezier.ctrl.y + bezier.end.y;
	double bx = 2.0 * (bezier.ctrl.x - bezier.beg.x);
	double by = 2.0 * (bezier.ctrl.y - bezier.beg.y);
	double x = (ax / 2.0 + bx) / 2.0 + bezier.beg.x;
	double y = (ay / 2.0 + by) / 2.0 + bezier.beg.y;
	return (Point) { x, y };
}

static Point interp_points(Point a, Point b)
{
	double x = (a.x + b.x) / 2.0; // TODO more bounded computation
	double y = (a.y + b.y) / 2.0;
	return (Point) { x, y };
}

static double manhattan_distance(Point a, Point b)
{
	return fabs(a.x - b.x) + fabs(a.y - b.y);
}

static int is_flat(Bezier bezier)
{
	Point mid = interp_points(bezier.beg, bezier.end);
	double dist = manhattan_distance(bezier.ctrl, mid);
	return dist <= 0.5;
}

static void split_bezier(Bezier bezier, Bezier segments[2])
{
	Point pivot = interp_bezier(bezier);
	Point ctrl0 = interp_points(bezier.beg, bezier.ctrl);
	Point ctrl1 = interp_points(bezier.ctrl, bezier.end);
	segments[0] = (Bezier) { bezier.beg, ctrl0, pivot };
	segments[1] = (Bezier) { pivot, ctrl1, bezier.end };
}

static void raster_bezier(Bezier bezier)
{
	if (is_flat(bezier)) {
		raster_line(cns_line(bezier.beg, bezier.end));
	} else {
		Bezier segments[2];
		split_bezier(bezier, segments);
		raster_bezier(segments[0]); // TODO don't overflow the stack
		raster_bezier(segments[1]); // in pathological cases.
	}
}

/*

gather() right now is mostly a stub. Further on in development it should
also do conversion to user-specified pixel formats, simple color fill,
gamma correction and sub-pixel rendering (if enabled).

*/

static void gather(void)
{
	int32_t acc = 0;
	for (int i = 0; i < WIDTH * HEIGHT; ++i) {
		acc += olt_GLOBAL_raster[i];
		olt_GLOBAL_image[i] = min(abs(acc), 255);
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
			unsigned char c = olt_GLOBAL_image[WIDTH * y + x];
			fputc(c, stdout); // r
			fputc(c, stdout); // g
			fputc(c, stdout); // b
		}
	}
}

int main(int argc, char const *argv[])
{
	(void) argc, (void) argv;

	Point beg1  = cns_point(R(-1, 4), R(0, 1));
	Point ctrl1 = cns_point(R(0, 1), R(1, 2));
	Point end1  = cns_point(R(1, 4), R(0, 1));
	Bezier bez1 = { beg1, ctrl1, end1 };
	raster_bezier(bez1);

	Point beg2  = cns_point(R(1, 4), R(0, 1));
	Point ctrl2 = cns_point(R(0, 1), R(-1, 2));
	Point end2  = cns_point(R(-1, 4), R(0, 1));
	Bezier bez2 = { beg2, ctrl2, end2 };
	raster_bezier(bez2);

	gather();
	write_bmp();
	return EXIT_SUCCESS;
}
