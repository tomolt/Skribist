#include <stdint.h>

#include "../source/Reading.c" // FIXME remove this dependency
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

static void draw_outline(SKR_Font const * font, Glyph glyph, Transform transform)
{
	SKR_Rect rect = skrGetOutlineBounds(skrGetOutlineAddr(font, glyph));

	SKR_Dimensions dims = {
		.width  = ceil(rect.xMax * transform.scale.x + transform.move.x),
		.height = ceil(rect.yMax * transform.scale.y + transform.move.y) };

	RasterCell * raster = calloc(dims.width * dims.height, sizeof(RasterCell));
	unsigned char * image = calloc(dims.width * dims.height, sizeof(unsigned char));

	SKR_Status s = skrDrawOutline(font, glyph, transform, raster, dims);

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

	for (double size = 6.0; size <= 48.0; size += 2.0) {
		for (Glyph glyph = 0; glyph < 500; ++glyph) {
			Transform transform = {
				{ size / font.unitsPerEm, size / font.unitsPerEm },
				{ 64.0, 64.0 } };
			draw_outline(&font, glyph, transform);
		}
	}

	free(rawData);

	return EXIT_SUCCESS;
}
