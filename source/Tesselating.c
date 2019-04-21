static float ManhattanDistance(Point a, Point b)
{
	return gabs(a.x - b.x) + gabs(a.y - b.y);
}

static int IsFlat(Curve curve, float flatness)
{
	Point mid = Midpoint(curve.beg, curve.end);
	float dist = ManhattanDistance(curve.ctrl, mid);
	return dist <= flatness;
}

static void SplitCurve(Curve curve, Curve segments[2])
{
	Point ctrl0 = Midpoint(curve.beg, curve.ctrl);
	Point ctrl1 = Midpoint(curve.ctrl, curve.end);
	Point pivot = Midpoint(ctrl0, ctrl1);
	segments[0] = (Curve) { curve.beg, pivot, ctrl0 };
	segments[1] = (Curve) { pivot, curve.end, ctrl1 };
}

static void DrawCurve(Workspace * restrict ws, Curve initialCurve)
{
	/*
	Ínstead of recursion we use an explicit stack here.
	This is to prevent stack overflows from occuring when drawing large text.
	TODO dynamically (re-)allocate this by the user.
	*/
	Curve stack[1000];
	stack[0] = initialCurve;
	int top = 1;
	while (top > 0) {
		Curve curve = stack[--top];
		if (IsFlat(curve, 0.5f)) {
			DrawLine(ws, *(Line *) &curve);
		} else {
			SKR_assert(top + 2 <= 1000);
			SplitCurve(curve, &stack[top]);
			top += 2;
		}
	}
}
