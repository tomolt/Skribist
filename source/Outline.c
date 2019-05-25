#include "Internals.h"

#include <limits.h>

void DrawLine(Workspace * restrict ws, Line line);
void DrawCurve(Workspace * restrict ws, Curve initialCurve);
uint32_t CalcRasterWidth(SKR_Dimensions dims);

/*
======== glyph positioning ========
*/

SKR_Status skrGetHorMetrics(SKR_Font const * restrict font,
	Glyph glyph, SKR_HorMetrics * restrict metrics)
{
	if (!(glyph < font->numGlyphs)) return SKR_FAILURE;
	BYTES2 * restrict hmtxAddr = (BYTES2 *) ((BYTES1 *) font->data + font->hmtx.offset);
	if (glyph < font->numberOfHMetrics) {
		BYTES2 * restrict addr = &hmtxAddr[glyph * 2];
		metrics->advanceWidth = (float) ru16(addr[0]) / font->unitsPerEm;
		metrics->leftSideBearing = (float) ri16(addr[1]) / font->unitsPerEm;
		return SKR_SUCCESS;
	} else {
		BYTES2 * restrict addr = &hmtxAddr[(font->numberOfHMetrics - 1) * 2];
		metrics->advanceWidth = (float) ru16(addr[0]) / font->unitsPerEm;
		metrics->leftSideBearing = (float) ri16((addr + 2)[glyph - font->numberOfHMetrics]) / font->unitsPerEm;
		return SKR_SUCCESS;
	}
}

/*
======== character mapping ========
*/

static int FindSegment_Format4(int segCount,
	BYTES2 * restrict startCodes, BYTES2 * restrict endCodes, int charCode)
{
	/*
	TODO upgrade to binary search here.
	Right now this is linear search because there's less stuff that can go wrong with it.
	*/
	SKR_assert(charCode <= USHRT_MAX);
	for (int i = 0; i < segCount; ++i) {
		int endCode = ru16(endCodes[i]);
		if (endCode < charCode) continue;

		int startCode = ru16(startCodes[i]);
		if (startCode > charCode) return -1;

		return i;
	}
	return -1;
}

static Glyph GlyphFromCode_Format4(SKR_Font const * restrict font, int charCode)
{
	SKR_cmap_format4 const * restrict mapping = &font->mapping.format4;
	BYTES2 * startCodes = (BYTES2 *) ((BYTES1 *) font->data + mapping->startCodes);
	BYTES2 * endCodes = (BYTES2 *) ((BYTES1 *) font->data + mapping->endCodes);
	BYTES2 * idDeltas = (BYTES2 *) ((BYTES1 *) font->data + mapping->idDeltas);
	BYTES2 * idRangeOffsets = (BYTES2 *) ((BYTES1 *) font->data + mapping->idRangeOffsets);

	int segment = FindSegment_Format4(mapping->segCount, startCodes, endCodes, charCode);

	if (segment < 0) {
		return 0;
	}

	int startCode = ru16(startCodes[segment]);
	int idDelta = ru16(idDeltas[segment]);
	int idRangeOffset = ru16(idRangeOffsets[segment]);

	if (idRangeOffset == 0) {
		return (uint16_t) (charCode + idDelta);
	}

	SKR_assert(idRangeOffset % 2 == 0);

	BYTES2 * glyphAddr = &idRangeOffsets[segment] + idRangeOffset / 2 + (charCode - startCode);
	Glyph glyph = ru16(*glyphAddr);
	return glyph > 0 ? (uint16_t) (glyph + idDelta) : 0;
}

static Glyph GlyphFromCode_Format6(SKR_Font const * restrict font, unsigned int charCode)
{
	SKR_cmap_format6 const * restrict mapping = &font->mapping.format6;
	if (charCode < mapping->firstCode) return 0;
	unsigned int relCode = charCode - mapping->firstCode;
	if (!(relCode < mapping->entryCount)) return 0;
	BYTES2 * restrict glyphIndexArray =
		(BYTES2 *) ((BYTES1 *) font->data + mapping->glyphIndexArray);
	return ru16(glyphIndexArray[relCode]);
}

Glyph skrGlyphFromCode(SKR_Font const * restrict font, int charCode)
{
	switch (font->mappingFormat) {
	case 4:
		return GlyphFromCode_Format4(font, charCode);
	case 6:
		return GlyphFromCode_Format6(font, charCode);
	default:
		SKR_assert(0);
		return 0;
	}
}

