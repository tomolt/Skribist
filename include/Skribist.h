#ifndef SKRIBIST_H
#define SKRIBIST_H

// depends on stdint.h

/*
The layout of SKR_Status aligns with posix return value conventions,
so you can compare it to 0 just as you would with any other posix call.
*/
typedef enum {
	SKR_FAILURE = -1,
	SKR_SUCCESS
} SKR_Status;

typedef long Glyph;

typedef struct {
	unsigned long offset;
	unsigned long length;
} SKR_TTF_Table;

typedef struct {
	void const * data;
	unsigned long length;
	SKR_TTF_Table glyf, head, loca, maxp;
	short unitsPerEm, indexToLocFormat, numGlyphs;
} SKR_Font;

SKR_Status skrInitializeFont(SKR_Font * font);
unsigned long olt_INTERN_get_outline(SKR_Font const * font, Glyph glyph);

typedef struct {
	double x, y;
} Point;

typedef struct {
	double xMin, yMin, xMax, yMax;
} SKR_Rect;

typedef struct {
	Point beg, end;
} Line;

typedef struct {
	Point beg, ctrl, end;
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

typedef struct {
	long width, height;
	RasterCell * data;
} SKR_Raster;

typedef struct {
	// TODO format
	long width, height, stride;
	unsigned char * data;
} SKR_Image;

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

SKR_Rect skrGetOutlineBounds(BYTES1 const * glyfEntry);
void skrExploreOutline(BYTES1 * glyfEntry, ParsingClue * destination);
void skrParseOutline(ParsingClue * clue, CurveBuffer * destination);

void skrBeginTesselating(CurveBuffer const *source,
	Transform transform, CurveBuffer *stack);
void skrContinueTesselating(CurveBuffer *stack,
	double flatness, LineBuffer *dest);

void skrRasterizeLines(
	LineBuffer const * source,
	SKR_Raster dest);

void olt_INTERN_gather(SKR_Raster raster, SKR_Image image);

#endif
