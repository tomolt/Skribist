
/*
C stdlib headers - dependency on these should be removed as soon as possible.
*/
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
C standard headers - we can use these even if we don't link with the standard library.
*/
#include <stdint.h>
#include <limits.h>
#include <emmintrin.h> // TODO MSVC
#include <smmintrin.h>

#include "Skribist.h"

#if 0
#define SKR_assert(stmt) assert(stmt)
#else
#define SKR_assert(stmt) do {} while (0)
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define gabs(x) ((x) >= 0 ? (x) : -(x))

typedef struct {
	float x, y;
} Point;

typedef struct {
	Point beg, end;
} Line;

typedef struct {
	Point beg, end, ctrl;
} Curve;

typedef struct {
	RasterCell * restrict raster;
	SKR_Dimensions dims;
	uint32_t rasterWidth;
} Workspace;

static int CompareStrings(char const * a, char const * b, long n)
{
	for (long i = 0; i < n; ++i) {
		if (a[i] != b[i])
			return a[i] - b[i];
	}
	return 0;
}

static Point Midpoint(Point a, Point b)
{
	float x = (a.x + b.x) / 2.0f; // TODO more bounded computation
	float y = (a.y + b.y) / 2.0f;
	return (Point) { x, y };
}

#include "Reading.c"
#include "Rasterizing.c"
#include "Tesselating.c"
#include "LoadingTTF.c"
