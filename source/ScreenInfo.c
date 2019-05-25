#include "Internals.h"

static float SKR_logf(float x)
{
	float const E = 2.718281828459f;

	SKR_assert(x > 0.0f);
	SKR_assert(x <= 1.0f);

	float r = 0.0f;
	while (x < 0.5f) {
		x *= E;
		--r;
	}

	x -= 1.0f;
	float c = -1.0f;
	for (int i = 1; i < 8; ++i) {
		c *= -x;
		r += c / i;
	}

	return r;
}

static float SKR_expf(float x)
{
	SKR_assert(x <= 0.0f);

	x = -x;
	float r = 1.0f, c = 1.0f; 
	for (int i = 1; i < 8; ++i) {
		c *= x / i;
		r += c;
	}

	return 1.0f / r;
}

static float SKR_powf(float base, float exponent)
{
	SKR_assert(base >= 0.0f);
	if (base == 0.0f) {
		return 0.0f;
	}
	return SKR_expf(exponent * SKR_logf(base));
}

static float LinearToGamma(float linear, float gammaValue)
{
	float corrected;
	if (linear <= 0.0031308f) {
		corrected = linear * 12.92f;
	} else {
		corrected = 1.055f * SKR_powf(linear, 1.0f / gammaValue) - 0.055f;
	}
	return corrected;
}

void skrBuildScreenInfo(SKR_ScreenInfo * restrict screenInfo)
{
	for (int i = 0; i < SKR_GAMMA_TABLE_LENGTH; ++i) {
		float ratio = i / (float) (SKR_GAMMA_TABLE_LENGTH - 1);
		float gamma = LinearToGamma(ratio, screenInfo->gammaValue);
		screenInfo->gammaTable[i] = roundf(gamma * 255.0);
	}
}

