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
	SKR_TTF_Table cmap, glyf, head, loca, maxp;
	short unitsPerEm, indexToLocFormat, numGlyphs;
} SKR_Font;

SKR_Status skrInitializeFont(SKR_Font * font);

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
	long edgeValue, tailValue;
} RasterCell;

typedef struct {
	long width, height;
} SKR_Dimensions;

#if 0
typedef struct {
	// SKR_Format format;
	long stride;
} SKR_ImageInfo;
#endif

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

	Transform transform;
	RasterCell * raster;
	SKR_Dimensions dims;
} ContourFSM;

#if 0
typedef struct {
	BYTES1 * outlineAddr;
	OutlineIntel outlineIntel;
	ContourFSM contourFsm;
	Transform transform;
	SKR_Dimensions dimensions;
	RasterCell * raster;
} DrawingArgs;
#endif

void skrInitializeLibrary(void);

BYTES1 * skrGetOutlineAddr(SKR_Font const * font, Glyph glyph);
SKR_Rect skrGetOutlineBounds(BYTES1 * glyfEntry);
SKR_Status skrDrawOutline(SKR_Font const * font, Glyph glyph,
	Transform transform, RasterCell * raster, SKR_Dimensions dims);
SKR_Status skrLoadCMap(BYTES1 * addr);

void skrBeginTesselating(CurveBuffer const *source,
	Transform transform, CurveBuffer *stack);
void skrContinueTesselating(CurveBuffer *stack,
	double flatness, LineBuffer *dest);

void skrCastImage(
	RasterCell const * restrict source,
	unsigned char * restrict dest,
	SKR_Dimensions dim);

#endif
