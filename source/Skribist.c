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

static char * FormatUint(unsigned int n, char buf[8])
{
	buf[7] = '\0';
	char * ptr = buf + 7;
	while (n > 0) {
		*--ptr = (n % 10) + '0';
		n /= 10;
	}
	return ptr;
}

static size_t LengthOfString(char const * str)
{
	size_t i = 0;
	while (str[i] != '\0') ++i;
	return i;
}

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

/* from Platform.S */
extern ssize_t SKR_assert_write(int fd, void const * buf, size_t count);
extern __attribute__((noreturn)) void SKR_assert_abort(void);

static inline void SKR_assert_print(char const * message)
{
	size_t length = LengthOfString(message);
	SKR_assert_write(2, message, length);
}

static inline __attribute__((noreturn))
void SKR_assert_fail(char const * expr, char const * file,
	unsigned int line, char const * function)
{
	(void) file;
	char buf[8];
	char * lineNo = FormatUint(line, buf);
	SKR_assert_print(file);
	SKR_assert_print(": ");
	SKR_assert_print(lineNo);
	SKR_assert_print(": ");
	SKR_assert_print(function);
	SKR_assert_print("(): Assertion \"");
	SKR_assert_print(expr);
	SKR_assert_print("\" failed.\n");
	SKR_assert_abort();
}

#define SKR_assert(expr) \
	((void) ((expr) ? 0 : (SKR_assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__), 0)))

#include "Reading.c"
#include "Exporting.c"
#include "Rasterizing.c"
#include "Tesselating.c"
#include "Outline.c"
#include "Assembly.c"
#include "Preparing.c"
