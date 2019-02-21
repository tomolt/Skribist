#include "rational.h"

#include <stdlib.h>

#define sign(x) ((x) >= 0 ? 1 : -1)

Rational olt_INTERN_abs_rational(Rational rat)
{
	return R(abs(rat.numer), rat.denom);
}

Rational olt_INTERN_inv_rational(Rational rat)
{
	return R(rat.denom, rat.numer);
}

static unsigned int greatest_common_divisor(unsigned int a, unsigned int b)
{
	if (b == 0)
		return a;
	else
		return greatest_common_divisor(b, a % b);
}

static Rational reduce(Rational rat)
{
	int ratSign = sign(rat.numer);
	unsigned int div = greatest_common_divisor(abs(rat.numer), rat.denom);
	return R(rat.numer * ratSign / div, rat.denom / div);
}

Rational olt_INTERN_add_rational(Rational a, Rational b)
{
	int numer1 = a.numer * b.denom;
	int numer2 = b.numer * a.denom;
	unsigned int denom = a.denom * b.denom;
	return reduce(R(numer1 + numer2, denom));
}

Rational olt_INTERN_sub_rational(Rational a, Rational b)
{
	b.numer *= -1;
	return addr(a, b);
}

Rational olt_INTERN_mul_rational(Rational a, Rational b)
{
	Rational c = reduce(R(a.numer, b.denom));
	Rational d = reduce(R(b.numer, a.denom));
	int numer = c.numer * d.numer;
	unsigned int denom = c.denom * d.denom;
	return R(numer, denom);
}

Rational olt_INTERN_rational_floor(Rational rat)
{
	return R(rat.numer / rat.denom, 1);
}

Rational olt_INTERN_rational_ceil(Rational rat)
{
	return R((rat.numer + rat.denom - 1) / rat.denom, 1);
}

Rational olt_INTERN_rational_round(Rational rat)
{
	return floorr(addr(rat, R(1, 2)));
}

int olt_INTERN_rational_cmp(Rational a, Rational b)
{
	return a.numer * b.denom - b.numer * a.denom;
}
