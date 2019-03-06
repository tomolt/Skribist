static double ManhattanDistance(Point a, Point b)
{
	return fabs(a.x - b.x) + fabs(a.y - b.y);
}

static int IsFlat(Curve curve, double flatness)
{
	Point mid = Midpoint(curve.beg, curve.end);
	double dist = ManhattanDistance(curve.ctrl, mid);
	return dist <= flatness;
}

static void SplitCurve(Curve curve, Curve segments[2])
{
	Point ctrl0 = Midpoint(curve.beg, curve.ctrl);
	Point ctrl1 = Midpoint(curve.ctrl, curve.end);
	Point pivot = Midpoint(ctrl0, ctrl1);
	segments[0] = (Curve) { curve.beg, ctrl0, pivot };
	segments[1] = (Curve) { pivot, ctrl1, curve.end };
}

static Point TransformPoint(Point point, Transform trf)
{
	return (Point) {
		point.x * trf.scale.x + trf.move.x,
		point.y * trf.scale.y + trf.move.y };
}

static Curve TransformCurve(Curve curve, Transform trf)
{
	return (Curve) { TransformPoint(curve.beg, trf),
		TransformPoint(curve.ctrl, trf), TransformPoint(curve.end, trf) };
}

void skrBeginTesselating(CurveList const *source,
	Transform transform, CurveStack *stack)
{
	assert(stack->space >= source->count);
	for (int i = 0; i < source->count; ++i) {
		Curve curve = TransformCurve(source->elems[i], transform);
		stack->elems[stack->top] = curve;
		++stack->top;
	}
}

void skrContinueTesselating(CurveStack *stack, double flatness)
{
	while (stack->top > 0) {
		--stack->top;
		Curve curve = stack->elems[stack->top];
		if (IsFlat(curve, flatness)) {
			raster_line((Line) { curve.beg, curve.end });
		} else {
			assert(stack->top + 2 <= stack->space);
			SplitCurve(curve, &stack->elems[stack->top]);
			stack->top += 2;
		}
	}
}
