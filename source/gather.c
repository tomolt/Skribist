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
	for (int r = 0; r < HEIGHT; ++r) {
		int32_t acc = 0;
		for (int c = 0; c < WIDTH; ++c) {
			int i = WIDTH * r + c;
			RasterCell *cell = &olt_GLOBAL_raster[i];
			int32_t value = cell->windingAndCover * cell->area;
			acc += value;
			olt_GLOBAL_image[i] = min(abs(acc), 255);
			acc += cell->windingAndCover - value;
		}
	}
}
