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

static uint32_t ru32(BYTES2 raw)
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
#define SGF_ON_CURVE_POINT                       0x01
#define SGF_X_SHORT_VECTOR                       0x02
#define SGF_Y_SHORT_VECTOR                       0x04
#define SGF_REPEAT_FLAG                          0x08
#define SGF_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR 0x10
#define SGF_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR 0x20
#define SGF_OVERLAP_SIMPLE                       0x40

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

static void print_glyph_flags(BYTES1 *glyf_ent)
{
	BYTES1 *glyf_cursor = glyf_ent;

	ShHdr *sh = (ShHdr *) glyf_cursor;
	int num_pts = ri16(sh->numContours);
	assert(num_pts >= 0);
	glyf_cursor += 10;

	glyf_cursor += 2 * num_pts;

	int instr_len = ri16(*(BYTES2 *) glyf_cursor);
	glyf_cursor += 2 + instr_len;

	int cur_pt = 0;
	while (cur_pt < num_pts) {
		uint8_t flag = *(glyf_cursor++);
		if (flag & SGF_ON_CURVE_POINT) {
			printf("| on curve point");
		}
		if (flag & SGF_X_SHORT_VECTOR) {
			printf("| X short vector");
		}
		if (flag & SGF_Y_SHORT_VECTOR) {
			printf("| Y short vector");
		}
		if (flag & SGF_REPEAT_FLAG) {
			int add_times = *(glyf_cursor++);
			printf("| repeats %d times", add_times + 1);
			cur_pt += add_times;
		}
		if (flag & SGF_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR) {
			printf("| foo1");
		}
		if (flag & SGF_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR) {
			printf("| foo2");
		}
		if (flag & SGF_OVERLAP_SIMPLE) {
			printf("| simple overlap");
		}
		printf("\n");
		++cur_pt;
	}
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

	printf("%u\n", ri16(0xAFBD));

	OffsetCache offcache = cache_offsets((offsetTbl *) mapped);
	printf("glyf: %lu\n", offcache.glyf);
	printf("head: %lu\n", offcache.head);

	// print_glyph_flags(mapped + offcache.glyf);

	munmap(mapped, stat.st_size);
	return EXIT_SUCCESS;
}
