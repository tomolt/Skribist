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

extern int16_t olt_GLOBAL_raster[WIDTH * HEIGHT];
extern uint8_t olt_GLOBAL_image[WIDTH * HEIGHT];

void olt_INTERN_raster_curve(Curve curve, Transform transform);

void olt_INTERN_gather(void);
