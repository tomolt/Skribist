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
#include <assert.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef uint8_t  BYTES1;
typedef uint16_t BYTES2;
typedef uint32_t BYTES4;
typedef uint64_t BYTES8;

static uint16_t ru16(BYTES2 raw)
{
	BYTES1 *bytes = (BYTES1 *) &raw;
	uint16_t b0 = bytes[1], b1 = bytes[0];
	return b0 | b1 << 8;
}

static int16_t ri16(BYTES2 raw)
{
	uint16_t u = ru16(raw);
	return *(int16_t *) &u;
}

static uint32_t ru32(BYTES4 raw)
{
	BYTES1 *bytes = (BYTES1 *) &raw;
	uint32_t b0 = bytes[3], b1 = bytes[2];
	uint32_t b2 = bytes[1], b3 = bytes[0];
	return b0 | b1 << 8 | b2 << 16 | b3 << 24;
}

typedef struct {
	BYTES4 version;
	BYTES4 fontRevision;
	BYTES4 checkSumAdjustment;
	BYTES4 magicNumber;
	BYTES2 flags;
	BYTES2 unitsPerEm;
	BYTES8 created;
	BYTES8 modified;
	BYTES2 xMin;
	BYTES2 yMin;
	BYTES2 xMax;
	BYTES2 yMax;
	BYTES2 macStyle;
	BYTES2 lowestRecPPEM;
	BYTES2 fontDirectionHint;
	BYTES2 indexToLocFormat;
	BYTES2 glyphDataFormat;
} headTbl;

// Simple glyph flags
#define SGF_ON_CURVE_POINT 0x01
#define SGF_SHORT_X_COORD  0x02
#define SGF_SHORT_Y_COORD  0x04
#define SGF_REPEAT_FLAG    0x08
#define SGF_POSITIVE_X     0x10
#define SGF_REUSE_PREV_X   0x10
#define SGF_POSITIVE_Y     0x20
#define SGF_REUSE_PREV_Y   0x20
#define SGF_OVERLAP_SIMPLE 0x40

typedef struct {
	char tag[4];
	BYTES4 checksum;
	BYTES4 offset;
	BYTES4 length;
} offsetEnt;

typedef struct {
	BYTES4 scalerType;
	BYTES2 numTables;
	BYTES2 searchRange;
	BYTES2 entrySelector;
	BYTES2 rangeShift;
	offsetEnt entries[];
} offsetTbl;

typedef struct {
	unsigned long glyf;
	unsigned long head;
} OffsetCache;

static OffsetCache cache_offsets(offsetTbl *offt)
{
	OffsetCache cache = { 0 };
	int count = ru16(offt->numTables);
	int idx = 0, cmp;
	do {
		cmp = strncmp(offt->entries[idx].tag, "glyf", 4);
		if (!cmp) cache.glyf = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	do {
		cmp = strncmp(offt->entries[idx].tag, "head", 4);
		if (!cmp) cache.head = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	return cache;
}

typedef struct {
	BYTES2 numContours;
	BYTES2 xMin;
	BYTES2 yMin;
	BYTES2 xMax;
	BYTES2 yMax;
} ShHdr;

typedef struct {
	int numContours;
	BYTES2 *endPts;
	BYTES1 *flagsPtr;
	BYTES1 *xPtr;
	BYTES1 *yPtr;
} OutlineInfo;

static OutlineInfo gather_outline_info(BYTES1 *glyfEntry)
{
	BYTES1 *glyfCursor = glyfEntry;
	OutlineInfo info = { 0 };

	ShHdr *sh = (ShHdr *) glyfCursor;
	int numContours = ri16(sh->numContours);
	assert(numContours >= 0);
	info.numContours = numContours;
	glyfCursor += 10;

	BYTES2 *endPts = (BYTES2 *) glyfCursor;
	int numPts = numContours == 0 ? 0 : (ru16(endPts[numContours - 1]) + 1);
	info.endPts = endPts;
	glyfCursor += 2 * numContours;

	int instrLength = ri16(*(BYTES2 *) glyfCursor);
	glyfCursor += 2 + instrLength;

	info.flagsPtr = glyfCursor;

	unsigned long xBytes = 0;

	int curPt = 0;
	while (curPt < numPts) {
		uint8_t flag = *(glyfCursor++);
		if (flag & SGF_SHORT_X_COORD)
			xBytes += 1;
		else if (!(flag & SGF_REUSE_PREV_X))
			xBytes += 2;
		if (flag & SGF_REPEAT_FLAG)
			curPt += *(glyfCursor++);
		++curPt;
	}

	info.xPtr = glyfCursor;
	info.yPtr = glyfCursor + xBytes;

	return info;
}

static long pull_coordinate(BYTES1 flags, BYTES1 **ptr, long prev)
{
	long co = prev;
	if (flags & SGF_SHORT_X_COORD) {
		if (flags & SGF_POSITIVE_X) {
			co += **ptr;
			(*ptr) += 1;
		} else {
			co -= **ptr;
			(*ptr) += 1;
		}
	} else if (!(flags & SGF_REUSE_PREV_X)) {
		co += ri16(*(BYTES2 *) *ptr);
		(*ptr) += 2;
	}
	return co;
}

static void raster_outline(OutlineInfo info)
{
	int pointIdx = 0;

	for (int c = 0; c < info.numContours; ++c) {
		printf("~~ Contour #%d ~~\n", c);

		long prev_x = 0, prev_y = 0;

		int endPt = ru16(info.endPts[c]);
		while (pointIdx <= endPt) {
			BYTES1 flags = *(info.flagsPtr++);

			int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(info.flagsPtr++);

			for (int t = 0; t < times; ++t) { // TODO reverse & don't check on first iteration
				long x = pull_coordinate(flags, &info.xPtr, prev_x);
				long y = pull_coordinate(flags >> 1, &info.yPtr, prev_y);
				printf("(%ld, %ld)\n", x, y);
				prev_x = x, prev_y = y;
				++pointIdx;
			}
		}
	}
}

static void print_head(BYTES1 *ptr)
{
	headTbl *head = (headTbl *) ptr;
	int unitsPerEm = ru16(head->unitsPerEm);
	printf("unitsPerEm: %d\n", unitsPerEm);
}

int main(int argc, char const *argv[])
{
	int descr = open("Ubuntu-C.ttf", O_RDONLY);
	assert(descr >= 0);
	struct stat stat;
	int fstat_ret = fstat(descr, &stat);
	assert(fstat_ret == 0);
	BYTES1 *mapped = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, descr, 0);
	assert(mapped != MAP_FAILED);
	close(descr);

	OffsetCache offcache = cache_offsets((offsetTbl *) mapped);

	print_head(mapped + offcache.head);

	OutlineInfo info = gather_outline_info(mapped + offcache.glyf);
	raster_outline(info);

	munmap(mapped, stat.st_size);
	return EXIT_SUCCESS;
}
