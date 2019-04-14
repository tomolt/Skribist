
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

#include "Skribist.h"

#define SKR_assert(stmt) assert(stmt)

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct {
	double x, y;
} Point;

typedef struct {
	Point beg, end;
} Line;

typedef struct {
	Point beg, ctrl, end;
} Curve;

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
	double x = (a.x + b.x) / 2.0; // TODO more bounded computation
	double y = (a.y + b.y) / 2.0;
	return (Point) { x, y };
}

#include "Reading.c"
#include "Rasterizing.c"
#include "Tesselating.c"
#include "LoadingTTF.c"
