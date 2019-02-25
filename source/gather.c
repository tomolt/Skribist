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

#include <stdio.h> // For debug only

void olt_INTERN_gather(void)
{
	for (int r = 0; r < HEIGHT; ++r) {
		long acc = 0; // in the range -127 - 127
		for (int c = 0; c < WIDTH; ++c) {
			int i = WIDTH * r + c;
			RasterCell *cell = &olt_GLOBAL_raster[i];
			int windingAndCover = cell->windingAndCover, area = cell->area;
			assert(windingAndCover >= -127 && windingAndCover <= 127);
			assert(area >= 0 && area <= 254);
			int value = acc + windingAndCover * area / 254; // in the range -127 - 127
			int scaledValue = value * 255 / 127; // in the range -255 - 255
			// TODO use standardized winding direction to obviate the need for this abs()
			olt_GLOBAL_image[i] = abs(scaledValue);
			acc += windingAndCover;
		}
		printf("%ld\n", acc);
	}
}
