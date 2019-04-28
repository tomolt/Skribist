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

static int code_from_utf8(char const * restrict str)
{
	int bytes = 0;
	for (int c = str[0]; c & 0x80; c <<= 1) ++bytes;
	if (bytes == 1 || bytes > 4) return '?';
	int code = str[0] & (0x7F >> bytes);
	for (int i = 1; i < bytes; ++i) {
		if ((str[i] & 0xC0) != 0x80) return '?';
		code = (code << 6) | (str[i] & 0x3F);
	}
	return code;
}

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
	fmt_le_dword(&hdr[2], 54 + (4 * dim.width) * dim.height); // size of file
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

int main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <char>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int charCode = code_from_utf8(argv[1]);

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

	Glyph glyph = skrGlyphFromCode(&font, charCode);

	SKR_Transform transform1 = { 64.0, 64.0, 0.0, 0.0 };

	SKR_Bounds bounds;
	s = skrGetOutlineBounds(&font, glyph, transform1, &bounds);
	if (s != SKR_SUCCESS) {
		return EXIT_FAILURE;
	}

	SKR_Transform transform2 = transform1;
	transform2.xMove -= bounds.xMin;
	transform2.yMove -= bounds.yMin;

	SKR_Dimensions dims = {
		.width  = bounds.xMax - bounds.xMin,
		.height = bounds.yMax - bounds.yMin };

	unsigned long cellCount = skrCalcCellCount(dims);
	RasterCell * raster = calloc(cellCount, sizeof(RasterCell));
	unsigned char * image = calloc(dims.width * dims.height, 4);

	s = skrDrawOutline(&font, glyph, transform2, raster, dims);
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
