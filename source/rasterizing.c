static void RasterizeDot(Point beg, Point end, RasterCell * dest, long stride)
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

	RasterCell * cell = &dest[stride * py + px];

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

static void RasterizeLine(Line line, RasterCell * dest, long stride)
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

		RasterizeDot(prev_pt, pt, dest, stride);

		prev_t = t;
		prev_pt = pt;
	}

	RasterizeDot(prev_pt, line.end, dest, stride);
}

void skrRasterizeLines(
	LineBuffer const * source,
	RasterCell * dest,
	SKR_Dimensions dim)
{
	for (int i = 0; i < source->count; ++i) {
		RasterizeLine(source->elems[i], dest, dim.width);
	}
}

/*
skrCastImage() right now is mostly a stub. Further on in development it should
also do conversion to user-specified pixel formats, simple color fill,
gamma correction and sub-pixel rendering (if enabled).
*/

void skrCastImage(
	RasterCell const * source,
	unsigned char * dest,
	SKR_Dimensions dim)
{
	for (long r = 0; r < dim.height; ++r) {
		long acc = 0; // in the range -127 - 127
		for (long c = 0; c < dim.width; ++c) {
			RasterCell const * cell = &source[dim.width * r + c];
			int windingAndCover = cell->windingAndCover, area = cell->area;
			assert(windingAndCover >= -127 && windingAndCover <= 127);
			assert(area >= 0 && area <= 254);
			int value = acc + windingAndCover * area / 254; // in the range -127 - 127
			assert(value >= -127 && value <= 127);
			int scaledValue = value * 255 / 127; // in the range -255 - 255
			assert(scaledValue >= -255 && scaledValue <= 255);
			// TODO use standardized winding direction to obviate the need for this abs()
			dest[dim.width * r + c] = abs(scaledValue);
			acc += windingAndCover;
		}
		assert(acc == 0);
	}
}

