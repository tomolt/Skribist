static int SKR_strncmp(char const *a, char const *b, long n)
{
	for (long i = 0; i < n; ++i) {
		if (a[i] != b[i])
			return a[i] - b[i];
	}
	return 0;
}

typedef struct {
	char tag[4];
	BYTES4 checksum;
	BYTES4 offset;
	BYTES4 length;
} offsetEnt;

typedef struct {
	BYTES4 scalerType;
	BYTES2 numTables;
	BYTES2 searchRange;
	BYTES2 entrySelector;
	BYTES2 rangeShift;
	offsetEnt entries[];
} offsetTbl;

OffsetCache olt_INTERN_cache_offsets(void *addr)
{
	offsetTbl *offt = (offsetTbl *) addr;
	OffsetCache cache = { 0 };
	int count = ru16(offt->numTables);
	int idx = 0, cmp;
	do {
		cmp = SKR_strncmp(offt->entries[idx].tag, "glyf", 4);
		if (!cmp) cache.glyf = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	do {
		cmp = SKR_strncmp(offt->entries[idx].tag, "head", 4);
		if (!cmp) cache.head = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	do {
		cmp = SKR_strncmp(offt->entries[idx].tag, "loca", 4);
		if (!cmp) cache.loca = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	do {
		cmp = SKR_strncmp(offt->entries[idx].tag, "maxp", 4);
		if (!cmp) cache.maxp = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	return cache;
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
} headTbl;

short olt_GLOBAL_unitsPerEm;
short olt_GLOBAL_indexToLocFormat;

void olt_INTERN_parse_head(void *addr)
{
	headTbl *head = (headTbl *) addr;
	olt_GLOBAL_unitsPerEm = ru16(head->unitsPerEm);
	olt_GLOBAL_indexToLocFormat = ri16(head->indexToLocFormat);
	assert(olt_GLOBAL_indexToLocFormat == 0 || olt_GLOBAL_indexToLocFormat == 1);
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
} maxpTbl;

short olt_GLOBAL_numGlyphs;

void olt_INTERN_parse_maxp(void *addr)
{
	maxpTbl *maxp = (maxpTbl *) addr;
	assert(ru32(maxp->version) == 0x00010000);
	olt_GLOBAL_numGlyphs = ru16(maxp->numGlyphs);
}

/*
TODO maybe dedicated offset type
*/
unsigned long olt_INTERN_get_outline(void *locaAddr, Glyph glyph)
{
	int n = olt_GLOBAL_numGlyphs + 1;
	assert(glyph < n);
	if (!olt_GLOBAL_indexToLocFormat) {
		BYTES2 *loca = (BYTES2 *) locaAddr;
		return (unsigned long) ru16(loca[glyph]) * 2;
	} else {
		BYTES4 *loca = (BYTES4 *) locaAddr;
		return ru32(loca[glyph]);
	}
}
