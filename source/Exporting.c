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

static __m128i GatherEdge(__m128i * restrict pointer)
{
	__m128i const edgeMask = _mm_set1_epi32(0xFFFF);
	__m128i lowerEdge = _mm_and_si128(pointer[0], edgeMask);
	__m128i upperEdge = _mm_and_si128(pointer[1], edgeMask);
	return _mm_packus_epi32(lowerEdge, upperEdge);
}

static __m128i GatherTail(__m128i * restrict pointer)
{
	__m128i lowerTail = _mm_srli_epi32(pointer[0], 16);
	__m128i upperTail = _mm_srli_epi32(pointer[1], 16);
	return _mm_packus_epi32(lowerTail, upperTail);
}

#if 0
	SKR_ALPHA_8_UINT,
	SKR_ALPHA_16_UINT,
	SKR_RGB_5_6_5_UINT,
	SKR_RGBA_32_UINT,
	SKR_RGBA_64_UINT,
	SKR_GRAY_8_SRGB,
	SKR_RGBA_32_SRGB,
#endif

static __m128i BoundPixelValues(__m128i value)
{
	__m128i const constMax = _mm_set1_epi16(0xFF);
	return _mm_min_epi16(value, constMax);
}

static void ConvertPixels(__m128i value, __m128i * restrict pixels)
{
	__m128i const lowerMask = _mm_set_epi8(
		0xFF, 0x06, 0x06, 0x06,
		0xFF, 0x04, 0x04, 0x04,
		0xFF, 0x02, 0x02, 0x02,
		0xFF, 0x00, 0x00, 0x00);
	__m128i const upperMask = _mm_set_epi8(
		0xFF, 0x0E, 0x0E, 0x0E,
		0xFF, 0x0C, 0x0C, 0x0C,
		0xFF, 0x0A, 0x0A, 0x0A,
		0xFF, 0x08, 0x08, 0x08);
	pixels[0] = _mm_shuffle_epi8(value, lowerMask);
	pixels[1] = _mm_shuffle_epi8(value, upperMask);
}

static void WritePixels(unsigned char * restrict image, SKR_Dimensions dims,
	__m128i * restrict pixels, unsigned long row, unsigned long col)
{
	SKR_assert(col < dims.width && row < dims.height);
	uint32_t * restrict image32 = (uint32_t *) image;
	unsigned long idx = dims.width * row + col;
	int headroom = dims.width - col;
	if (headroom >= 8) {
		_mm_storeu_si128((__m128i *) (image32 + idx), pixels[0]);
		_mm_storeu_si128((__m128i *) (image32 + idx + 4), pixels[1]);
	} else {
		uint32_t data[8];
		_mm_storeu_si128((__m128i *) &data[0], pixels[0]);
		_mm_storeu_si128((__m128i *) &data[4], pixels[1]);
		for (int i = 0; i < headroom; ++i) {
			image32[idx + i] = data[i];
		}
	}
}

void skrExportImage(RasterCell * restrict raster,
	unsigned char * restrict image, SKR_Dimensions dims)
{
	long const width = CalcRasterWidth(dims);
	for (long col = 0; col < width; col += 8) {
		uint32_t * cursor = raster + col;
		__m128i accumulator = _mm_setzero_si128();
		for (long row = 0; row < dims.height; ++row, cursor += width) {
			__m128i * restrict pointer = (__m128i *) cursor;

			__m128i edgeValue = GatherEdge(pointer);
			__m128i tailValue = GatherTail(pointer);

			__m128i cellValue = _mm_adds_epi16(accumulator, edgeValue);
			accumulator = _mm_adds_epi16(accumulator, tailValue);
			cellValue = _mm_max_epi16(cellValue, _mm_setzero_si128());

			cellValue = BoundPixelValues(cellValue);
			__m128i pixels[2];
			ConvertPixels(cellValue, pixels);
			WritePixels(image, dims, pixels, row, col);
		}
		SKR_assert(_mm_movemask_epi8(_mm_cmpeq_epi8(accumulator, _mm_setzero_si128())) == 0xFFFF);
	}
}

