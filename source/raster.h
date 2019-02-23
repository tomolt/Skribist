// depends on stdint.h
// depends on outline.h

#ifdef OLT_INTERN_RASTER_H
#error multiple inclusion
#endif
#define OLT_INTERN_RASTER_H

#define WIDTH 256
#define HEIGHT 256

/*
The transformation order goes: first scale, then move.
*/
typedef struct {
	Point scale, move;
} Transform;

typedef struct {
	int8_t windingAndCover; // in the range -127 - 127
	uint8_t area; // in the range 0 - 254
} RasterCell;

#if 0
typedef struct {
	SkrFormat format;
	long width;
	long height;
	long stride;
	void *data;
} SkrImage;
#endif

extern RasterCell olt_GLOBAL_raster[WIDTH * HEIGHT];
extern uint8_t olt_GLOBAL_image[WIDTH * HEIGHT];

void olt_INTERN_raster_curve(Curve curve, Transform transform);

void olt_INTERN_gather(void);
