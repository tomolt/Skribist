#ifndef SKRIBIST_H
#define SKRIBIST_H

// depends on stdint.h

typedef long Glyph;

typedef struct {
	unsigned long glyf;
	unsigned long head;
	unsigned long loca;
	unsigned long maxp;
} OffsetCache;

extern short olt_GLOBAL_unitsPerEm;
extern short olt_GLOBAL_indexToLocFormat;
extern short olt_GLOBAL_numGlyphs;

OffsetCache olt_INTERN_cache_offsets(void *addr);
void olt_INTERN_parse_head(void *addr);
void olt_INTERN_parse_maxp(void *addr);
unsigned long olt_INTERN_get_outline(void *locaAddr, Glyph glyph);

// Please update glossary when messing with units.
typedef struct {
	double x;
	double y;
} Point;

typedef struct {
	Point beg;
	Point end;
} Line;

typedef struct {
	Point beg;
	Point ctrl;
	Point end;
} Curve;

void olt_INTERN_parse_outline(void *addr);

#define WIDTH 256
#define HEIGHT 256

/*
The transformation order goes: first scale, then move.
*/
typedef struct {
	Point scale, move;
} Transform;

typedef struct {
	int space, count;
	Curve *elems;
} CurveBuffer;

typedef struct {
	int space, count;
	Line *elems;
} LineBuffer;

typedef struct {
	int8_t windingAndCover; // in the range -127 - 127
	uint8_t area; // in the range 0 - 254
} RasterCell;

#if 0
typedef struct {
	SkrFormat format;
	long width;
	long height;
	long stride;
	void *data;
} SkrImage;
#endif

/*
All information resulting from the
exploration pass.
*/
typedef struct {
	int numContours;
	BYTES2 * endPts;
	BYTES1 * flagsPtr;
	BYTES1 * xPtr;
	BYTES1 * yPtr;
	int neededSpace;
} ParsingClue;

extern RasterCell olt_GLOBAL_raster[WIDTH * HEIGHT];
extern uint8_t olt_GLOBAL_image[WIDTH * HEIGHT];

void skrExploreOutline(BYTES1 * glyfEntry, ParsingClue * destination);
void skrParseOutline(ParsingClue * clue, CurveBuffer * destination);

void skrBeginTesselating(CurveBuffer const *source,
	Transform transform, CurveBuffer *stack);
void skrContinueTesselating(CurveBuffer *stack,
	double flatness, LineBuffer *dest);

void skrRasterizeLines(LineBuffer const *source);

void olt_INTERN_gather(void);

#endif
