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
		long acc = 0; // in the range -127 - 127
		for (int c = 0; c < WIDTH; ++c) {
			int i = WIDTH * r + c;
			RasterCell *cell = &olt_GLOBAL_raster[i];
			int windingAndCover = cell->windingAndCover, area = cell->area;
			int value = acc + windingAndCover * area / 254; // in the range -127 - 127
			int normValue = value * 255 / 127; // TODO we lose a lot of precision here.
			olt_GLOBAL_image[i] = abs(normValue); // TODO use standardized winding direction to obviate the need for this abs()
			acc += windingAndCover;
		}
	}
}
