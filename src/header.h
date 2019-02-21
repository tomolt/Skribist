#ifdef OLT_INTERN_HEADER_H
#error multiple inclusion
#endif
#define OLT_INTERN_HEADER_H

typedef struct {
	unsigned long glyf;
	unsigned long head;
} OffsetCache;

extern int olt_GLOBAL_unitsPerEm;

OffsetCache olt_INTERN_cache_offsets(void *addr);
void olt_INTERN_parse_head(void *addr);
