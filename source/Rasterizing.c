static void RasterizeDot(
	long qbx, long qby, long qex, long qey,
	RasterCell * restrict dest, SKR_Dimensions dims)
{
	// pixel coordinates
	long px = min(qbx, qex) / 1024;
	long py = min(qby, qey) / 1024;
	// fractional beg & end coordinates
	long fbx = qbx - px * 1024;
	long fby = qby - py * 1024;
	long fex = qex - px * 1024;
	long fey = qey - py * 1024;

	SKR_assert(px >= 0 && px < dims.width);
	SKR_assert(py >= 0 && py < dims.height);

	long tailValue = fey - fby; // winding * cover
	long area = labs(fex - fbx) / 2 + 1024 - max(fex, fbx);
	long edgeValue = tailValue * area / 1024;

	long cellIdx = dims.width * (py / 8) + px;

	dest[cellIdx].tailValues[py % 8] += tailValue;
	dest[cellIdx].edgeValues[py % 8] += edgeValue;
}

#define QUANTIZE(x) ((long) ((x) * 1024.0 + 0.5))

/*
RasterizeLine() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.
*/

static void RasterizeLine(Line line, RasterCell * restrict dest, SKR_Dimensions dims)
{
	Point diff = { line.end.x - line.beg.x, line.end.y - line.beg.y };

	double sx, sy; // step size along each axis
	double xt, yt; // t of next vertical / horizontal intersection

	if (diff.x != 0.0) {
		sx = fabs(1.0 / diff.x);
		if (diff.x > 0.0) {
			xt = sx * (ceil(line.beg.x) - line.beg.x);
		} else {
			xt = sx * (line.beg.x - floor(line.beg.x));
		}
	} else {
		sx = 0.0;
		xt = 9.9;
	}

	if (diff.y != 0.0) {
		sy = fabs(1.0 / diff.y);
		if (diff.y > 0.0) {
			yt = sy * (ceil(line.beg.y) - line.beg.y);
		} else {
			yt = sy * (line.beg.y - floor(line.beg.y));
		}
	} else {
		sy = 0.0;
		yt = 9.9;
	}

	double prev_t = 0.0;
	long prev_qx = QUANTIZE(line.beg.x);
	long prev_qy = QUANTIZE(line.beg.y);

	while (xt < 1.0 || yt < 1.0) {
		double t;

		if (xt < yt) {
			t = xt;
			xt += sx;
		} else {
			t = yt;
			yt += sy;
		}

		if (t == prev_t) continue;

		long qx = QUANTIZE(line.beg.x + t * diff.x);
		long qy = QUANTIZE(line.beg.y + t * diff.y);

		RasterizeDot(prev_qx, prev_qy, qx, qy, dest, dims);

		prev_t = t;
		prev_qx = qx;
		prev_qy = qy;
	}

	long qx = QUANTIZE(line.end.x);
	long qy = QUANTIZE(line.end.y);

	RasterizeDot(prev_qx, prev_qy, qx, qy, dest, dims);
}

static void DrawLine(Line line, RasterCell * restrict dest, SKR_Dimensions dims)
{
	RasterizeLine(line, dest, dims);
}

// TODO take monitor gamma i guess?
static double CalcLinearToGamma(double L)
{
	double S;
	if (L <= 0.0031308) {
		S = L * 12.92;
	} else {
		S = 1.055 * pow(L, 1.0 / 2.2) - 0.055;
	}
	return S;
}

static unsigned char LinearToGamma[1025];

void skrInitializeLibrary(void)
{
	for (int i = 0; i <= 1024; ++i) {
		LinearToGamma[i] = round(CalcLinearToGamma(i / 1024.0) * 255.0);
	}
}

unsigned long skrCalcCellCount(SKR_Dimensions dims)
{
	return (dims.height + 7) / 8 * dims.width;
}

void skrCastImage(
	RasterCell const * restrict source,
	unsigned char * restrict dest,
	SKR_Dimensions dims)
{
	for (long row = 0; row < (dims.height + 7) / 8; ++row) {

		__m128i accumulators = _mm_setzero_si128();

		for (long col = 0; col < dims.width; ++col) {

			long cellIdx = row * dims.width + col;
			__m128i edgeValues = _mm_loadu_si128(
				(__m128i const *) source[cellIdx].edgeValues);
			__m128i tailValues = _mm_loadu_si128(
				(__m128i const *) source[cellIdx].tailValues);

			__m128i values = _mm_adds_epi16(accumulators, edgeValues);
			__m128i linearValues = _mm_min_epi16(_mm_max_epi16(values,
				_mm_setzero_si128()), _mm_set1_epi16(1024));
			// TODO gamma correction
			__m128i gammaValues = _mm_srai_epi16(linearValues, 2);
			gammaValues = _mm_min_epi16(gammaValues, _mm_set1_epi16(255));

			accumulators = _mm_adds_epi16(accumulators, tailValues);

			short pixels[8];
			_mm_storeu_si128((__m128i *) pixels, gammaValues);
			for (int q = 0; q < 8; ++q) {
				if (row * 8 + q < dims.height) {
					dest[(row * 8 + q) * dims.width + col] = pixels[q];
				}
			}
		}
		// TODO assertion
	}
}
