#include <stdint.h>

#include "Skribist.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// TODO platform-independent "deterministic" prng

char const * WordList[] = {
	"lorem", "ipsum", "dolor", "sit", "amet,",
	"consectetur", "adipisci elit,"
};

int const WordCount = sizeof(WordList) / sizeof(*WordList);

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

static SKR_Status draw_word(SKR_Font * font, float size, char const * word)
{
	SKR_Status s;

	int count;
	SKR_Assembly assembly[100];
	s = skrAssembleStringUTF8(font, word, size, assembly, &count);

	SKR_Bounds bounds;
	s = skrGetAssemblyBounds(font, assembly, count, &bounds);
	if (s) return s;

	SKR_Dimensions dims = {
		.width  = bounds.xMax - bounds.xMin,
		.height = bounds.yMax - bounds.yMin };

	unsigned long cellCount = skrCalcCellCount(dims);
	RasterCell * raster = calloc(cellCount, sizeof(RasterCell));
	unsigned char * image = malloc(4 * dims.width * dims.height);

	s = skrDrawAssembly(font, assembly, count, raster, bounds);
	if (s) return s;

	skrExportImage(raster, image, dims);

	free(raster);
	free(image);

	return SKR_SUCCESS;
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
	ret = read_file("../Ubuntu-C.ttf", (void **) &rawData);
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
	unsigned long long iterations = 0;

	do {
		int size = 10 + (rand() % 50);
		char const * word = WordList[rand() % WordCount];

		s = draw_word(&font, size, word);
		if (!s) ++iterations;

		clock_gettime(CLOCK_MONOTONIC_RAW, &nowTime); // TODO error handling
		elapsedTime = time_in_seconds(&nowTime) - time_in_seconds(&startTime);
	} while (elapsedTime < seconds);

	printf("SUMMARY:\n");
	printf("Ran %lld iterations in %f seconds,\n", iterations, elapsedTime);
	printf("Resulting in an average speed of %f kHz.\n", iterations / (elapsedTime * 1000.0));

	free(rawData);

	return EXIT_SUCCESS;
}
