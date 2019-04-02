#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define sign(x) ((x) >= 0.0 ? 1.0 : -1.0)

static void RasterizeDot(Point beg, Point end, SKR_Raster dest)
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

	RasterCell * cell = &dest.data[dest.width * py + px];

	int winding = sign(fey - fby);
	int cover = abs(fey - fby); // in the range 0 - 127
	cell->windingAndCover += winding * cover; // in the range -127 - 127

	// TODO clamp
	cell->area += abs(fex - fbx) + 254 - 2 * max(fex, fbx); // in the range 0 - 254
}

/*

RasterizeLine() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.

*/

static void RasterizeLine(Line line, SKR_Raster dest)
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

		RasterizeDot(prev_pt, pt, dest);

		prev_t = t;
		prev_pt = pt;
	}

	RasterizeDot(prev_pt, line.end, dest);
}

void skrRasterizeLines(
	LineBuffer const * source,
	SKR_Raster dest)
{
	for (int i = 0; i < source->count; ++i) {
		RasterizeLine(source->elems[i], dest);
	}
}
