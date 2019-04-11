static void RasterizeDot(Point beg, Point end, RasterCell * dest, SKR_Dimensions dims)
{
	// quantized beg & end coordinates
	long qbx = round(beg.x * 1024.0);
	long qby = round(beg.y * 1024.0);
	long qex = round(end.x * 1024.0);
	long qey = round(end.y * 1024.0);
	// pixel coordinates
	long px = min(qbx, qex) / 1024;
	long py = min(qby, qey) / 1024;
	// fractional beg & end coordinates
	long fbx = qbx - px * 1024;
	long fby = qby - py * 1024;
	long fex = qex - px * 1024;
	long fey = qey - py * 1024;

	SKR_assert(px >= 0 && px < dims.width);
	SKR_assert(py >= 0 && py < dims.height);

	RasterCell * cell = &dest[dims.width * py + px];

	long tailValue = fey - fby; // winding * cover
	long area = labs(fex - fbx) / 2 + 1024 - max(fex, fbx);
	long edgeValue = tailValue * area / 1024;

	cell->tailValue += tailValue;
	cell->edgeValue += edgeValue;
}

/*
RasterizeLine() is intended to take in a single line and pass it on as a sequence of dots.
Its algorithm is actually fairly simple: It computes the exact intervals at
which the line crosses a horizontal or vertical pixel edge respectively, and
orders them based on the variable scalar in the line equation.
*/

static void RasterizeLine(Line line, RasterCell * dest, SKR_Dimensions dims)
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

		RasterizeDot(prev_pt, pt, dest, dims);

		prev_t = t;
		prev_pt = pt;
	}

	// TODO reevaluate the need for the condition
	if (!(prev_pt.x == line.end.x && prev_pt.y == line.end.y)) {
		RasterizeDot(prev_pt, line.end, dest, dims);
	}
}

static void DrawScaledLine(Line line, RasterCell * dest, SKR_Dimensions dims)
{
	RasterizeLine(line, dest, dims);
}

#include <math.h> // TODO get rid of

// TODO take monitor gamma i guess?
static int LinearToGamma(int linear)
{
	double L = linear / 255.0;
	double S;
	if (L <= 0.0031308) {
		S = L * 12.92;
	} else {
		S = 1.055 * pow(L, 1/2.4) - 0.055;
	}
	return round(S * 255.0);
}

void skrCastImage(
	RasterCell const * source,
	unsigned char * dest,
	SKR_Dimensions dim)
{
	for (long r = 0; r < dim.height; ++r) {
		long acc = 0;
		for (long c = 0; c < dim.width; ++c) {
			RasterCell const * cell = &source[dim.width * r + c];
			int edgeValue = cell->edgeValue, tailValue = cell->tailValue;
			int value = acc + edgeValue;
			int scaledValue = value * 255 / 1024;
			int linearValue = max(scaledValue, 0);
			int gammaValue = LinearToGamma(linearValue);
			dest[dim.width * r + c] = gammaValue;
			acc += tailValue;
		}
		// SKR_assert(acc == 0);
	}
}

