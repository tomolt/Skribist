#include <stdint.h>

#include "Skribist.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <assert.h>

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
	unsigned char * image = calloc(dims.width * dims.height, sizeof(unsigned char));

	SKR_Status s = skrDrawOutline(font, glyph, transform2, raster, dims);

	if (!s) {
		skrCastImage(raster, image, dims);
	}

	free(raster);
	free(image);
}

int main()
{
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

	for (int i = 0; i < 100; ++i) {
		for (double size = 6.0; size <= 48.0; size += 2.0) {
			for (Glyph glyph = 0; glyph < 500; ++glyph) {
				SKR_Transform transform = { size, size, 0.0, 0.0 };
				draw_outline(&font, glyph, transform);
			}
		}
	}

	free(rawData);

	return EXIT_SUCCESS;
}
