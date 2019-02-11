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

#include <stdint.h>

#include "raster.h"
#include "gather.h"

#include <stdio.h>
#include <stdlib.h>

static Point cns_point(double x, double y)
{
	return (Point) { 0.5 + WIDTH / 2.0 + x * WIDTH, 0.5 + HEIGHT / 2.0 + y * HEIGHT };
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

	Point beg1  = cns_point(-0.25, 0.0);
	Point ctrl1 = cns_point(0.0, 0.5);
	Point end1  = cns_point(0.25, 0.0);
	Bezier bez1 = { beg1, ctrl1, end1 };
	olt_INTERN_raster_bezier(bez1);

	Point beg2  = cns_point(0.25, 0.0);
	Point ctrl2 = cns_point(0.0, -0.5);
	Point end2  = cns_point(-0.25, 0.0);
	Bezier bez2 = { beg2, ctrl2, end2 };
	olt_INTERN_raster_bezier(bez2);

	olt_INTERN_gather();
	write_bmp();
	return EXIT_SUCCESS;
}
