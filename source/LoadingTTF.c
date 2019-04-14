typedef struct {
	char tag[4];
	BYTES4 checksum;
	BYTES4 offset;
	BYTES4 length;
} TTF_OffsetEntry;

typedef struct {
	BYTES4 scalerType;
	BYTES2 numTables;
	BYTES2 searchRange;
	BYTES2 entrySelector;
	BYTES2 rangeShift;
	TTF_OffsetEntry entries[];
} TFF_OffsetTable;

static SKR_Status ScanForOffsetEntry(
	TFF_OffsetTable const * restrict offt,
	int * restrict cur,
	char tag[4],
	SKR_TTF_Table * restrict table)
{
	int count = ru16(offt->numTables);
	for (;;) {
		int cmp = CompareStrings(offt->entries[*cur].tag, tag, 4);
		if (!cmp) {
			table->offset = ru32(offt->entries[*cur].offset);
			table->length = ru32(offt->entries[*cur].length);
			return SKR_SUCCESS;
		} else if (cmp > 0) {
			return SKR_FAILURE;
		} else if (!(*cur < count)) {
			return SKR_FAILURE;
		}
		++*cur;
	}
}

static SKR_Status ExtractOffsets(SKR_Font * font)
{
	SKR_Status s = SKR_SUCCESS;
	TFF_OffsetTable const * offt = (TFF_OffsetTable const *) font->data;
	int cur = 0;
	s = ScanForOffsetEntry(offt, &cur, "cmap", &font->cmap);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "glyf", &font->glyf);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "head", &font->head);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "loca", &font->loca);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "maxp", &font->maxp);
	return s;
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
} TTF_head;

static SKR_Status Parse_head(SKR_Font * font)
{
	void const * addr = (BYTES1 *) font->data + font->head.offset;
	TTF_head *head = (TTF_head *) addr;
	font->unitsPerEm = ru16(head->unitsPerEm);
	font->indexToLocFormat = ri16(head->indexToLocFormat);
	return (font->indexToLocFormat == 0 || font->indexToLocFormat == 1) ?
		SKR_SUCCESS : SKR_FAILURE;
}

typedef struct {
	BYTES4 version;
	BYTES2 numGlyphs;
	BYTES2 maxPoints;
	BYTES2 maxContours;
	BYTES2 maxCompositePoints;
	BYTES2 maxCompositeContours;
	BYTES2 maxZones;
	BYTES2 maxTwilightPoints;
	BYTES2 maxStorage;
	BYTES2 maxFunctionDefs;
	BYTES2 maxInstructionDefs;
	BYTES2 maxStackElements;
	BYTES2 maxSizeofInstructions;
	BYTES2 maxComponentElements;
	BYTES2 maxComponentDepth;
} TTF_maxp;

static SKR_Status Parse_maxp(SKR_Font * font)
{
	void const * addr = (BYTES1 *) font->data + font->maxp.offset;
	TTF_maxp *maxp = (TTF_maxp *) addr;
	if (ru32(maxp->version) != 0x00010000)
		return SKR_FAILURE;
	font->numGlyphs = ru16(maxp->numGlyphs);
	return SKR_SUCCESS;
}

SKR_Status skrInitializeFont(SKR_Font * font)
{
	SKR_Status s = SKR_SUCCESS;
	s = ExtractOffsets(font);
	if (s) return s;
	s = Parse_head(font);
	if (s) return s;
	s = Parse_maxp(font);
	return s;
}

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

// TODO GetOutlineRange instead
static SKR_Status GetOutlineAddr(SKR_Font const * font, Glyph glyph, BYTES1 ** addr)
{
	void const * locaAddr = (BYTES1 *) font->data + font->loca.offset;
	int n = font->numGlyphs + 1;
	if (!(glyph < n)) return SKR_FAILURE;
	unsigned long offset;
	if (!font->indexToLocFormat) {
		BYTES2 * loca = (BYTES2 *) locaAddr;
		offset = 2 * (unsigned long) ru16(loca[glyph]);
	} else {
		BYTES4 * loca = (BYTES4 *) locaAddr;
		offset = ru32(loca[glyph]);
	}
	*addr = (BYTES1 *) font->data + font->glyf.offset + offset;
	return SKR_SUCCESS;
}

