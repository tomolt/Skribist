#include "Internals.h"

#include <limits.h>

/*
======== initialization ========
*/

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

static SKR_Status ExtractOffsets(SKR_Font * restrict font)
{
	SKR_Status s = SKR_SUCCESS;
	TFF_OffsetTable const * restrict offt =
		(TFF_OffsetTable const *) font->data;
	int cur = 0;
	s = ScanForOffsetEntry(offt, &cur, "cmap", &font->cmap);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "glyf", &font->glyf);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "head", &font->head);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "hhea", &font->hhea);
	if (s) return s;
	s = ScanForOffsetEntry(offt, &cur, "hmtx", &font->hmtx);
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
	BYTES2 created[4];
	BYTES2 modified[4];
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

static SKR_Status Parse_head(SKR_Font * restrict font)
{
	TTF_head * restrict head = (TTF_head *)
		((BYTES1 *) font->data + font->head.offset);
	font->unitsPerEm = ru16(head->unitsPerEm);
	font->indexToLocFormat = ri16(head->indexToLocFormat);
	return (font->indexToLocFormat == 0 || font->indexToLocFormat == 1) ?
		SKR_SUCCESS : SKR_FAILURE;
}

typedef struct {
	BYTES4 version;
	BYTES2 ascender;
	BYTES2 descender;
	BYTES2 lineGap;
	BYTES2 advanceWidthMax;
	BYTES2 minLeftSideBearing;
	BYTES2 minRightSideBearing;
	BYTES2 xMaxExtent;
	BYTES2 caretSlopeRise;
	BYTES2 caretSlopeRun;
	BYTES2 caretOffset;
	BYTES2 reserved1, reserved2, reserved3, reserved4;
	BYTES2 metricDataFormat;
	BYTES2 numberOfHMetrics;
} TTF_hhea;

static SKR_Status Parse_hhea(SKR_Font * restrict font)
{
	TTF_hhea const * restrict hhea = (TTF_hhea const *)
		((BYTES1 *) font->data + font->hhea.offset);
	font->lineGap = ri16(hhea->lineGap);
	font->numberOfHMetrics = ru16(hhea->numberOfHMetrics);
	return SKR_SUCCESS;
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

static SKR_Status Parse_maxp(SKR_Font * restrict font)
{
	TTF_maxp const * restrict maxp = (TTF_maxp const *)
		((BYTES1 *) font->data + font->maxp.offset);
	if (ru32(maxp->version) != 0x00010000)
		return SKR_FAILURE;
	font->numGlyphs = ru16(maxp->numGlyphs);
	return SKR_SUCCESS;
}

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
} TTF_cmap_format4;

typedef struct {
	BYTES2 format;
	BYTES2 length;
	BYTES2 language;
	BYTES2 firstCode;
	BYTES2 entryCount;
} TTF_cmap_format6;

/*
lower is better, INT_MAX means ignore completely.
*/
static int GetEncodingPriority(TTF_EncodingRecord const * restrict record)
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

static int GetRecordFormat(BYTES1 * restrict cmapAddr,
		TTF_EncodingRecord const * restrict record)
{
	BYTES1 * restrict addr = cmapAddr + ru32(record->offset);
	return ru16(*(BYTES2 * restrict) addr);
}

static int IsSupportedFormat(int format)
{
	/* Wanted Formats: 4, 6, 12 */
	switch (format) {
	case 4:
	case 6:
		return 1;
	default:
		return 0;
	}
}

static SKR_Status Parse_cmap_format4(SKR_Font * restrict font, unsigned long offset)
{
	font->mappingFormat = 4;

	BYTES1 * restrict fmt4Addr = (BYTES1 *) font->data + font->cmap.offset + offset;
	TTF_cmap_format4 const * restrict table = (TTF_cmap_format4 const *) fmt4Addr;
	SKR_cmap_format4 * restrict fmt4 = &font->mapping.format4;

	int segCountX2 = ru16(table->segCountX2);
	SKR_assert(segCountX2 % 2 == 0);
	fmt4->segCount = segCountX2 / 2;

	unsigned long baseOffset = (BYTES1 *) table - (BYTES1 *) font->data;
	fmt4->endCodes = baseOffset + 14;
	fmt4->startCodes = fmt4->endCodes + segCountX2 + 2;
	fmt4->idDeltas = fmt4->startCodes + segCountX2;
	fmt4->idRangeOffsets = fmt4->idDeltas + segCountX2;

	return SKR_SUCCESS;
}

static SKR_Status Parse_cmap_format6(SKR_Font * restrict font, unsigned long offset)
{
	font->mappingFormat = 6;

	BYTES1 * restrict addr = (BYTES1 *) font->data + font->cmap.offset + offset;
	TTF_cmap_format6 const * restrict table = (TTF_cmap_format6 const *) addr;
	SKR_cmap_format6 * restrict fmt = &font->mapping.format6;

	fmt->firstCode = ru16(table->firstCode);
	fmt->entryCount = ru16(table->entryCount);
	fmt->glyphIndexArray = font->cmap.offset + offset + 10;

	return SKR_SUCCESS;
}

static SKR_Status Parse_cmap(SKR_Font * restrict font)
{
	BYTES1 * restrict cmapAddr = (BYTES1 *) font->data + font->cmap.offset;
	TTF_cmap * restrict cmap = (TTF_cmap *) cmapAddr;

	SKR_Status s = ru16(cmap->version) == 0 ? SKR_SUCCESS : SKR_FAILURE;
	if (s) return s;

	TTF_EncodingRecord const * restrict record;
	int bestPriority = INT_MAX;
	int numTables = ru16(cmap->numTables);
	for (int i = 0; i < numTables; ++i) {
		TTF_EncodingRecord const * restrict contender = &cmap->encodingRecords[i];
		int priority = GetEncodingPriority(contender);
		int format = GetRecordFormat(cmapAddr, contender);

		if (priority >= bestPriority) continue;
		if (!IsSupportedFormat(format)) continue;

		record = contender;
		bestPriority = priority;
	}
	s = bestPriority < INT_MAX ? SKR_SUCCESS : SKR_FAILURE;
	if (s) return s;

	int format = GetRecordFormat(cmapAddr, record);
	switch (format) {
	case 4:
		s = Parse_cmap_format4(font, ru32(record->offset));
		break;
	case 6:
		s = Parse_cmap_format6(font, ru32(record->offset));
		break;
	default:
		SKR_assert(0);
	}

	return s;
}

SKR_Status skrInitializeFont(SKR_Font * restrict font)
{
	SKR_Status s;
	s = ExtractOffsets(font);
	if (s) return s;
	s = Parse_head(font);
	if (s) return s;
	s = Parse_hhea(font);
	if (s) return s;
	s = Parse_maxp(font);
	if (s) return s;
	s = Parse_cmap(font);
	return s;
}

