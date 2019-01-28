/*

Copyright 2019 Thomas Oltmann

---- LICENSE ----

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/*

---- REFERENCES ----

V. Gaultney, M. Hosken, A. Ward, "An Introduction to TrueType Fonts: A look inside the TTF format",
	https://scripts.sil.org/iws-chapter08
Steve Hanov, "Let's read a Truetype font file from scratch",
	http://stevehanov.ca/blog/index.php?id=143
Apple Inc., "TrueType Reference Manual",
	https://developer.apple.com/fonts/TrueType-Reference-Manual/
Eric S. Raymond, "The Lost Art of Structure Packing",
	http://www.catb.org/esr/structure-packing/
MikroElektronika d.o.o., "Packed Structures - Make the Memory Feel Safe",
	https://www.mikroe.com/blog/packed-structures-make-memory-feel-safe

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define xU16_(b) ((b)[0] << 8 | (b)[1])
#define xU16(v) xU16_((uint8_t *) &(v))
#define xU32_(b) ((b)[0] << 24 | (b)[1] << 16 | (b)[2] << 8 | (b)[3])
#define xU32(v) xU32_((uint8_t *) &(v))

typedef struct {
	int32_t version;
	int32_t fontRevision;
	uint32_t checkSumAdjustment;
	uint32_t magicNumber;
	uint16_t flags;
	uint16_t unitsPerEm;
	int64_t created;
	int64_t modified;
	int16_t xMin;
	int16_t yMin;
	int16_t xMax;
	int16_t yMax;
	uint16_t macStyle;
	uint16_t lowestRecPPEM;
	int16_t fontDirectionHint;
	int16_t indexToLocFormat;
	int16_t glyphDataFormat;
} headTbl;

typedef struct {
	int16_t numContours;
	int16_t xMin;
	int16_t yMin;
	int16_t xMax;
	int16_t yMax;
} GlyphHdr;

typedef uint8_t *glyfTbl;

typedef struct {
	char tag[4];
	uint32_t checksum;
	uint32_t offset;
	uint32_t length;
} TableInfo;

typedef struct {
	uint32_t scaler_type;
	uint16_t num_tables;
	uint16_t search_range;
	uint16_t entry_selector;
	uint16_t range_shift;
	TableInfo tables[];
} offsetTbl;

typedef struct {
	unsigned long glyf;
	unsigned long head;
} OffsetCache;

static OffsetCache cache_offsets(offsetTbl *offt)
{
	OffsetCache cache = { 0 };
	int count = xU16(offt->num_tables);
	int idx = 0, cmp;
	do {
		cmp = strncmp(offt->tables[idx].tag, "glyf", 4);
		if (!cmp) cache.glyf = xU32(offt->tables[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	do {
		cmp = strncmp(offt->tables[idx].tag, "head", 4);
		if (!cmp) cache.head = xU32(offt->tables[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	return cache;
}

int main(int argc, char const *argv[])
{
	int descr = open("Ubuntu-C.ttf", O_RDONLY);
	struct stat stat;
	fstat(descr, &stat);
	unsigned char *mapped = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, descr, 0);
	close(descr);

	OffsetCache offcache = cache_offsets((offsetTbl *) mapped);
	printf("glyf: %lu\n", offcache.glyf);
	printf("head: %lu\n", offcache.head);

	munmap(mapped, stat.st_size);
	return EXIT_SUCCESS;
}
