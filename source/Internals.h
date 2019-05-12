/*
C standard headers - we can use these even if we don't link with the standard library.
*/
#include <stdint.h>
#include <limits.h>
#include <immintrin.h> // TODO MSVC

#include "Skribist.h"

/*
So as it turns out, these first three naive macros are actually
faster than any bit-tricks or specialized functions on amd64.
*/

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define gabs(x)   ((x) >= 0 ? (x) : -(x))
#define floorf(x) __builtin_floorf(x)
#define ceilf(x)  __builtin_ceilf(x)

#define GRAIN_BITS 8
#define GRAIN (1 << GRAIN_BITS)

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

char * FormatUint(unsigned int n, char buf[8]);
size_t LengthOfString(char const * str);
int CompareStrings(char const * a, char const * b, long n);
Point Midpoint(Point a, Point b);

__attribute__((noreturn))
void SKR_assert_fail(char const * expr, char const * file,
	unsigned int line, char const * function);

#define SKR_assert(expr) \
	((void) ((expr) ? 0 : (SKR_assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__), 0)))

typedef uint8_t  const BYTES1;
typedef uint16_t const BYTES2;
typedef uint32_t const BYTES4;
typedef uint64_t const BYTES8;

static inline uint16_t ru16(BYTES2 raw)
{
	BYTES1 *bytes = (BYTES1 *) &raw;
	uint16_t b0 = bytes[1], b1 = bytes[0];
	return b0 | b1 << 8;
}

static inline int16_t ri16(BYTES2 raw)
{
	uint16_t u = ru16(raw);
	return *(int16_t *) &u;
}

static inline uint32_t ru32(BYTES4 raw)
{
	BYTES1 *bytes = (BYTES1 *) &raw;
	uint32_t b0 = bytes[3], b1 = bytes[2];
	uint32_t b2 = bytes[1], b3 = bytes[0];
	return b0 | b1 << 8 | b2 << 16 | b3 << 24;
}
