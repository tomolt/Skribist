#include <stdint.h>

#include "outline.h"
#include "reading.h"

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

/*
This header is shared by the simple
and compound glyph outlines.
*/
typedef struct {
	BYTES2 numContours;
	BYTES2 xMin;
	BYTES2 yMin;
	BYTES2 xMax;
	BYTES2 yMax;
} ShHdr;

/*
All information resulting from the
pre-scan pass.
*/
typedef struct {
	int numContours;
	BYTES2 *endPts;
	BYTES1 *flagsPtr;
	BYTES1 *xPtr;
	BYTES1 *yPtr;
	int numCurves;
} OutlineInfo;

olt_Parse olt_GLOBAL_parse;

static unsigned int olt_GLOBAL_nodeState;
static unsigned int olt_GLOBAL_numCurves;

/*

By the design of TrueType it's not possible to parse an outline in a single pass.
So instead, we run a 'pre-scan' before the actual parsing stage.

*/

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

static OutlineInfo pre_scan_outline(BYTES1 *glyfEntry)
{
	BYTES1 *glyfCursor = glyfEntry;
	OutlineInfo info = { 0 };

	ShHdr *sh = (ShHdr *) glyfCursor;
	info.numContours = ri16(sh->numContours);
	assert(info.numContours >= 0);
	glyfCursor += 10;

	BYTES2 *endPts = (BYTES2 *) glyfCursor;
	info.endPts = endPts;
	glyfCursor += 2 * info.numContours;

	int instrLength = ri16(*(BYTES2 *) glyfCursor);
	glyfCursor += 2 + instrLength;

	info.flagsPtr = glyfCursor;

	unsigned long xBytes = 0;
	int pointIdx = 0;

	for (int c = 0; c < info.numContours; ++c) {
		int endPt = ru16(info.endPts[c]);

		olt_GLOBAL_nodeState = 0;

		while (pointIdx <= endPt) {
			uint8_t flags = *(glyfCursor++);
			if (flags & SGF_SHORT_X_COORD)
				xBytes += 1;
			else if (!(flags & SGF_REUSE_PREV_X))
				xBytes += 2;
			unsigned int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(glyfCursor++);
			for (unsigned int t = 0; t < times; ++t) {
				pre_scan_node(flags & SGF_ON_CURVE_POINT);
			}
			pointIdx += times;
		}
	}

	info.numCurves = olt_GLOBAL_numCurves;

	info.xPtr = glyfCursor;
	info.yPtr = glyfCursor + xBytes;

	return info;
}

static Point interp_points(Point a, Point b)
{
	return (Point) { (a.x + b.x) / 2, (a.y + b.y) / 2 };
}

/*

When we find a new point, we can't generally output a new curve for it right away.
Instead, we have to buffer them up until we can construct a valid curve.
Because of the complicated packing scheme used in TrueType, this is somewhat
complicated. push_point() implements this using a simple finite state machine.

*/

static Point olt_GLOBAL_queuedStart;
static Point olt_GLOBAL_queuedPivot;

static void push_point(Point newNode, int onCurve)
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

/*

Pulls the next coordinate from the memory pointed to by ptr, and
interprets it according to flags. Technically, pull_coordinate()
is only implemented for the x coordinate, however by right
shifting the relevant flags by 1 we can also use this function for
y coordinates.

*/
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
	int pointIdx = 0;
	long prevX = 0, prevY = 0;

	for (int c = 0; c < info.numContours; ++c) {
		int endPt = ru16(info.endPts[c]);

		olt_GLOBAL_nodeState = 0;

		while (pointIdx <= endPt) {
			BYTES1 flags = *(info.flagsPtr++);

			unsigned int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(info.flagsPtr++);

			do {
				long x = pull_coordinate(flags, &info.xPtr, prevX);
				long y = pull_coordinate(flags >> 1, &info.yPtr, prevY);
				push_point((Point) { x, y }, flags & SGF_ON_CURVE_POINT);
				prevX = x, prevY = y;
				++pointIdx;
				--times;
			} while (times > 0);
		}
	}
}

void olt_INTERN_parse_outline(void *addr)
{
	OutlineInfo info = pre_scan_outline(addr);
	olt_GLOBAL_parse.curves = calloc(info.numCurves, sizeof(Curve));
	parse_outline(info);

	assert(info.numCurves == olt_GLOBAL_parse.numCurves);
}
