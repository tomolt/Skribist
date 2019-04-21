#define GRAIN_BITS 8
#define GRAIN (1 << GRAIN_BITS)

static uint32_t CalcRasterWidth(SKR_Dimensions dims)
{
	return (dims.width + 7) & ~7;
}

static float CalcStepSize(float diff)
{
	if (diff == 0.0f) return 0.0f;
	return fabs(1.0f / diff);
}

static float FindFirstCrossing(float beg, float diff, float stepSize)
{
	if (stepSize == 0.0f) return 9.9f; // return anything >= 1.0f
	if (diff > 0.0f) {
		return stepSize * (ceilf(beg) - beg);
	} else {
		return stepSize * (beg - floorf(beg));
	}
}

static void RasterizeDots(Workspace * restrict ws, int count)
{
	__m128i qbx = _mm_load_si128((__m128i *) ws->dotBuffer.qbx);
	__m128i qby = _mm_load_si128((__m128i *) ws->dotBuffer.qby);
	__m128i qex = _mm_load_si128((__m128i *) ws->dotBuffer.qex);
	__m128i qey = _mm_load_si128((__m128i *) ws->dotBuffer.qey);

	// pixel coordinates
	__m128i px = _mm_srli_epi32(_mm_min_epi32(qbx, qex), GRAIN_BITS);
	__m128i py = _mm_srli_epi32(_mm_min_epi32(qby, qey), GRAIN_BITS);

	__m128i rasterWidth = _mm_set1_epi32(ws->rasterWidth);
	__m128i idx = _mm_add_epi32(_mm_mullo_epi32(rasterWidth, py), px);

	__m128i windingAndCover = _mm_sub_epi32(qbx, qex);
	__m128i area = _mm_set1_epi32(GRAIN);
	area = _mm_add_epi32(area, _mm_srai_epi32(
		_mm_abs_epi32(_mm_sub_epi32(qey, qby)), 1));
	area = _mm_sub_epi32(area, _mm_max_epi32(qey, qby));
	area = _mm_add_epi32(area, _mm_slli_epi32(py, GRAIN_BITS));
	__m128i edge = _mm_srai_epi32(_mm_mullo_epi32(windingAndCover, area), GRAIN_BITS);
	__m128i edgeAndTail = _mm_packs_epi32(edge, windingAndCover);
	
	__attribute__((aligned(16))) uint32_t idxS[4];
	_mm_store_si128((__m128i *) idxS, idx);
	__attribute__((aligned(16))) int16_t edgeAndTailS[8];
	_mm_store_si128((__m128i *) edgeAndTailS, edgeAndTail);

	for (int i = 0; i < count; ++i) {
		uint32_t curIdx = idxS[i];

		RasterCell cell = ws->raster[curIdx];

		cell.edgeValue += edgeAndTailS[i];
		cell.tailValue += edgeAndTailS[i + 4];

		ws->raster[curIdx] = cell;
	}
}

static void QueueDot(Workspace * restrict ws,
	uint32_t qbx, uint32_t qby, uint32_t qex, uint32_t qey)
{
	if (ws->dotBufferCount == 4) {
		RasterizeDots(ws, 4);
		ws->dotBufferCount = 0;
	}
	int i = ws->dotBufferCount++;
	ws->dotBuffer.qbx[i] = qbx;
	ws->dotBuffer.qby[i] = qby;
	ws->dotBuffer.qex[i] = qex;
	ws->dotBuffer.qey[i] = qey;
}

#define QUANTIZE(x) ((uint32_t) ((x) * (float) GRAIN + 0.5f))

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

	uint32_t prev_qx = QUANTIZE(line.beg.x);
	uint32_t prev_qy = QUANTIZE(line.beg.y);

	while (xt < 1.0f || yt < 1.0f) {
		float t;
		if (xt < yt) {
			t = xt;
			xt += sx;
		} else {
			t = yt;
			yt += sy;
		}

		uint32_t qx = QUANTIZE(line.beg.x + t * dx);
		uint32_t qy = QUANTIZE(line.beg.y + t * dy);

		QueueDot(ws, prev_qx, prev_qy, qx, qy);

		prev_qx = qx;
		prev_qy = qy;
	}

	uint32_t qx = QUANTIZE(line.end.x);
	uint32_t qy = QUANTIZE(line.end.y);

	QueueDot(ws, prev_qx, prev_qy, qx, qy);
}

static void DrawLine(Workspace * restrict ws, Line line)
{
	if (gabs(line.end.x - line.beg.x) >= 1.0f / GRAIN) {
		RasterizeLine(ws, line);
	}
}

static void EndRasterizing(Workspace * restrict ws)
{
	if (ws->dotBufferCount > 0) {
		RasterizeDots(ws, ws->dotBufferCount);
		ws->dotBufferCount = 0;
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

