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

#include "../../source/Reading.c" // FIXME remove this dependency
#include "Skribist.h"

#include <stdio.h>
#include <stdlib.h>
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

static void write_bmp(unsigned char * image, FILE * outFile, SKR_Dimensions dim)
{
	char hdr[54] = { 0 };
	// Header
	hdr[0] = 'B';
	hdr[1] = 'M';
	fmt_le_dword(&hdr[2], 54 + 3 * dim.width * dim.height); // size of file
	hdr[10] = 54; // offset to image data
	// InfoHeader
	hdr[14] = 40; // size of InfoHeader
	fmt_le_dword(&hdr[18], dim.width);
	fmt_le_dword(&hdr[22], dim.height);
	hdr[26] = 1; // color planes
	hdr[28] = 24; // bpp
	fwrite(hdr, 1, 54, outFile);
	for (int y = 0; y < dim.height; ++y) {
		for (int x = 0; x < dim.width; ++x) {
			unsigned char c = image[dim.width * y + x];
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
	if (ret != 0) {
		fprintf(stderr, "Unable to open TTF font file.\n");
		return EXIT_FAILURE;
	}

	FILE *outFile = fopen("out.bmp", "wb");
	if (outFile == NULL) {
		fprintf(stderr, "Unable to open output BMP file.\n");
		return EXIT_FAILURE;
	}

	SKR_Font font = {
		.data = rawData,
		.length = 0 }; // FIXME
	s = skrInitializeFont(&font);
	if (s != SKR_SUCCESS) {
		fprintf(stderr, "Unable to read TTF font file.\n");
		return EXIT_FAILURE;
	}

	s = skrLoadCMap((BYTES1 *) font.data + font.cmap.offset);
	if (s != SKR_SUCCESS) {
		fprintf(stderr, "Unsupported cmap format / encoding.\n");
		return EXIT_FAILURE;
	}

	BYTES1 * outline = skrGetOutlineAddr(&font, glyph);

	SKR_Rect rect = skrGetOutlineBounds(outline);

	Transform transform = {
		{ 64.0 / font.unitsPerEm, 64.0 / font.unitsPerEm },
		{ 64.0, 64.0 } };

	ParsingClue parsingClue;
	skrExploreOutline(outline, &parsingClue);
	CurveBuffer curveList = (CurveBuffer) {
		.space = parsingClue.neededSpace,
		.count = 0,
		.elems = calloc(parsingClue.neededSpace, sizeof(Curve)) };
	skrParseOutline(&parsingClue, &curveList);
	// assert(curveList.space == curveList.count);

	// TODO API cleanup; Parser output should maybe even be directly used as the tessel stack.
	CurveBuffer tesselStack = {
		.space = 1000,
		.count = 0,
		.elems = malloc(1000 * sizeof(Curve)) };
	LineBuffer lineList = {
		.space = 1000,
		.count = 0,
		.elems = malloc(1000 * sizeof(Line)) };

	SKR_Dimensions dim = {
		.width  = ceil(rect.xMax * transform.scale.x + transform.move.x),
		.height = ceil(rect.yMax * transform.scale.y + transform.move.y) };
	RasterCell * raster = calloc(dim.width * dim.height, sizeof(RasterCell));
	unsigned char * image = calloc(dim.width * dim.height, sizeof(unsigned char));

	skrBeginTesselating(&curveList, transform, &tesselStack);
	skrContinueTesselating(&tesselStack, 0.5, &lineList);

	skrRasterizeLines(&lineList, raster, dim);
	skrCastImage(raster, image, dim);

	write_bmp(image, outFile, dim);

	fclose(outFile);

	free(rawData);

	return EXIT_SUCCESS;
}
