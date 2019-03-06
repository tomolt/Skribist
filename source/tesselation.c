static double manhattan_distance(Point a, Point b)
{
	return fabs(a.x - b.x) + fabs(a.y - b.y);
}

static int is_flat(Curve curve, double flatness)
{
	Point mid = interp_points(curve.beg, curve.end);
	double dist = manhattan_distance(curve.ctrl, mid);
	return dist <= flatness;
}

static void split_curve(Curve curve, Curve segments[2])
{
	Point ctrl0 = interp_points(curve.beg, curve.ctrl);
	Point ctrl1 = interp_points(curve.ctrl, curve.end);
	Point pivot = interp_points(ctrl0, ctrl1);
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

void skrBeginTesselating(CurveList const *source,
	Transform transform, CurveStack *stack)
{
	assert(stack->space >= source->count);
	for (int i = 0; i < source->count; ++i) {
		Curve curve = trf_curve(source->elems[i], transform);
		stack->elems[stack->top] = curve;
		++stack->top;
	}
}

void skrContinueTesselating(CurveStack *stack, double flatness)
{
	do {
		--stack->top;
		Curve curve = stack->elems[stack->top];
		if (is_flat(curve, flatness)) {
			raster_line((Line) { curve.beg, curve.end });
		} else {
			assert(stack->top + 2 <= stack->space);
			split_curve(curve, &stack->elems[stack->top]);
			stack->top += 2;
		}
	} while (stack->top > 0);
}
