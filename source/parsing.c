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
		assert(onCurve);
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
	default: assert(0);
	}
}

void skrExploreOutline(BYTES1 * glyfEntry, ParsingClue * destination)
{
	BYTES1 *glyfCursor = glyfEntry;
	ParsingClue clue = { 0 };

	ShHdr *sh = (ShHdr *) glyfCursor;
	clue.numContours = ri16(sh->numContours);
	assert(clue.numContours >= 0);
	glyfCursor += 10;

	BYTES2 *endPts = (BYTES2 *) glyfCursor;
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
		assert(onCurve);
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
	default: assert(0);
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
