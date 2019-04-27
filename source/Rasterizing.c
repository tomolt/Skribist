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

	uint32_t idx = ws->rasterWidth * (qly / GRAIN) + qlx / GRAIN;

	uint32_t cell = ws->raster[idx];

	int32_t windingAndCover = qbx - qex;
	int32_t area = GRAIN - gabs(qby - qey) / 2 - (qly & (GRAIN - 1));
	int32_t edgeValue = windingAndCover * area / GRAIN;

	cell = cell + ((edgeValue & 0xFFFF) | (windingAndCover << 16));

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

static void TransposeCells(RasterCell const cells[8],
	__m128i * restrict edgeValues, __m128i * restrict tailValues)
{
	__m128i const edgeMask = _mm_set1_epi32(0xFFFF);

	__m128i lowerCells = _mm_loadu_si128((__m128i const *) cells);
	__m128i upperCells = _mm_loadu_si128((__m128i const *) cells + 1);

	__m128i lowerEdges = _mm_and_si128(lowerCells, edgeMask);
	__m128i upperEdges = _mm_and_si128(upperCells, edgeMask);
	*edgeValues = _mm_packs_epi32(lowerEdges, upperEdges);

	__m128i lowerTails = _mm_srli_epi32(lowerCells, 16);
	__m128i upperTails = _mm_srli_epi32(upperCells, 16);
	*tailValues = _mm_packs_epi32(lowerTails, upperTails);
}

void skrCastImage(
	RasterCell const * restrict source,
	unsigned char * restrict dest,
	SKR_Dimensions dims)
{
	// TODO read from workspace instead
	long const width = CalcRasterWidth(dims);

	for (long col = 0; col < width; col += 8) {

		RasterCell const * restrict cell = source + col;
		unsigned char * restrict pixel = dest + col;

		__m128i accumulators = _mm_setzero_si128();

		for (long i = dims.height; i > 0; --i, cell += width, pixel += dims.width) {
			__m128i edgeValues, tailValues;
			TransposeCells(cell, &edgeValues, &tailValues);

			__m128i values = _mm_adds_epi16(accumulators, edgeValues);
			accumulators = _mm_adds_epi16(accumulators, tailValues);
			
			__m128i compactValues = _mm_packus_epi16(values, _mm_setzero_si128());
			int toGo = dims.width - col;
			if (toGo >= 8) {
				_mm_storeu_si64(pixel, compactValues);
			} else {
				__attribute__((aligned(8))) char pixels[8];
				_mm_storel_epi64((__m128i *) pixels, compactValues);
				memcpy(pixel, pixels, toGo);
			}
		}
		// TODO assertion
	}
}

#if 0
void skrConvertImage(unsigned char const * restrict source, unsigned char * restrict dest, SKR_Dimensions dims, SKR_Format destFormat))
{
	for (long i = 0; i < dims.width * dims.height; ++i) {
		
	}
}
#endif

