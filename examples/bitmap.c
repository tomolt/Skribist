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

// TODO public domain or equivalent licensing for all examples!!

#include <stdint.h>

#include "Skribist.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <assert.h>

static int read_file(char const *filename, void **addr)
{
	FILE *file = fopen(filename, "rw");
	if (file == NULL) {
		return -1;
	}
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	unsigned char *data = malloc(length);
	if (data == NULL) {
		fclose(file);
		return -1;
	}
	long count = fread(data, 1, length, file);
	if (count != length) {
		free(data);
		fclose(file);
		return -1;
	}
	fclose(file);
	*addr = data;
	return 0;
}

static void fmt_le_dword(unsigned char *buf, uint32_t v)
{
	buf[0] = v & 0xFF;
	buf[1] = v >> 8 & 0xFF;
	buf[2] = v >> 16 & 0xFF;
	buf[3] = v >> 24 & 0xFF;
}

static void write_bmp(unsigned char * image, FILE * outFile, SKR_Dimensions dim)
{
	unsigned char hdr[54] = { 0 };
	// Header
	hdr[0] = 'B';
	hdr[1] = 'M';
	fmt_le_dword(&hdr[2], 54 + 4 * dim.width * dim.height); // size of file
	hdr[10] = 54; // offset to image data
	// InfoHeader
	hdr[14] = 40; // size of InfoHeader
	fmt_le_dword(&hdr[18], dim.width);
	fmt_le_dword(&hdr[22], dim.height);
	hdr[26] = 1; // color planes
	hdr[28] = 32; // bpp
	fwrite(hdr, 1, 54, outFile);
	fwrite(image, 4, dim.width * dim.height, outFile);
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct {
	int glyph;
	float size;
	float x, y;
} SKR_Assembly;

static int GetCharCodeFromUTF8(char const * restrict * restrict ptr)
{
	int bytes = 0;
	for (int c = **ptr; c & 0x80; c <<= 1) ++bytes;
	if (bytes == 1 || bytes > 4) return '?';
	int code = *(*ptr)++ & (0x7F >> bytes);
	for (int i = 1; i < bytes; ++i, ++*ptr) {
		if ((**ptr & 0xC0) != 0x80) return '?';
		code = (code << 6) | (**ptr & 0x3F);
	}
	return code;
}

SKR_Status skrAssembleStringUTF8(SKR_Font * restrict font,
	char const * restrict line, float size,
	SKR_Assembly * restrict assembly, int * restrict count)
{
	// TODO watch for buffer overflows
	*count = 0;
	float x = 0.0f;
	while (*line != '\0') {
		int charCode = GetCharCodeFromUTF8(&line);
		Glyph glyph = skrGlyphFromCode(font, charCode);
		SKR_HorMetrics metrics;
		SKR_Status s = skrGetHorMetrics(font, glyph, &metrics);
		if (s) return s;
		assembly[(*count)++] = (SKR_Assembly) { glyph, size, x, 0.0f };
		x += metrics.advanceWidth * size;
	}
	return SKR_SUCCESS;
}

static SKR_Status skrGetAssemblyBounds(SKR_Font * restrict font,
	SKR_Assembly * restrict assembly, int count, SKR_Bounds * restrict bounds)
{
	if (count <= 0) return SKR_FAILURE;
	SKR_Bounds total, next;
	SKR_Transform transform = {
		assembly[0].size, assembly[0].size, assembly[0].x, assembly[0].y };
	SKR_Status s = skrGetOutlineBounds(font, assembly[0].glyph, transform, &total);
	if (s) return s;
	for (int i = 1; i < count; ++i) {
		SKR_Transform transform = {
			assembly[i].size, assembly[i].size, assembly[i].x, assembly[i].y };
		s = skrGetOutlineBounds(font, assembly[i].glyph, transform, &next);
		if (s) return s;
		total.xMin = min(total.xMin, next.xMin);
		total.yMin = min(total.yMin, next.yMin);
		total.xMax = max(total.xMax, next.xMax);
		total.yMax = max(total.yMax, next.yMax);
	}
	*bounds = total;
	return SKR_SUCCESS;
}

static SKR_Status skrDrawAssembly(SKR_Font * restrict font,
	SKR_Assembly * restrict assembly, int count,
	RasterCell * restrict raster, SKR_Bounds bounds)
{
	SKR_Dimensions dims = { bounds.xMax - bounds.xMin, bounds.yMax - bounds.yMin };
	for (int i = 0; i < count; ++i) {
		SKR_Assembly amb = assembly[i];
		SKR_Transform transform = { amb.size, amb.size,
			amb.x - bounds.xMin, amb.y - bounds.yMin };
		SKR_Status s = skrDrawOutline(font, amb.glyph, transform, raster, dims);
		if (s) return s;
	}
	return SKR_SUCCESS;
}

int main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <char>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int ret;
	SKR_Status s = SKR_SUCCESS;

	unsigned char *rawData;
	ret = read_file("Ubuntu-C.ttf", (void **) &rawData);
	if (ret != 0) {
		fprintf(stderr, "Unable to open TTF font file.\n");
		return EXIT_FAILURE;
	}

	FILE *outFile = fopen("bitmap.bmp", "wb");
	if (outFile == NULL) {
		fprintf(stderr, "Unable to open output BMP file.\n");
		return EXIT_FAILURE;
	}

	skrInitializeLibrary();

	SKR_Font font = {
		.data = rawData,
		.length = 0 }; // FIXME
	s = skrInitializeFont(&font);
	if (s != SKR_SUCCESS) {
		fprintf(stderr, "Unable to read TTF font file.\n");
		return EXIT_FAILURE;
	}

	float const size = 64.0f;

	int count;
	SKR_Assembly assembly[100];
	s = skrAssembleStringUTF8(&font, argv[1], size, assembly, &count);

	SKR_Bounds bounds;
	s = skrGetAssemblyBounds(&font, assembly, count, &bounds);
	if (s != SKR_SUCCESS) {
		return EXIT_FAILURE;
	}

	SKR_Dimensions dims = {
		.width  = bounds.xMax - bounds.xMin,
		.height = bounds.yMax - bounds.yMin };

	unsigned long cellCount = skrCalcCellCount(dims);
	RasterCell * raster = calloc(cellCount, sizeof(RasterCell));
	unsigned char * image = calloc(dims.width * dims.height, 4);

	s = skrDrawAssembly(&font, assembly, count, raster, bounds);
	if (s != SKR_SUCCESS) {
		fprintf(stderr, "This type of outline is not implemented yet.\n");
		return EXIT_FAILURE;
	}

	skrTransposeRaster(raster, dims);
	skrAccumulateRaster(raster, dims);
	skrExportImage(raster, image, dims);

	free(raster);

	write_bmp(image, outFile, dims);

	free(image);

	fclose(outFile);

	free(rawData);

	return EXIT_SUCCESS;
}
