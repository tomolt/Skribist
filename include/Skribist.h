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
	unsigned long endCodes, startCodes, idDeltas, idRangeOffsets;
	int segCount;
} SKR_cmap_format4;

typedef struct {
	void const * data;
	unsigned long length;

	SKR_TTF_Table cmap, glyf, head, hhea, hmtx, loca, maxp;

	short unitsPerEm, indexToLocFormat, numGlyphs;

	short mappingFormat;
	union {
		SKR_cmap_format4 format4;
	} mapping;

	short lineGap;
	unsigned short numberOfHMetrics;
} SKR_Font;

typedef struct {
	unsigned short advanceWidth;
	short leftSideBearing;
} SKR_HorMetrics;

typedef struct {
	uint32_t width, height;
} SKR_Dimensions;

/*
The transformation order goes: first scale, then move.
*/
typedef struct {
	float xScale, yScale, xMove, yMove;
} SKR_Transform;

typedef struct {
	long xMin, yMin, xMax, yMax;
} SKR_Bounds;

typedef struct {
	short edgeValues[8];
	short tailValues[8];
} RasterCell;

void skrInitializeLibrary(void);
SKR_Status skrInitializeFont(SKR_Font * restrict font);

Glyph skrGlyphFromCode(SKR_Font const * restrict font, int charCode);

SKR_Status skrGetHorMetrics(SKR_Font const * restrict font,
	Glyph glyph, SKR_HorMetrics * restrict metrics);

SKR_Status skrGetOutlineBounds(SKR_Font const * restrict font, Glyph glyph,
	SKR_Transform transform, SKR_Bounds * restrict bounds);
SKR_Status skrDrawOutline(SKR_Font const * restrict font, Glyph glyph,
	SKR_Transform transform, RasterCell * restrict raster, SKR_Dimensions dims);

unsigned long skrCalcCellCount(SKR_Dimensions dims);
void skrCastImage(
	RasterCell const * restrict source,
	unsigned char * restrict dest,
	SKR_Dimensions dim);

#endif
