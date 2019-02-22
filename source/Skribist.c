
/*
C stdlib headers - dependency on these should be removed as soon as possible.
*/
#include <math.h>
#include <stdlib.h>
#include <assert.h>

/*
C standard headers - we can use these even if we don't link with the standard library.
*/
#include <stdint.h>

#include "header.h"
#include "outline.h"
#include "raster.h"
#include "reading.h"

#include "gather.c"
#include "header.c"
#include "parse.c"
#include "raster.c"
