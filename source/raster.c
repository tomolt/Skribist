#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define sign(x) ((x) >= 0.0 ? 1.0 : -1.0)

RasterCell olt_GLOBAL_raster[WIDTH * HEIGHT];
uint8_t olt_GLOBAL_image[WIDTH * HEIGHT];

static Line cns_line(Point beg, Point end)
{
	return (Line) { beg, (Point) { end.x - beg.x, end.y - beg.y } };
}

/*

cns_dot() is completely flawed right now and has to be redone.
floating point inaccuracies can lead to wrong pixel coordinates
if computed naively, so we add a small epsilon before rounding down.
this however means that beg.x - px could become negative, which is
something we *really* don't want. However, we also can't just round
to nearest instead of down, because only one of the coordinates will
be pixel-aligned in the common case.

*/

static Dot cns_dot(Point beg, Point end)
{
	int px = min(beg.x, end.x) + 0.001; // TODO cleanup
	int py = min(beg.y, end.y) + 0.001; // TODO cleanup
	int bx = round((beg.x - px) * 255.0);
	int by = round((beg.y - py) * 255.0);
	int ex = round((end.x - px) * 255.0);
	int ey = round((end.y - py) * 255.0);
	return (Dot) { px, py, bx, by, ex, ey };
}

static void raster_dot(Dot dot)
{
	RasterCell *cell = &olt_GLOBAL_raster[WIDTH * dot.py + dot.px];

	int winding = sign(dot.ey - dot.by); // FIXME more robust way?
	int cover = abs(dot.ey - dot.by); // in the range 0 - 255
	cell->windingAndCover += winding * cover;

	cell->area += 255 + abs(dot.ex - dot.bx) / 2 - max(dot.bx, dot.ex); // in the range 0 - 255
}

/*

raster_line() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.

*/

static void raster_line(Line line)
{
	double sx, sy; // step size along each axis
	double xt, yt; // t of next vertical / horizontal intersection

	if (line.diff.x != 0.0) {
		sx = fabs(1.0 / line.diff.x);
		if (line.diff.x > 0.0) {
			xt = sx * (ceil(line.beg.x) - line.beg.x);
		} else {
			xt = sx * (line.beg.x - floor(line.beg.x));
		}
	} else {
		sx = 0.0;
		xt = 9.9;
	}

	if (line.diff.y != 0.0) {
		sy = fabs(1.0 / line.diff.y);
		if (line.diff.y > 0.0) {
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

		double td = t - prev_t;
		pt.x += td * line.diff.x;
		pt.y += td * line.diff.y;

		raster_dot(cns_dot(prev_pt, pt));

		prev_t = t;
		prev_pt = pt;
	}

	Point last_pt = { line.beg.x + line.diff.x, line.beg.y + line.diff.y };
	raster_dot(cns_dot(prev_pt, last_pt));
}

static Point interp_curve(Curve curve)
{
	double ax = curve.beg.x - 2.0 * curve.ctrl.x + curve.end.x;
	double ay = curve.beg.y - 2.0 * curve.ctrl.y + curve.end.y;
	double bx = 2.0 * (curve.ctrl.x - curve.beg.x);
	double by = 2.0 * (curve.ctrl.y - curve.beg.y);
	double x = (ax / 2.0 + bx) / 2.0 + curve.beg.x;
	double y = (ay / 2.0 + by) / 2.0 + curve.beg.y;
	return (Point) { x, y };
}

static double manhattan_distance(Point a, Point b)
{
	return fabs(a.x - b.x) + fabs(a.y - b.y);
}

static int is_flat(Curve curve)
{
	Point mid = interp_points(curve.beg, curve.end);
	double dist = manhattan_distance(curve.ctrl, mid);
	return dist <= 0.5;
}

static void split_curve(Curve curve, Curve segments[2])
{
	Point pivot = interp_curve(curve);
	Point ctrl0 = interp_points(curve.beg, curve.ctrl);
	Point ctrl1 = interp_points(curve.ctrl, curve.end);
	segments[0] = (Curve) { curve.beg, ctrl0, pivot };
	segments[1] = (Curve) { pivot, ctrl1, curve.end };
}

static Point trf_point(Point point, Transform trf)
{
	return (Point) {
		point.x * trf.scale.x + trf.move.x,
		point.y * trf.scale.y + trf.move.y };
}

static Curve trf_curve(Curve curve, Transform trf)
{
	return (Curve) { trf_point(curve.beg, trf),
		trf_point(curve.ctrl, trf), trf_point(curve.end, trf) };
}

static void raster_curve(Curve curve, Transform transform)
{
	if (is_flat(curve)) {
		raster_line(cns_line(curve.beg, curve.end));
	} else {
		Curve segments[2];
		split_curve(curve, segments);
		raster_curve(segments[0], transform); // TODO don't overflow the stack
		raster_curve(segments[1], transform); // in pathological cases.
	}
}

void olt_INTERN_raster_curve(Curve curve, Transform transform)
{
	Curve rasterCurve = trf_curve(curve, transform);
	raster_curve(rasterCurve, transform);
}
