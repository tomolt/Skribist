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

#include <stdint.h>

#include "outline.h"
#include "raster.h"

#include <stdlib.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define clamp(v, l, h) min(max((v), (l)), (h))

/*

olt_INTERN_gather() right now is mostly a stub. Further on in development it should
also do conversion to user-specified pixel formats, simple color fill,
gamma correction and sub-pixel rendering (if enabled).

*/

void olt_INTERN_gather(void)
{
#if 0
	int32_t acc = 0;
	for (int i = 0; i < WIDTH * HEIGHT; ++i) {
		acc += olt_GLOBAL_raster[i];
		olt_GLOBAL_image[i] = min(abs(acc), 255);
	}
#endif
#if 0
	for (int i = 0; i < WIDTH * HEIGHT; ++i) {
		olt_GLOBAL_image[i] = clamp(olt_GLOBAL_raster[i] / 2 + 127, 0, 255);
	}
#endif
	for (int r = 0; r < HEIGHT; ++r) {
		int32_t acc = 0;
		for (int c = 0; c < WIDTH; ++c) {
			int i = WIDTH * r + c;
			acc += olt_GLOBAL_raster[i];
			olt_GLOBAL_image[i] = min(abs(acc), 255);
		}
	}
}
