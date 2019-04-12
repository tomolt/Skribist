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

static void DrawCurve(Curve curve, RasterCell * dest, SKR_Dimensions dims)
{
	/*
	TODO replace recursion here with iteration over an explicit stack.
	This is to prevent stack overflows from occuring when drawing large text.
	*/
	if (IsFlat(curve, 0.5)) {
		Line line = { curve.beg, curve.end };
		DrawLine(line, dest, dims);
	} else {
		Curve segments[2];
		SplitCurve(curve, segments);
		DrawCurve(segments[0], dest, dims);
		DrawCurve(segments[1], dest, dims);
	}
}
