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

#include "rational.h"
#include "outline.h"
#include "reading.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
	int numCurves;
} OutlineInfo;

static unsigned int olt_GLOBAL_nodeState;
static unsigned int olt_GLOBAL_numCurves;

static void pre_scan_node(int onCurve)
{
	switch (olt_GLOBAL_nodeState) {
	case 0:
		assert(onCurve);
		olt_GLOBAL_nodeState = 1;
		break;
	case 1:
		if (onCurve) {
			++olt_GLOBAL_numCurves;
			break;
		} else {
			olt_GLOBAL_nodeState = 2;
			break;
		}
	case 2:
		if (onCurve) {
			++olt_GLOBAL_numCurves;
			olt_GLOBAL_nodeState = 1;
			break;
		} else {
			++olt_GLOBAL_numCurves;
			break;
		}
	default: assert(0);
	}
}

static OutlineInfo gather_outline_info(BYTES1 *glyfEntry)
{
	BYTES1 *glyfCursor = glyfEntry;
	OutlineInfo info = { 0 };

	ShHdr *sh = (ShHdr *) glyfCursor;
	info.numContours = ri16(sh->numContours);
	assert(info.numContours >= 0);
	glyfCursor += 10;

	BYTES2 *endPts = (BYTES2 *) glyfCursor;
	int numPts = info.numContours == 0 ? 0
		: (ru16(endPts[info.numContours - 1]) + 1);
	info.endPts = endPts;
	glyfCursor += 2 * info.numContours;

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
		unsigned int times = 1;
		if (flag & SGF_REPEAT_FLAG)
			times += *(glyfCursor++);
		for (unsigned int t = 0; t < times; ++t) {
			pre_scan_node(flag & SGF_ON_CURVE_POINT);
		}
		curPt += times;
	}

	info.numCurves = olt_GLOBAL_numCurves;

	info.xPtr = glyfCursor;
	info.yPtr = glyfCursor + xBytes;

	return info;
}

static Point olt_GLOBAL_queuedStart;
static Point olt_GLOBAL_queuedPivot;
static olt_Parse olt_GLOBAL_parse;

static Point interp_points(Point a, Point b)
{
	return (Point) { (a.x + b.x) / 2, (a.y + b.y) / 2 };
}

static void parse_node(Point newNode, int onCurve)
{
	switch (olt_GLOBAL_nodeState) {
	case 0:
		assert(onCurve);
		olt_GLOBAL_queuedStart = newNode;
		olt_GLOBAL_nodeState = 1;
		break;
	case 1:
		if (onCurve) {
			Point pivot = interp_points(olt_GLOBAL_queuedStart, newNode);
			Curve curve = { olt_GLOBAL_queuedStart, pivot, newNode };
			olt_GLOBAL_parse.curves[olt_GLOBAL_parse.numCurves++] = curve;
			olt_GLOBAL_queuedStart = newNode;
			break;
		} else {
			olt_GLOBAL_queuedPivot = newNode;
			olt_GLOBAL_nodeState = 2;
			break;
		}
	case 2:
		if (onCurve) {
			Curve curve = { olt_GLOBAL_queuedStart, olt_GLOBAL_queuedPivot, newNode };
			olt_GLOBAL_parse.curves[olt_GLOBAL_parse.numCurves++] = curve;
			olt_GLOBAL_queuedStart = newNode;
			olt_GLOBAL_nodeState = 1;
			break;
		} else {
			Point implicit = interp_points(olt_GLOBAL_queuedPivot, newNode);
			Curve curve = { olt_GLOBAL_queuedStart, olt_GLOBAL_queuedPivot, implicit };
			olt_GLOBAL_parse.curves[olt_GLOBAL_parse.numCurves++] = curve;
			olt_GLOBAL_queuedStart = implicit;
			olt_GLOBAL_queuedPivot = newNode;
			break;
		}
	default: assert(0);
	}
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

static void parse_outline(OutlineInfo info)
{
	olt_GLOBAL_nodeState = 0;

	int pointIdx = 0;
	long prevX = 0, prevY = 0;

	for (int c = 0; c < info.numContours; ++c) {
		int endPt = ru16(info.endPts[c]);
		while (pointIdx <= endPt) {
			BYTES1 flags = *(info.flagsPtr++);

			unsigned int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(info.flagsPtr++);

			do {
				long x = pull_coordinate(flags, &info.xPtr, prevX);
				long y = pull_coordinate(flags >> 1, &info.yPtr, prevY);
				parse_node((Point) { x, y }, flags & SGF_ON_CURVE_POINT);
				prevX = x, prevY = y;
				++pointIdx;
				--times;
			} while (times > 0);
		}
	}
}

void olt_INTERN_parse_outline(void *addr)
{
	OutlineInfo info = gather_outline_info(addr);
	olt_GLOBAL_parse.curves = calloc(info.numCurves, sizeof(Curve));
	parse_outline(info);

	fprintf(stderr, "pre-scanned number of curves: %u\n", info.numCurves);
	fprintf(stderr, "parsed number of curves (must match up): %u\n", olt_GLOBAL_parse.numCurves);
	assert(info.numCurves == olt_GLOBAL_parse.numCurves);

	for (int i = 0; i < olt_GLOBAL_parse.numCurves; ++i) {
		Curve curve = olt_GLOBAL_parse.curves[i];
		fprintf(stderr, "(%f, %f) -> (%f, %f) -> (%f, %f)\n",
			curve.beg.x, curve.beg.y, curve.ctrl.x, curve.ctrl.y, curve.end.x, curve.end.y);
	}
}
