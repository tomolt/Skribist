#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define WIDTH 256
#define HEIGHT 256
#define ANGLES 10

static uint8_t image[WIDTH * HEIGHT];

static void bresenham(int x0, int y0, int x1, int y1)
{
	int bx = MIN(x0, x1); // , by = MIN(y0, y1);
	int lx = abs(x1 - x0), ly = abs(y1 - y0);
	if (lx >= ly) {
		if (x0 > x1) {
			int xt = x0, yt = y0;
			x0 = x1, y0 = y1;
			x1 = xt, y1 = yt;
		}
		int es = y0 <= y1 ? 1 : -1; // error sign
		int eu = 2 * lx; // error unit
		int dy = 2 * ly;
		int y = y0;
		int ye = eu / 2; // error along y
		for (int x = bx; x <= bx + lx; ++x) {
			int c = 255 - ye * 255 / eu;
			image[y * WIDTH + x] = c;
			image[(y + es) * WIDTH + x] = 255 - c;
			ye += dy;
			y += es * (ye / eu);
			ye = ye % eu;
		}
	} else {
		if (y0 > y1) {
			int xt = x0, yt = y0;
			x0 = x1, y0 = y1;
			x1 = xt, y1 = yt;
		}
		double dx = (double) (x1 - x0) / (y1 - y0);
		double x = x0 + 0.5;
		for (int y = y0; y <= y1; ++y) {
			image[y * WIDTH + (int) x] = 255;
			x += dx;
		}
	}
}

static uint16_t u16(uint16_t v)
{
	// TODO
	return v;
}

static uint32_t u32(uint32_t v)
{
	// TODO
	return v;
}

static int32_t i32(int32_t v)
{
 // TODO
 return v;
}

static void write_bmp(void)
{
	char hdr[54] = { 0 };
	// Header
	hdr[0] = 'B';
	hdr[1] = 'M';
	*(uint32_t *) &hdr[2] = u32(54 + 3 * WIDTH * HEIGHT); // size of file
	*(uint32_t *) &hdr[10] = u32(54); // offset to image data
	// InfoHeader
	*(uint32_t *) &hdr[14] = u32(40); // size of InfoHeader
	*( int32_t *) &hdr[18] = i32(WIDTH);
	*( int32_t *) &hdr[22] = i32(HEIGHT);
	*(uint16_t *) &hdr[26] = u16(1); // color planes
	*(uint16_t *) &hdr[28] = u16(24); // bpp
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
	double mx = WIDTH / 2, my = HEIGHT / 2;
	double ri = 20;
	double ro = 0.5 * MIN(WIDTH, HEIGHT) - 20;
	for (int i = 0; i < ANGLES; ++i) {
		double angle = (2.0 * M_PI) * i / ANGLES;
		double xd = sin(angle);
		double yd = cos(angle);
		bresenham(
			(int) (mx + xd * ri), (int) (my + yd * ri),
			(int) (mx + xd * ro), (int) (my + yd * ro));
	}
	write_bmp();
	return EXIT_SUCCESS;
}
