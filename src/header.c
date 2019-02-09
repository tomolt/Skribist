#include "header.h"

#include <stdint.h>
#include "reading.h"

#include <string.h>

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
		cmp = strncmp(offt->entries[idx].tag, "glyf", 4);
		if (!cmp) cache.glyf = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	do {
		cmp = strncmp(offt->entries[idx].tag, "head", 4);
		if (!cmp) cache.head = ru32(offt->entries[idx].offset);
	} while (cmp < 0 && idx < count && ++idx);
	return cache;
}