SKR_Status skrGetOutlineBounds(SKR_Font const * font, Glyph glyph,
	SKR_Transform transform, SKR_Bounds * bounds)
{
	SKR_Status s;
	BYTES1 * outlineAddr;
	s = GetOutlineAddr(font, glyph, &outlineAddr);
	if (s) return s;
	ShHdr const * sh = (ShHdr const *) outlineAddr;

	transform.xScale /= font->unitsPerEm;
	transform.yScale /= font->unitsPerEm;

	// TODO i guess the floor() is not neccessary here.
	bounds->xMin = floor((double) (ri16(sh->xMin) - 1) * transform.xScale + transform.xMove);
	bounds->yMin = floor((double) (ri16(sh->yMin) - 1) * transform.yScale + transform.yMove);
	bounds->xMax = ceil ((double) (ri16(sh->xMax) + 1) * transform.xScale + transform.xMove);
	bounds->yMax = ceil ((double) (ri16(sh->yMax) + 1) * transform.yScale + transform.yMove);

	return SKR_SUCCESS;
}

/*

By the design of TrueType it's not possible to parse an outline in a single pass.
So instead, we run a 'scouting' phase before the actual loading stage.

*/

SKR_Status ScoutOutline(BYTES1 * outlineAddr, OutlineIntel * intel)
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

static void ExtendContour(ContourFSM * fsm, Point newNode, int onCurve)
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
			DrawLine(line, fsm->raster, fsm->dims);
			fsm->queuedStart = newNode;
			break;
		} else {
			fsm->queuedPivot = newNode;
			fsm->state = 2;
			break;
		}
	case 2:
		if (onCurve) {
			Curve curve = { fsm->queuedStart, fsm->queuedPivot, newNode };
			DrawCurve(curve, fsm->raster, fsm->dims);
			fsm->queuedStart = newNode;
			fsm->state = 1;
			break;
		} else {
			Point implicit = Midpoint(fsm->queuedPivot, newNode);
			Curve curve = { fsm->queuedStart, fsm->queuedPivot, implicit };
			DrawCurve(curve, fsm->raster, fsm->dims);
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
static long GetCoordinateAndAdvance(BYTES1 flags, BYTES1 ** ptr, long prev)
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

static void DrawOutlineWithIntel(OutlineIntel * intel,
	SKR_Transform transform, RasterCell * raster, SKR_Dimensions dims)
{
	int pointIdx = 0;
	long prevX = 0, prevY = 0;

	ContourFSM fsm = { 0 };
	fsm.raster = raster;
	fsm.dims = dims;

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
				ExtendContour(&fsm, point, flags & SGF_ON_CURVE_POINT);
				prevX = x, prevY = y;
				++pointIdx;
				--times;
			} while (times > 0);
		}

		// Close the loop - but don't update relative origin point
		ExtendContour(&fsm, fsm.looseEnd, SGF_ON_CURVE_POINT);
	}
}

SKR_Status skrDrawOutline(SKR_Font const * font, Glyph glyph,
	SKR_Transform transform, RasterCell * raster, SKR_Dimensions dims)
{
	SKR_Status s;
	BYTES1 * outlineAddr;
	s = GetOutlineAddr(font, glyph, &outlineAddr);
	if (s) return s;
	OutlineIntel intel = { 0 };
	s = ScoutOutline(outlineAddr, &intel);
	if (s) return s;
	transform.xScale /= font->unitsPerEm;
	transform.yScale /= font->unitsPerEm;
	DrawOutlineWithIntel(&intel, transform, raster, dims);
	return SKR_SUCCESS;
}

// -------- cmap table --------

