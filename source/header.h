#ifdef OLT_INTERN_HEADER_H
#error multiple inclusion
#endif
#define OLT_INTERN_HEADER_H

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
