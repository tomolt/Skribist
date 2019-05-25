#include "Internals.h"

char * FormatUint(unsigned int n, char buf[8])
{
	buf[7] = '\0';
	char * ptr = buf + 7;
	while (n > 0) {
		*--ptr = (n % 10) + '0';
		n /= 10;
	}
	return ptr;
}

size_t LengthOfString(char const * str)
{
	size_t i = 0;
	while (str[i] != '\0') ++i;
	return i;
}

int CompareStrings(char const * a, char const * b, long n)
{
	for (long i = 0; i < n; ++i) {
		if (a[i] != b[i])
			return a[i] - b[i];
	}
	return 0;
}

Point Midpoint(Point a, Point b)
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

__attribute__((noreturn))
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

static float slow_logf(float x)
{
	SKR_assert(x > 0.0f);
	bool p = true;
	if (x < 1.0f) {
		p = false;
		x = 1.0f / x;
	}
	x -= 1.0f;
	float r = 0.0f, c = -1.0f;
	for (int i = 1; i < 16; ++i) {
		c *= -x;
		r += c / i;
	}
	return p ? r : -r;
}

static float slow_expf(float x)
{
	bool p = true;
	if (x < 0.0f) {
		p = false;
		x = -x;
	}
	float r = 1.0f, c = 1.0f; 
	for (int i = 1; i < 16; ++i) {
		c *= x / i;
		r += c;
	}
	return p ? r : (1.0f / r);
}

float SKR_slow_powf(float base, float exponent)
{
	SKR_assert(base >= 0.0f);
	if (base == 0.0f) {
		return 0.0f;
	}
	return slow_expf(exponent * slow_logf(base));
}

/* Device */

static float LinearToGamma(float linear, float gammaValue)
{
	float corrected;
	if (linear <= 0.0031308f) {
		corrected = linear * 12.92f;
	} else {
		corrected = 1.055f * SKR_slow_powf(linear, 1.0f / 2.2f) - 0.055f;
	}
	return corrected;
}

void skrBuildDevice(SKR_Device * restrict device)
{
	for (int i = 0; i < SKR_GAMMA_TABLE_LENGTH; ++i) {
		float ratio = i / (float) (SKR_GAMMA_TABLE_LENGTH - 1);
		device->gammaTable[i] = round(LinearToGamma(i, device->gammaValue) * 255.0);
	}
}