/*
======== outlines ========
*/

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
All information resulting from the
scouting pass.
*/
typedef struct {
	int numContours;
	BYTES2 * endPts;
	BYTES1 * flagsPtr;
	BYTES1 * xPtr;
	BYTES1 * yPtr;
} OutlineIntel;

typedef struct {
	unsigned int state;
	Point queuedStart;
	Point queuedPivot;
	Point looseEnd;

	RasterCell * raster;
	SKR_Dimensions dims;
} ContourFSM;

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

typedef struct {
	BYTES1 * lowerBound, * upperBound;
} MemRange;

static SKR_Status GetOutlineRange(SKR_Font const * restrict font,
	Glyph glyph, MemRange * restrict range)
{
	void const * locaAddr = (BYTES1 *) font->data + font->loca.offset;
	BYTES1 * glyfAddr = (BYTES1 *) font->data + font->glyf.offset;
	int n = font->numGlyphs + 1;
	if (!(glyph < n)) return SKR_FAILURE;
	if (!font->indexToLocFormat) {
		BYTES2 * loca = (BYTES2 *) locaAddr;
		range->lowerBound = glyfAddr + 2 * (unsigned long) ru16(loca[glyph]);
		range->upperBound = glyfAddr + 2 * (unsigned long) ru16(loca[glyph + 1]);
	} else {
		BYTES4 * loca = (BYTES4 *) locaAddr;
		range->lowerBound = glyfAddr + ru32(loca[glyph]);
		range->upperBound = glyfAddr + ru32(loca[glyph + 1]);
	}
	return SKR_SUCCESS;
}

SKR_Status skrGetOutlineBounds(SKR_Font const * restrict font, Glyph glyph,
	SKR_Transform transform, SKR_Bounds * restrict bounds)
{
	/* TODO return empty bounds in case glyf length is zero */
	SKR_Status s;
	MemRange range;
	s = GetOutlineRange(font, glyph, &range);
	if (s) return s;
	if (range.upperBound == range.lowerBound) {
		*bounds = (SKR_Bounds) { 0, 0, 0, 0 }; // TODO get rid of this
		return SKR_SUCCESS;
	}
	if ((unsigned long) (range.upperBound - range.lowerBound) < sizeof(ShHdr)) return SKR_FAILURE;
	ShHdr const * restrict sh = (ShHdr const *) range.lowerBound;

	transform.xScale /= font->unitsPerEm;
	transform.yScale /= font->unitsPerEm;

	// TODO i guess the floor() is not neccessary here.
	bounds->xMin = floorf((ri16(sh->xMin) - 1) * transform.xScale + transform.xMove);
	bounds->yMin = floorf((ri16(sh->yMin) - 1) * transform.yScale + transform.yMove);
	bounds->xMax = ceilf ((ri16(sh->xMax) + 1) * transform.xScale + transform.xMove);
	bounds->yMax = ceilf ((ri16(sh->yMax) + 1) * transform.yScale + transform.yMove);

	return SKR_SUCCESS;
}

/*

By the design of TrueType it's not possible to parse an outline in a single pass.
So instead, we run a 'scouting' phase before the actual loading stage.

*/

SKR_Status ScoutOutline(BYTES1 * outlineAddr, OutlineIntel * restrict intel)
{
	BYTES1 * glyfCursor = outlineAddr;

	ShHdr *sh = (ShHdr *) glyfCursor;
	intel->numContours = ri16(sh->numContours);
	if (intel->numContours < 0) return SKR_FAILURE;
	glyfCursor += 10;

	BYTES2 * endPts = (BYTES2 *) glyfCursor;
	intel->endPts = endPts;
	glyfCursor += 2 * intel->numContours;

	int instrLength = ri16(*(BYTES2 *) glyfCursor);
	glyfCursor += 2 + instrLength;

	intel->flagsPtr = glyfCursor;

	unsigned long xBytes = 0;
	int pointIdx = 0;

	for (int c = 0; c < intel->numContours; ++c) {
		int endPt = ru16(intel->endPts[c]);

		while (pointIdx <= endPt) {
			uint8_t flags = *(glyfCursor++);
			unsigned int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(glyfCursor++);
			if (flags & SGF_SHORT_X_COORD)
				xBytes += times;
			else if (!(flags & SGF_REUSE_PREV_X))
				xBytes += 2 * times;
			pointIdx += times;
		}
	}

	intel->xPtr = glyfCursor;
	intel->yPtr = glyfCursor + xBytes;

	return SKR_SUCCESS;
}

