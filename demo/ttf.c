/*

Copyright 2019 Thomas Oltmann

---- LICENSE ----

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdint.h>

#include "mapping.h"
#include "header.h"
#include "reading.h"
#include "rational.h"
#include "outline.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

static void print_head(BYTES1 *ptr)
{
	headTbl *head = (headTbl *) ptr;
	int unitsPerEm = ru16(head->unitsPerEm);
	printf("unitsPerEm: %d\n", unitsPerEm);
}

int main(int argc, char const *argv[])
{
	(void) argc, (void) argv;

	int ret;

	BYTES1 *rawData;
	mapping_handle_t mapping;
	ret = olt_INTERN_map_file("../demo/Ubuntu-C.ttf", (void **) &rawData, &mapping);
	assert(ret == 0);

	OffsetCache offcache = olt_INTERN_cache_offsets(rawData);

	print_head(rawData + offcache.head);

	olt_INTERN_parse_outline(rawData + offcache.glyf);

	ret = olt_INTERN_unmap_file((void *) rawData, mapping);
	assert(ret == 0);
	return EXIT_SUCCESS;
}
