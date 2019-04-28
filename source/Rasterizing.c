#define GRAIN_BITS 8
#define GRAIN (1 << GRAIN_BITS)

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

static void RasterizeDot(
	Workspace * restrict ws,
	uint32_t qbx, uint32_t qby, uint32_t qex, uint32_t qey)
{
	uint32_t qlx = min(qbx, qex);
	uint32_t qly = min(qby, qey);
	
	SKR_assert(qlx / GRAIN < ws->dims.width);
	SKR_assert(qly / GRAIN < ws->dims.height);

	uint32_t idx = ws->rasterWidth * (qly / GRAIN) + qlx / GRAIN;

	uint32_t cell = ws->raster[idx];

	int32_t windingAndCover = qbx - qex;
	int32_t area = GRAIN - gabs(qby - qey) / 2 - (qly & (GRAIN - 1));
	int32_t edgeValue = windingAndCover * area / GRAIN;

	((int16_t * restrict) &cell)[0] += edgeValue;
	((int16_t * restrict) &cell)[1] += windingAndCover;

	ws->raster[idx] = cell;
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

		RasterizeDot(ws, prev_qx, prev_qy, qx, qy);

		prev_qx = qx;
		prev_qy = qy;
	}

	uint32_t qx = QUANTIZE(line.end.x);
	uint32_t qy = QUANTIZE(line.end.y);

	RasterizeDot(ws, prev_qx, prev_qy, qx, qy);
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

void skrTransposeRaster(RasterCell * restrict raster, SKR_Dimensions dims)
{
	// TODO read from workspace instead
	long const width = CalcRasterWidth(dims);
	__m128i const edgeMask = _mm_set1_epi32(0xFFFF);
	RasterCell * const rasterEnd = raster + width * dims.height;

	for (RasterCell * row = raster; row < rasterEnd; row += width) {
		for (RasterCell * col = row; col < row + width; col += 8) {
			__m128i * restrict lower = (__m128i *) col;
			__m128i * restrict upper = (__m128i *) col + 1;

			__m128i lowerEdges = _mm_and_si128(*lower, edgeMask);
			__m128i upperEdges = _mm_and_si128(*upper, edgeMask);

			__m128i lowerTails = _mm_srli_epi32(*lower, 16);
			__m128i upperTails = _mm_srli_epi32(*upper, 16);

			*lower = _mm_packus_epi32(lowerEdges, upperEdges);
			*upper = _mm_packus_epi32(lowerTails, upperTails);
		}
	}
}

void skrAccumulateRaster(RasterCell * restrict raster, SKR_Dimensions dims)
{
	// TODO read from workspace instead
	long const width = CalcRasterWidth(dims);
	for (long col = 0; col < width; col += 8) {
		uint32_t * cursor = raster + col;
		__m128i accumulator = _mm_setzero_si128();
		for (long i = 0; i < dims.height; ++i, cursor += width) {
			__m128i * restrict pointer = (__m128i *) cursor;
			__m128i cellValue = _mm_adds_epi16(accumulator, *pointer);
			accumulator = _mm_adds_epi16(accumulator, *(pointer + 1));
			cellValue = _mm_max_epi16(cellValue, _mm_setzero_si128());
			_mm_storeu_si128(pointer, _mm_unpacklo_epi16(cellValue, _mm_setzero_si128()));
			_mm_storeu_si128(pointer + 1, _mm_unpackhi_epi16(cellValue, _mm_setzero_si128()));
		}
		SKR_assert(_mm_movemask_epi8(_mm_cmpeq_epi8(accumulator, _mm_setzero_si128())) == 0xFFFF);
	}
}

#if 0
	SKR_ALPHA_8_UINT,
	SKR_GRAY_8_SRGB,

	SKR_ALPHA_16_UINT,
	SKR_RGB_5_6_5_UINT,
	SKR_BGR_5_6_5_UINT,

	SKR_RGBA_32_UINT,
	SKR_RGBA_32_SRGB,
	SKR_BGRA_32_UINT,
	SKR_BGRA_32_SRGB,

	SKR_RGB_48_UINT,
	SKR_RGBA_64_UINT,
	SKR_RGBA_128_FLOAT
#endif

void skrExportImage(RasterCell * restrict raster,
	unsigned char * restrict image, SKR_Dimensions dims)
{
	// NOTE this codepath assumes little-endianness
	// TODO read from workspace instead
	long const width = CalcRasterWidth(dims);
	uint32_t * restrict image32 = (uint32_t *) image;
	__m128i const constMax = _mm_set1_epi32(0xFF);
	__m128i const shuffleMask = _mm_set_epi8(
			0xFF, 0x0C, 0x0C, 0x0C,
			0xFF, 0x08, 0x08, 0x08,
			0xFF, 0x04, 0x04, 0x04,
			0xFF, 0x00, 0x00, 0x00);
	for (long row = 0; row < dims.height; ++row) {
		for (long col = 0; col < dims.width; col += 4) {
			__m128i value = _mm_loadu_si128((__m128i const *) &raster[width * row + col]);
			value = _mm_max_epi16(value, _mm_setzero_si128());
			value = _mm_min_epi16(value, constMax);
			__m128i pixel = _mm_shuffle_epi8(value, shuffleMask);
			unsigned long imageIdx = dims.width * row + col;
			int headroom = dims.width - col;
			if (headroom >= 4) {
				_mm_storeu_si128((__m128i *) &image32[imageIdx], pixel);
			} else {
				uint32_t pixelS[4];
				_mm_storeu_si128((__m128i *) pixelS, pixel);
				for (int i = 0; i < headroom; ++i) {
					image32[imageIdx + i] = pixelS[i];
				}
			}
		}
	}
}

