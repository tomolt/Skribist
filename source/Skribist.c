
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
#include "rasterizing.h"
#include "reading.h"

#include "casting.c"
#include "header.c"
#include "parsing.c"
#include "rasterizing.c"
