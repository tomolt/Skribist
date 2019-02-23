#ifdef OLT_INTERN_HEADER_H
#error multiple inclusion
#endif
#define OLT_INTERN_HEADER_H

typedef struct {
	unsigned long glyf;
	unsigned long head;
	unsigned long loca;
	unsigned long maxp;
} OffsetCache;

extern short olt_GLOBAL_unitsPerEm;
extern short olt_GLOBAL_indexToLocFormat;

OffsetCache olt_INTERN_cache_offsets(void *addr);
void olt_INTERN_parse_head(void *addr);
