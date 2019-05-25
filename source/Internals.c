#include "Internals.h"

#include <sys/types.h>

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

unsigned long LengthOfString(char const * str)
{
	unsigned long i = 0;
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

