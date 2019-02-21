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
