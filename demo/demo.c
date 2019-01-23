#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define WIDTH 256
#define HEIGHT 256

typedef struct {
	double x, y;
} point_t;

typedef struct {
	point_t pts[2];
} line_t;

static uint8_t image[WIDTH * HEIGHT];

static void split_line(line_t line)
{
	double sx = line.pts[0].x;
	double sy = line.pts[0].y;
	double dx = line.pts[1].x - line.pts[0].x;
	double dy = line.pts[1].y - line.pts[0].y;

	double minx = min(line.pts[0].x, line.pts[1].x);
	double miny = min(line.pts[0].y, line.pts[1].y);
	double maxx = max(line.pts[0].x, line.pts[1].x);
	double maxy = max(line.pts[0].y, line.pts[1].y);

	double ts[1000];
	int ts_top = 0;

	if (dx != 0.0) {
		for (int wx = ceil(minx); wx <= floor(maxx); ++wx) {
			double t = (wx - sx) / dx;
			ts[ts_top++] = t;
		}
	}

	if (dy != 0.0) {
		for (int wy = ceil(miny); wy <= floor(maxy); ++wy) {
			double t = (wy - sy) / dy;
			ts[ts_top++] = t;
		}
	}

	for (int i = 0; i < ts_top; ++i) {
		double t = ts[i];
		printf("t = %f, f(t) = (%f, %f)\n", t, sx + t * dx, sy + t * dy);
	}
}

static void fmt_le_dword(char *buf, uint32_t v)
{
	buf[0] = v & 0xFF;
	buf[1] = v >> 8 & 0xFF;
	buf[2] = v >> 16 & 0xFF;
	buf[3] = v >> 24 & 0xFF;
}

static void write_bmp(void)
{
	char hdr[54] = { 0 };
	// Header
	hdr[0] = 'B';
	hdr[1] = 'M';
	fmt_le_dword(&hdr[2], 54 + 3 * WIDTH * HEIGHT); // size of file
	hdr[10] = 54; // offset to image data
	// InfoHeader
	hdr[14] = 40; // size of InfoHeader
	fmt_le_dword(&hdr[18], WIDTH);
	fmt_le_dword(&hdr[22], HEIGHT);
	hdr[26] = 1; // color planes
	hdr[28] = 24; // bpp
	fwrite(hdr, 1, 54, stdout);
	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) {
			unsigned char c = image[WIDTH * y + x];
			fputc(c, stdout); // r
			fputc(c, stdout); // g
			fputc(c, stdout); // b
		}
	}
}

int main(int argc, char const *argv[])
{
	line_t line = { { { 5.5, 3.5 }, { 182.3, 101.1 } } };
	split_line(line);
	// write_bmp();
	return EXIT_SUCCESS;
}
