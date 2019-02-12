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

// depends on stdint.h

#ifdef OLT_INTERN_RASTER_H
#error multiple inclusion
#endif
#define OLT_INTERN_RASTER_H

#define WIDTH 256
#define HEIGHT 256

// Please update glossary when messing with units.
typedef struct {
	double x, y;
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

extern int16_t olt_GLOBAL_raster[WIDTH * HEIGHT];
extern uint8_t olt_GLOBAL_image[WIDTH * HEIGHT];

void olt_INTERN_raster_bezier(Bezier bezier);

void olt_INTERN_gather(void);
