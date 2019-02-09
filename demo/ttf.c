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

#include <stdint.h>

#include "mapping.h"
#include "header.h"
#include "reading.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

typedef struct {
	long x, y;
} Node;

static void print_line(Node start, Node end)
{
	printf("(%ld, %ld) -> (%ld, %ld)\n", start.x, start.y, end.x, end.y);
}

static void print_bezier(Node start, Node pivot, Node end)
{
	printf("(%ld, %ld) -> (%ld, %ld) -> (%ld, %ld)\n",
		start.x, start.y, pivot.x, pivot.y, end.x, end.y);
}

static int olt_GLOBAL_nodeState;
static Node olt_GLOBAL_queuedStart;
static Node olt_GLOBAL_queuedPivot;

static Node interp_nodes(Node a, Node b)
{
	return (Node) { (a.x + b.x) / 2, (a.y + b.y) / 2 };
}

static void raster_node(Node newNode, int onCurve)
{
	switch (olt_GLOBAL_nodeState) {
	case 0:
		assert(onCurve);
		olt_GLOBAL_queuedStart = newNode;
		olt_GLOBAL_nodeState = 1;
		break;
	case 1:
		if (onCurve) {
			print_line(olt_GLOBAL_queuedStart, newNode);
			olt_GLOBAL_queuedStart = newNode;
			break;
		} else {
			olt_GLOBAL_queuedPivot = newNode;
			olt_GLOBAL_nodeState = 2;
			break;
		}
	case 2:
		if (onCurve) {
			print_bezier(olt_GLOBAL_queuedStart, olt_GLOBAL_queuedPivot, newNode);
			olt_GLOBAL_queuedStart = newNode;
			olt_GLOBAL_nodeState = 1;
			break;
		} else {
			Node implicit = interp_nodes(olt_GLOBAL_queuedPivot, newNode);
			print_bezier(olt_GLOBAL_queuedStart, olt_GLOBAL_queuedPivot, implicit);
			olt_GLOBAL_queuedStart = implicit;
			olt_GLOBAL_queuedPivot = newNode;
			break;
		}
	default: assert(0);
	}
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
				raster_node((Node) { x, y }, flags & SGF_ON_CURVE_POINT);
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
	(void) argc, (void) argv;

	int ret;

	BYTES1 *rawData;
	mapping_handle_t mapping;
	ret = olt_INTERN_map_file("Ubuntu-C.ttf", (void **) &rawData, &mapping);
	assert(ret == 0);

	OffsetCache offcache = olt_INTERN_cache_offsets(rawData);

	print_head(rawData + offcache.head);

	OutlineInfo info = gather_outline_info(rawData + offcache.glyf);
	raster_outline(info);

	ret = olt_INTERN_unmap_file((void *) rawData, mapping);
	assert(ret == 0);
	return EXIT_SUCCESS;
}
