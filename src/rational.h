/*

Copyright 2019 Thomas Oltmann

---- LICENSE ----

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifdef OLT_INTERN_RATIONAL_H
#error multiple inclusion
#endif
#define OLT_INTERN_RATIONAL_H

typedef struct {
	int numer;
	unsigned int denom;
} Rational;

Rational olt_INTERN_abs_rational(Rational rat);
Rational olt_INTERN_inv_rational(Rational rat);
Rational olt_INTERN_add_rational(Rational a, Rational b);
Rational olt_INTERN_sub_rational(Rational a, Rational b);
Rational olt_INTERN_mul_rational(Rational a, Rational b);
Rational olt_INTERN_rational_floor(Rational rat);
Rational olt_INTERN_rational_ceil(Rational rat);
Rational olt_INTERN_rational_round(Rational rat);
int olt_INTERN_rational_cmp(Rational a, Rational b);

#define R(n, d) ((Rational) { (n), (d) })

#define absr(x)    olt_INTERN_abs_rational(x)
#define invr(x)    olt_INTERN_inv_rational(x)
#define addr(a, b) olt_INTERN_add_rational((a), (b))
#define subr(a, b) olt_INTERN_sub_rational((a), (b))
#define mulr(a, b) olt_INTERN_mul_rational((a), (b))
#define floorr(x)  olt_INTERN_rational_floor(x)
#define ceilr(x)   olt_INTERN_rational_ceil(x)
#define roundr(x)  olt_INTERN_rational_round(x)
#define cmpr(a, b) olt_INTERN_rational_cmp((a), (b))
#define minr(a, b) ((cmpr((a), (b)) < 0) ? (a) : (b))
