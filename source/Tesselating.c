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

static void DrawScaledCurve(Curve curve, RasterCell * dest, SKR_Dimensions dims)
{
	if (IsFlat(curve, 0.5)) {
		Line line = { curve.beg, curve.end };
		DrawScaledLine(line, dest, dims);
	} else {
		Curve segments[2];
		SplitCurve(curve, segments);
		DrawScaledCurve(segments[0], dest, dims);
		DrawScaledCurve(segments[1], dest, dims);
	}
}

static Point TransformPoint(Point point, Transform trf)
{
	return (Point) {
		point.x * trf.scale.x + trf.move.x,
		point.y * trf.scale.y + trf.move.y };
}

static Line TransformLine(Line line, Transform trf)
{
	return (Line) { TransformPoint(line.beg, trf), TransformPoint(line.end, trf) };
}

static Curve TransformCurve(Curve curve, Transform trf)
{
	return (Curve) { TransformPoint(curve.beg, trf),
		TransformPoint(curve.ctrl, trf), TransformPoint(curve.end, trf) };
}

static void DrawLine(Line line, Transform trf, RasterCell * raster, SKR_Dimensions dims)
{
	Line scaledLine = TransformLine(line, trf);
	DrawScaledLine(scaledLine, raster, dims);
}

static void DrawCurve(Curve curve, Transform trf, RasterCell * raster, SKR_Dimensions dims)
{
	Curve scaledCurve = TransformCurve(curve, trf);
	DrawScaledCurve(scaledCurve, raster, dims);
}
