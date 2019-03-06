
/*
C stdlib headers - dependency on these should be removed as soon as possible.
*/
#include <math.h>
#include <stdlib.h>
#include <assert.h>

/*
C standard headers - we can use these even if we don't link with the standard library.
*/
#include <stdint.h>

#include "reading.c" // FIXME
#include "Skribist.h"

static Point Midpoint(Point a, Point b)
{
	double x = (a.x + b.x) / 2.0; // TODO more bounded computation
	double y = (a.y + b.y) / 2.0;
	return (Point) { x, y };
}

#include "casting.c"
#include "header.c"
#include "parsing.c"
#include "rasterizing.c"
#include "tesselation.c"