typedef struct {
	BYTES2 platformID;
	BYTES2 encodingID;
	BYTES4 offset;
} TTF_EncodingRecord;

typedef struct {
	BYTES2 version;
	BYTES2 numTables;
	TTF_EncodingRecord encodingRecords[];
} TTF_cmap;

typedef struct {
	BYTES2 format;
	BYTES2 length;
	BYTES2 language;
} TTF_SharedFormatHeader;

typedef struct {
	BYTES2 format;
	BYTES2 length;
	BYTES2 language;
	BYTES2 segCountX2;
	BYTES2 searchRange;
	BYTES2 entrySelector;
	BYTES2 rangeShift;
} TTF_Format4;

#include <stdio.h> // TEMP

static void InterpretFormat4(TTF_Format4 const * mapping)
{
	int segCount = ru16(mapping->segCountX2) / 2;
	BYTES2 * baseAddr = (BYTES2 *) mapping + 7;
	BYTES2 * endCodes = baseAddr;
	BYTES2 * startCodes = baseAddr + 1 + segCount;
	BYTES2 * idDeltas = baseAddr + 1 + segCount * 2;
	BYTES2 * idRangeOffsets = baseAddr + 1 + segCount * 3;
	BYTES2 * glyphIndexArray = baseAddr + 1 + segCount * 4;
	for (int i = 0; i < segCount; ++i) {
		int startCode = ru16(startCodes[i]);
		int endCode = ru16(endCodes[i]);
		int idDelta = ru16(idDeltas[i]);
		int idRangeOffset = ru16(idRangeOffsets[i]);
		printf("[%u, %u]: d = %u, o = %u\n",
			startCode, endCode, idDelta, idRangeOffset);
	}
}

/*
lower is better, INT_MAX means ignore completely.
*/
static int GetEncodingPriority(TTF_EncodingRecord const * record)
{
	int platformID = ru16(record->platformID);
	int encodingID = ru16(record->encodingID);
	if (platformID == 0) { // Unicode
		switch (encodingID) {
		case 0: return 105;
		case 1: return 104;
		case 2: return 103;
		case 3: return 102;
		case 4: return 101;
		default: return INT_MAX;
		}
	} else if (platformID == 3) { // Windows
		switch (encodingID) {
		case 0: return 203;
		case 1: return 202;
		case 10: return 201;
		default: return INT_MAX;
		}
	} else {
		return INT_MAX;
	}
}

static int IsSupportedFormat(BYTES1 * cmapAddr, TTF_EncodingRecord const * record)
{
	/*
	Wanted Formats: 4, 6, 12
	*/
	BYTES1 * addr = cmapAddr + ru32(record->offset);
	int format = ru16(*(BYTES2 *) addr);
	printf("Got format %d.\n", format);
	switch (format) {
	case 4: return 1;
	default: return 0;
	}
}

SKR_Status skrLoadCMap(SKR_Font const * font)
{
	BYTES1 * cmapAddr = (BYTES1 *) font->data + font->cmap.offset;
	TTF_cmap * cmap = (TTF_cmap *) cmapAddr;

	uint16_t version = ru16(cmap->version);
	if (version != 0) return SKR_FAILURE;

	TTF_EncodingRecord const * record;
	int bestPriority = INT_MAX;
	int numTables = ru16(cmap->numTables);
	for (int i = 0; i < numTables; ++i) {
		TTF_EncodingRecord const * contender = &cmap->encodingRecords[i];
		int priority = GetEncodingPriority(contender);

		if (priority >= bestPriority) continue;
		if (!IsSupportedFormat(cmapAddr, contender)) continue;

		record = contender;
		bestPriority = priority;
	}
	if (bestPriority == INT_MAX) return SKR_FAILURE;

	TTF_Format4 const * format = (TTF_Format4 const *) (cmapAddr + ru32(record->offset));
	InterpretFormat4(format);

	return SKR_SUCCESS;
}
