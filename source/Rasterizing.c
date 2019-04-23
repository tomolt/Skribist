#define GRAIN_BITS 8
#define GRAIN (1 << GRAIN_BITS)

static uint32_t Quantize(float value)
{
	return (uint32_t) (value * GRAIN + 0.5f);
}

static void RasterizeDots(
	Workspace * restrict ws, int count,
	float startX, float startY, float endX[4], float endY[4])
{
	uint32_t qex[4], qey[4];
	for (int i = 0; i < 4; ++i) {
		qex[i] = Quantize(endX[i]);
		qey[i] = Quantize(endY[i]);
	}

	uint32_t qbx[4], qby[4];
	qbx[0] = Quantize(startX);
	qby[0] = Quantize(startY);
	for (int i = 1; i < 4; ++i) {
		qbx[i] = qex[i - 1];
		qby[i] = qey[i - 1];
	}

	uint32_t cellIdx[4];
	int16_t edgeValue[4], tailValue[4];
	for (int i = 0; i < 4; ++i) {
		uint32_t px = min(qbx[i], qex[i]) >> GRAIN_BITS;
		uint32_t py = min(qby[i], qey[i]) >> GRAIN_BITS;

		SKR_assert(px < ws->dims.width);
		SKR_assert(py < ws->dims.height);

		int32_t windingAndCover = qbx[i] - qex[i];
		uint32_t area = GRAIN;
		area += gabs(qey[i] - qby[i]) >> 1;
		area -= max(qey[i], qby[i]);
		area += py << GRAIN_BITS;

		cellIdx[i] = ws->rasterWidth * py + px;
		edgeValue[i] = windingAndCover * area / GRAIN;
		tailValue[i] = windingAndCover;
	}

	for (int i = 0; i < count; ++i) {
		uint32_t idx = cellIdx[i];

		RasterCell cell = ws->raster[idx];

		cell.edgeValue += edgeValue[i];
		cell.tailValue += tailValue[i];

		ws->raster[idx] = cell;
	}
}

static float CalcStepSize(float diff)
{
	if (diff == 0.0f) return 0.0f;
	return fabs(1.0f / diff);
}

static float FindFirstCrossing(float beg, float diff, float stepSize)
{
	if (diff == 0.0f) return 9.9f; // return anything >= 1.0f
	if (diff < 0.0f) beg = -beg;
	return stepSize * (ceilf(beg) - beg);
}

/*
RasterizeLine() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.
*/

static void RasterizeLine(Workspace * restrict ws, Line line)
{
	float dx = line.end.x - line.beg.x;
	float dy = line.end.y - line.beg.y;

	// step size along each axis
	float sx = CalcStepSize(dx);
	float sy = CalcStepSize(dy);
	// t of next vertical / horizontal intersection
	float xt = FindFirstCrossing(line.beg.x, dx, sx);
	float yt = FindFirstCrossing(line.beg.y, dy, sy);

	int count = 0;
	float startX, startY, endX[4], endY[4];
	startX = line.beg.x;
	startY = line.beg.y;

	while (xt < 1.0f || yt < 1.0f) {
		float t;
		if (xt < yt) {
			t = xt;
			xt += sx;
		} else {
			t = yt;
			yt += sy;
		}

		float curX = line.beg.x + t * dx;
		float curY = line.beg.y + t * dy;

		endX[count] = curX;
		endY[count] = curY;

		++count;

		if (count > 3) {
			RasterizeDots(ws, count, startX, startY, endX, endY);
			startX = curX;
			startY = curY;
			count = 0;
		}
	}

	endX[count] = line.end.x;
	endY[count] = line.end.y;

	++count;

	RasterizeDots(ws, count, startX, startY, endX, endY);
}

static void DrawLine(Workspace * restrict ws, Line line)
{
	if (gabs(line.end.x - line.beg.x) >= 1.0f / GRAIN) {
		RasterizeLine(ws, line);
	}
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

static unsigned char LinearToGamma[GRAIN + 1];

void skrInitializeLibrary(void)
{
	for (int i = 0; i <= GRAIN; ++i) {
		LinearToGamma[i] = round(CalcLinearToGamma(i / (float) GRAIN) * 255.0);
	}
}

uint32_t CalcRasterWidth(SKR_Dimensions dims)
{
	return (dims.width + 7) & ~7;
}

// TODO get rid of this function
unsigned long skrCalcCellCount(SKR_Dimensions dims)
{
	return CalcRasterWidth(dims) * dims.height;
}

void skrCastImage(
	RasterCell const * restrict source,
	unsigned char * restrict dest,
	SKR_Dimensions dims)
{
	// TODO read from workspace instead
	long const width = CalcRasterWidth(dims);

	__m128i const lowMask  = _mm_set1_epi32(0x0000FFFF);
	__m128i const highMask = _mm_set1_epi32(0xFFFF0000);
#define SHUFFLE_MASK _MM_SHUFFLE(3, 1, 2, 0)

	for (long col = 0; col < width; col += 8) {

		RasterCell const * restrict cell = source + col;
		unsigned char * restrict pixel = dest + col;

		__m128i accumulators = _mm_setzero_si128();

		for (long i = dims.height; i > 0; --i, cell += width, pixel += dims.width) {

			__m128i cells1 = _mm_loadu_si128(
				(__m128i const *) cell);
			__m128i cells2 = _mm_loadu_si128(
				(__m128i const *) (cell + 4));

			__m128i edgeValues1 = _mm_and_si128(cells1, lowMask);
			__m128i edgeValues2 = _mm_slli_epi32(cells2, 16);
			__m128i edgeValues = _mm_or_si128(edgeValues1, edgeValues2);

			__m128i tailValues1 = _mm_srli_epi32(cells1, 16);
			__m128i tailValues2 = _mm_and_si128(cells2, highMask);
			__m128i tailValues = _mm_or_si128(tailValues1, tailValues2);

			__m128i values = _mm_adds_epi16(accumulators, edgeValues);

			accumulators = _mm_adds_epi16(accumulators, tailValues);
			
			__m128i shuf1 = _mm_shufflelo_epi16(values, SHUFFLE_MASK);
			__m128i shuf2 = _mm_shufflehi_epi16(shuf1, SHUFFLE_MASK);
			__m128i shuf3 = _mm_shuffle_epi32(shuf2, SHUFFLE_MASK);

			__m128i compactValues = _mm_packus_epi16(shuf3, _mm_setzero_si128());
			int togo = dims.width - col;
			__attribute__((aligned(8))) char pixels[8];
			_mm_storel_epi64((__m128i *) pixels, compactValues);
			memcpy(pixel, pixels, min(togo, 8));
		}
		// TODO assertion
	}
#undef SHUFFLE_MASK
}

#if 0
void skrConvertImage(unsigned char const * restrict source, unsigned char * restrict dest, SKR_Dimensions dims, SKR_Format destFormat))
{
	for (long i = 0; i < dims.width * dims.height; ++i) {
		
	}
}
#endif

