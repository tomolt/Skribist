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

#include "../../source/reading.c" // FIXME remove this dependency
#include "Skribist.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

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

static void fmt_le_dword(char *buf, uint32_t v)
{
	buf[0] = v & 0xFF;
	buf[1] = v >> 8 & 0xFF;
	buf[2] = v >> 16 & 0xFF;
	buf[3] = v >> 24 & 0xFF;
}

static void write_bmp(SKR_Image image, FILE *outFile)
{
	char hdr[54] = { 0 };
	// Header
	hdr[0] = 'B';
	hdr[1] = 'M';
	fmt_le_dword(&hdr[2], 54 + 3 * image.width * image.height); // size of file
	hdr[10] = 54; // offset to image data
	// InfoHeader
	hdr[14] = 40; // size of InfoHeader
	fmt_le_dword(&hdr[18], image.width);
	fmt_le_dword(&hdr[22], image.height);
	hdr[26] = 1; // color planes
	hdr[28] = 24; // bpp
	fwrite(hdr, 1, 54, outFile);
	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; ++x) {
			unsigned char c = image.data[image.stride * y + x];
			fputc(c, outFile); // r
			fputc(c, outFile); // g
			fputc(c, outFile); // b
		}
	}
}

int main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <glyph>\n", argv[0]);
		return EXIT_FAILURE;
	}

	Glyph glyph = atol(argv[1]);

	int ret;
	SKR_Status s = SKR_SUCCESS;

	unsigned char *rawData;
	ret = read_file("../Ubuntu-C.ttf", (void **) &rawData);
	assert(ret == 0);

	FILE *outFile = fopen("out.bmp", "wb");
	assert(outFile != NULL);

	SKR_Font font = {
		.data = rawData,
		.length = 0 }; // FIXME
	s = skrInitializeFont(&font);
	if (s != SKR_SUCCESS) {
		fprintf(stderr, "Unable to read TTF font file.\n");
		return EXIT_FAILURE;
	}

	printf("unitsPerEm: %d\n", font.unitsPerEm);
	printf("numGlyphs: %d\n", font.numGlyphs + 1);

	long width = 256, height = 256;
	Transform transform = {
		{ 0.5 * width / font.unitsPerEm, 0.5 * height / font.unitsPerEm },
		{ width / 2.0, height / 2.0 } };

	unsigned long outlineOffset = olt_INTERN_get_outline(&font, glyph);
	printf("outlineOffset[0]: %lu\n", outlineOffset);

	// SKR_Rect rect = skrGetOutlineBounds((BYTES1 *) font.data + font.glyf.offset + outlineOffset);

	ParsingClue parsingClue;
	skrExploreOutline((BYTES1 *) font.data + font.glyf.offset + outlineOffset, &parsingClue);
	CurveBuffer curveList = (CurveBuffer) {
		.space = parsingClue.neededSpace,
		.count = 0,
		.elems = calloc(parsingClue.neededSpace, sizeof(Curve)) };
	skrParseOutline(&parsingClue, &curveList);
	assert(curveList.space == curveList.count);

	FILE *fOutParse = fopen("out.parse", "w");
	assert(fOutParse != NULL);
	fprintf(fOutParse, "# X Y\n");
	for (int i = 0; i < curveList.count; ++i) {
		Curve * curve = &curveList.elems[i];
		fprintf(fOutParse, "%f %f\n", curve->beg.x, curve->beg.y);
		fprintf(fOutParse, "%f %f\n", curve->ctrl.x, curve->ctrl.y);
		fprintf(fOutParse, "%f %f\n\n", curve->end.x, curve->end.y);
	}
	fclose(fOutParse);

	// TODO API cleanup; Parser output should maybe even be directly used as the tessel stack.
	CurveBuffer tesselStack = {
		.space = 1000,
		.count = 0,
		.elems = malloc(1000 * sizeof(Curve)) };
	LineBuffer lineList = {
		.space = 1000,
		.count = 0,
		.elems = malloc(1000 * sizeof(Line)) };

	RasterCell * rasterCells = calloc(width * height, sizeof(RasterCell));
	SKR_Raster raster = { .width = width, .height = height, .data = rasterCells };

	unsigned char * imageData = calloc(width * height, sizeof(unsigned char));
	SKR_Image image = { .width = width, .height = height, .stride = width, .data = imageData };

	skrBeginTesselating(&curveList, transform, &tesselStack);
	skrContinueTesselating(&tesselStack, 0.5, &lineList);

	skrRasterizeLines(&lineList, raster);

	olt_INTERN_gather(raster, image);
	write_bmp(image, outFile);

	fclose(outFile);

	free(rawData);

	return EXIT_SUCCESS;
}