/*

When we find a new point, we can't generally output a new curve for it right away.
Instead, we have to buffer them up until we can construct a valid curve.
Because of the complicated packing scheme used in TrueType, this is somewhat
complicated. ExtendContour() implements this using a simple finite state machine.

*/

static void ExtendContour(ContourFSM * restrict fsm, Point newNode, int onCurve, Workspace * restrict ws)
{
	switch (fsm->state) {
	case 0:
		SKR_assert(onCurve);
		fsm->queuedStart = newNode;
		fsm->looseEnd = newNode;
		fsm->state = 1;
		break;
	case 1:
		if (onCurve) {
			Line line = { fsm->queuedStart, newNode };
			DrawLine(ws, line);
			fsm->queuedStart = newNode;
			break;
		} else {
			fsm->queuedPivot = newNode;
			fsm->state = 2;
			break;
		}
	case 2:
		if (onCurve) {
			Curve curve = { fsm->queuedStart, newNode, fsm->queuedPivot };
			DrawCurve(ws, curve);
			fsm->queuedStart = newNode;
			fsm->state = 1;
			break;
		} else {
			Point implicit = Midpoint(fsm->queuedPivot, newNode);
			Curve curve = { fsm->queuedStart, implicit, fsm->queuedPivot };
			DrawCurve(ws, curve);
			fsm->queuedStart = implicit;
			fsm->queuedPivot = newNode;
			break;
		}
	default: SKR_assert(0);
	}
}

/*

Pulls the next coordinate from the memory pointed to by ptr, and
interprets it according to flags. Technically, GetCoordinateAndAdvance()
is only implemented for the x coordinate, however by right
shifting the relevant flags by 1 we can also use this function for
y coordinates.

*/
static long GetCoordinateAndAdvance(BYTES1 flags, BYTES1 * restrict * restrict ptr, long prev)
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
		co += ri16(*(BYTES2 * restrict) *ptr);
		(*ptr) += 2;
	}
	return co;
}

static void DrawOutlineWithIntel(OutlineIntel * restrict intel,
	SKR_Transform transform, Workspace * restrict ws)
{
	int pointIdx = 0;
	long prevX = 0, prevY = 0;

	ContourFSM fsm = { 0 };

	for (int c = 0; c < intel->numContours; ++c) {
		int endPt = ru16(intel->endPts[c]);

		fsm.state = 0;

		while (pointIdx <= endPt) {
			BYTES1 flags = *(intel->flagsPtr++);

			unsigned int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(intel->flagsPtr++);

			do {
				long x = GetCoordinateAndAdvance(flags, &intel->xPtr, prevX);
				long y = GetCoordinateAndAdvance(flags >> 1, &intel->yPtr, prevY);
				Point point = {
					x * transform.xScale + transform.xMove,
					y * transform.yScale + transform.yMove };
				ExtendContour(&fsm, point, flags & SGF_ON_CURVE_POINT, ws);
				prevX = x, prevY = y;
				++pointIdx;
				--times;
			} while (times > 0);
		}

		// Close the loop - but don't update relative origin point
		ExtendContour(&fsm, fsm.looseEnd, SGF_ON_CURVE_POINT, ws);
	}
}

SKR_Status skrDrawOutline(SKR_Font const * restrict font, Glyph glyph,
	SKR_Transform transform, RasterCell * restrict raster, SKR_Dimensions dims)
{
	SKR_Status s;
	MemRange range;
	s = GetOutlineRange(font, glyph, &range);
	if (s) return s;
	if (range.upperBound == range.lowerBound) return SKR_SUCCESS;
	OutlineIntel intel = { 0 };
	s = ScoutOutline(range.lowerBound, &intel);
	if (s) return s;
	transform.xScale /= font->unitsPerEm;
	transform.yScale /= font->unitsPerEm;
	Workspace ws = { raster, dims, CalcRasterWidth(dims) };
	DrawOutlineWithIntel(&intel, transform, &ws);
	return SKR_SUCCESS;
}
