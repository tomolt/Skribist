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
