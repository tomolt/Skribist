#include <stdint.h>

#include "Skribist.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static double time_in_seconds(struct timespec * ts)
{
    return (double) ts->tv_sec + (double) ts->tv_nsec / 1000000000.0;
}

static int read_file(char const *filename, void **addr)
{
	FILE *file = fopen(filename, "rw");
	if (file == NULL) {
		return -1;
	}
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	unsigned char *data = malloc(length);
	if (data == NULL) {
		fclose(file);
		return -1;
	}
	long count = fread(data, 1, length, file);
	if (count != length) {
		free(data);
		fclose(file);
		return -1;
	}
	fclose(file);
	*addr = data;
	return 0;
}

static long long OutlineCounter;

static void draw_outline(SKR_Font const * font, Glyph glyph, SKR_Transform transform1)
{
	SKR_Bounds bounds;
	skrGetOutlineBounds(font, glyph, transform1, &bounds);

	SKR_Transform transform2 = transform1;
	transform2.xMove -= bounds.xMin;
	transform2.yMove -= bounds.yMin;

	SKR_Dimensions dims = {
		.width  = bounds.xMax - bounds.xMin,
		.height = bounds.yMax - bounds.yMin };

	unsigned long cellCount = skrCalcCellCount(dims);
	RasterCell * raster = calloc(cellCount, sizeof(RasterCell));
	unsigned char * image = calloc(dims.width * dims.height, 4);

	SKR_Status s = skrDrawOutline(font, glyph, transform2, raster, dims);

	if (!s) {
		++OutlineCounter;
		skrProcessRaster(raster, dims);
		skrExportImage(raster, image, dims);
	}

	free(raster);
	free(image);
}

int main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: stress <seconds>\n");
		return EXIT_FAILURE;
	}

	double seconds = atof(argv[1]);
	if (seconds <= 0.0) {
		fprintf(stderr, "<seconds> should not be less than or equal 0.\n");
		return EXIT_FAILURE;
	}

	int ret;
	SKR_Status s = SKR_SUCCESS;

	unsigned char *rawData;
	// TODO better location for example font file
	ret = read_file("../examples/Ubuntu-C.ttf", (void **) &rawData);
	if (ret != 0) {
		fprintf(stderr, "Unable to open TTF font file.\n");
		return EXIT_FAILURE;
	}

	skrInitializeLibrary();

	SKR_Font font = {
		.data = rawData,
		.length = 0 }; // FIXME
	s = skrInitializeFont(&font);
	if (s != SKR_SUCCESS) {
		fprintf(stderr, "Unable to read TTF font file.\n");
		return EXIT_FAILURE;
	}

	struct timespec startTime, nowTime;
	clock_gettime(CLOCK_MONOTONIC_RAW, &startTime); // TODO error handling
	double elapsedTime; // in seconds

	for (;;) {
		for (int charCode = 0; charCode < 500; ++charCode) {
			for (double size = 10.0; size <= 60.0; size += 2.0) {
				Glyph glyph = skrGlyphFromCode(&font, charCode);
				SKR_Transform transform = { size, size, 0.0, 0.0 };
				draw_outline(&font, glyph, transform);

				clock_gettime(CLOCK_MONOTONIC_RAW, &nowTime); // TODO error handling
				elapsedTime = time_in_seconds(&nowTime) - time_in_seconds(&startTime);
				if (elapsedTime >= seconds) goto end_test;
			}
		}
	}
end_test:

	printf("SUMMARY:\n");
	printf("Drew %lld outlines in %f seconds,\n", OutlineCounter, elapsedTime);
	printf("Resulting in an average speed of %f kHz.\n", OutlineCounter / (elapsedTime * 1000.0));

	free(rawData);

	return EXIT_SUCCESS;
}
