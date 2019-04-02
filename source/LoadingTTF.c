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
BYTES1 const * skrGetOutlineAddr(SKR_Font const * font, Glyph glyph)
{
	void const * locaAddr = (BYTES1 *) font->data + font->loca.offset;
	int n = font->numGlyphs + 1;
	SKR_assert(glyph < n);
	unsigned long offset;
	if (!font->indexToLocFormat) {
		BYTES2 * loca = (BYTES2 *) locaAddr;
		offset = 2 * (unsigned long) ru16(loca[glyph]);
	} else {
		BYTES4 * loca = (BYTES4 *) locaAddr;
		offset = ru32(loca[glyph]);
	}
	return (BYTES1 *) font->data + font->glyf.offset + offset;
}

SKR_Rect skrGetOutlineBounds(BYTES1 * glyfEntry)
{
	ShHdr const * sh = (ShHdr const *) glyfEntry;
	return (SKR_Rect) {
		ri16(sh->xMin), ri16(sh->yMin),
		ri16(sh->xMax), ri16(sh->yMax) };
}

/*

By the design of TrueType it's not possible to parse an outline in a single pass.
So instead, we run an 'explore' phase before the actual parsing stage.

*/

typedef struct {
	unsigned int state, neededSpace;
} ExploringFSM;

static void ExtendContourWhilstExploring(ExploringFSM * fsm, int onCurve)
{
	switch (fsm->state) {
	case 0:
		SKR_assert(onCurve);
		fsm->state = 1;
		break;
	case 1:
		if (onCurve) {
			++fsm->neededSpace;
			break;
		} else {
			fsm->state = 2;
			break;
		}
	case 2:
		if (onCurve) {
			++fsm->neededSpace;
			fsm->state = 1;
			break;
		} else {
			++fsm->neededSpace;
			break;
		}
	default: SKR_assert(0);
	}
}

void skrExploreOutline(BYTES1 * glyfEntry, ParsingClue * destination)
{
	BYTES1 * glyfCursor = glyfEntry;
	ParsingClue clue = { 0 };

	ShHdr *sh = (ShHdr *) glyfCursor;
	clue.numContours = ri16(sh->numContours);
	SKR_assert(clue.numContours >= 0);
	glyfCursor += 10;

	BYTES2 * endPts = (BYTES2 *) glyfCursor;
	clue.endPts = endPts;
	glyfCursor += 2 * clue.numContours;

	int instrLength = ri16(*(BYTES2 *) glyfCursor);
	glyfCursor += 2 + instrLength;

	clue.flagsPtr = glyfCursor;

	unsigned long xBytes = 0;
	int pointIdx = 0;

	ExploringFSM fsm = { 0, 0 };

	for (int c = 0; c < clue.numContours; ++c) {
		int endPt = ru16(clue.endPts[c]);

		fsm.state = 0;

		while (pointIdx <= endPt) {
			uint8_t flags = *(glyfCursor++);
			unsigned int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(glyfCursor++);
			if (flags & SGF_SHORT_X_COORD)
				xBytes += times;
			else if (!(flags & SGF_REUSE_PREV_X))
				xBytes += 2 * times;
			for (unsigned int t = 0; t < times; ++t) {
				ExtendContourWhilstExploring(&fsm, flags & SGF_ON_CURVE_POINT);
			}
			pointIdx += times;
		}

		// Close the loop
		ExtendContourWhilstExploring(&fsm, SGF_ON_CURVE_POINT);
	}

	clue.neededSpace = fsm.neededSpace;

	clue.xPtr = glyfCursor;
	clue.yPtr = glyfCursor + xBytes;

	*destination = clue;
}

/*

When we find a new point, we can't generally output a new curve for it right away.
Instead, we have to buffer them up until we can construct a valid curve.
Because of the complicated packing scheme used in TrueType, this is somewhat
complicated. ExtendContourWhilstExploring() implements this using a simple finite state machine.

*/

typedef struct {
	unsigned int state;
	Point queuedStart;
	Point queuedPivot;
	Point looseEnd;
	CurveBuffer * dest;
} ParsingFSM;

static void ExtendContourWhilstParsing(ParsingFSM * fsm, Point newNode, int onCurve)
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
			Point pivot = Midpoint(fsm->queuedStart, newNode);
			Curve curve = { fsm->queuedStart, pivot, newNode };
			fsm->dest->elems[fsm->dest->count++] = curve;
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
			fsm->dest->elems[fsm->dest->count++] = curve;
			fsm->queuedStart = newNode;
			fsm->state = 1;
			break;
		} else {
			Point implicit = Midpoint(fsm->queuedPivot, newNode);
			Curve curve = { fsm->queuedStart, fsm->queuedPivot, implicit };
			fsm->dest->elems[fsm->dest->count++] = curve;
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

void skrParseOutline(ParsingClue * clue, CurveBuffer * destination)
{
	int pointIdx = 0;
	long prevX = 0, prevY = 0;

	ParsingFSM fsm = { 0, { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, destination };

	for (int c = 0; c < clue->numContours; ++c) {
		int endPt = ru16(clue->endPts[c]);

		fsm.state = 0;

		while (pointIdx <= endPt) {
			BYTES1 flags = *(clue->flagsPtr++);

			unsigned int times = 1;
			if (flags & SGF_REPEAT_FLAG)
				times += *(clue->flagsPtr++);

			do {
				long x = GetCoordinateAndAdvance(flags, &clue->xPtr, prevX);
				long y = GetCoordinateAndAdvance(flags >> 1, &clue->yPtr, prevY);
				ExtendContourWhilstParsing(&fsm, (Point) { x, y }, flags & SGF_ON_CURVE_POINT);
				prevX = x, prevY = y;
				++pointIdx;
				--times;
			} while (times > 0);
		}

		// Close the loop - but don't update relative origin point
		ExtendContourWhilstParsing(&fsm, fsm.looseEnd, SGF_ON_CURVE_POINT);
	}
}
