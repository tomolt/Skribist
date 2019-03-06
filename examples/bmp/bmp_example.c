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

#include <stdint.h>

#include "../../source/reading.c" // FIXME remove this dependency
#include "Skribist.h"

#include <stdio.h>
#include <stdlib.h>
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

static void fmt_le_dword(char *buf, uint32_t v)
{
	buf[0] = v & 0xFF;
	buf[1] = v >> 8 & 0xFF;
	buf[2] = v >> 16 & 0xFF;
	buf[3] = v >> 24 & 0xFF;
}

static void write_bmp(FILE *outFile)
{
	char hdr[54] = { 0 };
	// Header
	hdr[0] = 'B';
	hdr[1] = 'M';
	fmt_le_dword(&hdr[2], 54 + 3 * WIDTH * HEIGHT); // size of file
	hdr[10] = 54; // offset to image data
	// InfoHeader
	hdr[14] = 40; // size of InfoHeader
	fmt_le_dword(&hdr[18], WIDTH);
	fmt_le_dword(&hdr[22], HEIGHT);
	hdr[26] = 1; // color planes
	hdr[28] = 24; // bpp
	fwrite(hdr, 1, 54, outFile);
	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) {
			unsigned char c = olt_GLOBAL_image[WIDTH * y + x];
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

	unsigned char *rawData;
	ret = read_file("../Ubuntu-C.ttf", (void **) &rawData);
	assert(ret == 0);

	FILE *outFile = fopen("demo.bmp", "wb");
	assert(outFile != NULL);

	OffsetCache offcache = olt_INTERN_cache_offsets(rawData);
	olt_INTERN_parse_head(rawData + offcache.head);
	olt_INTERN_parse_maxp(rawData + offcache.maxp);

	printf("unitsPerEm: %d\n", olt_GLOBAL_unitsPerEm);
	printf("numGlyphs: %d\n", olt_GLOBAL_numGlyphs + 1);

	unsigned long outlineOffset = olt_INTERN_get_outline(rawData + offcache.loca, glyph);
	printf("outlineOffset[0]: %lu\n", outlineOffset);

	ParsingClue parsingClue = skrExploreOutline(rawData + offcache.glyf + outlineOffset);
	CurveBuffer curveList = (CurveBuffer) {
		.space = parsingClue.neededSpace,
		.count = 0,
		.elems = calloc(parsingClue.neededSpace, sizeof(Curve)) };
	skrParseOutline(parsingClue, &curveList);
	assert(curveList.space == curveList.count);

	Transform transform = { { 0.5 * WIDTH / olt_GLOBAL_unitsPerEm, 0.5 * HEIGHT / olt_GLOBAL_unitsPerEm }, { WIDTH / 2.0, HEIGHT / 2.0 } };

	// TODO API cleanup; Parser output should maybe even be directly used as the tessel stack.
	CurveBuffer tesselStack = {
		.space = 1000,
		.count = 0,
		.elems = malloc(1000 * sizeof(Curve)) };
	LineBuffer lineList = {
		.space = 1000,
		.count = 0,
		.elems = malloc(1000 * sizeof(Line)) };

	skrBeginTesselating(&curveList, transform, &tesselStack);
	skrContinueTesselating(&tesselStack, 0.5, &lineList);

	skrRasterizeLines(&lineList);

	olt_INTERN_gather();
	write_bmp(outFile);

	fclose(outFile);

	free(rawData);

	return EXIT_SUCCESS;
}
