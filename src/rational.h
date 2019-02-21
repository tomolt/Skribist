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
