#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define sign(x) ((x) >= 0.0 ? 1.0 : -1.0)

RasterCell olt_GLOBAL_raster[WIDTH * HEIGHT];
uint8_t olt_GLOBAL_image[WIDTH * HEIGHT];

static void raster_dot(Point beg, Point end)
{
	// quantized beg & end coordinates
	long qbx = round(beg.x * 127.0);
	long qby = round(beg.y * 127.0);
	long qex = round(end.x * 127.0);
	long qey = round(end.y * 127.0);
	// pixel coordinates
	int px = min(qbx, qex) / 127;
	int py = min(qby, qey) / 127;
	// quantized corner coordinates
	long qcx = px * 127;
	long qcy = py * 127;
	// fractional beg & end coordinates
	int fbx = qbx - qcx;
	int fby = qby - qcy;
	int fex = qex - qcx;
	int fey = qey - qcy;

	RasterCell *cell = &olt_GLOBAL_raster[WIDTH * py + px];

	int winding = sign(fey - fby);
	int cover = abs(fey - fby); // in the range 0 - 127
	cell->windingAndCover += winding * cover; // in the range -127 - 127

	// TODO clamp
	cell->area += abs(fex - fbx) + 254 - 2 * max(fex, fbx); // in the range 0 - 254
}

/*

raster_line() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.

*/

static void raster_line(Line line)
{
	Point diff = { line.end.x - line.beg.x, line.end.y - line.beg.y };

	double sx, sy; // step size along each axis
	double xt, yt; // t of next vertical / horizontal intersection

	if (diff.x != 0.0) {
		sx = fabs(1.0 / diff.x);
		if (diff.x > 0.0) {
			xt = sx * (ceil(line.beg.x) - line.beg.x);
		} else {
			xt = sx * (line.beg.x - floor(line.beg.x));
		}
	} else {
		sx = 0.0;
		xt = 9.9;
	}

	if (diff.y != 0.0) {
		sy = fabs(1.0 / diff.y);
		if (diff.y > 0.0) {
			yt = sy * (ceil(line.beg.y) - line.beg.y);
		} else {
			yt = sy * (line.beg.y - floor(line.beg.y));
		}
	} else {
		sy = 0.0;
		yt = 9.9;
	}

	double prev_t = 0.0;
	Point prev_pt = line.beg, pt = prev_pt;

	while (xt <= 1.0 || yt <= 1.0) {
		double t;

		if (xt < yt) {
			t = xt;
			xt += sx;
		} else {
			t = yt;
			yt += sy;
		}

		if (t == prev_t) continue;

		pt.x = line.beg.x + t * diff.x;
		pt.y = line.beg.y + t * diff.y;

		raster_dot(prev_pt, pt);

		prev_t = t;
		prev_pt = pt;
	}

	raster_dot(prev_pt, line.end);
}
