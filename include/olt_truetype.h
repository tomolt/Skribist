#ifndef OLT_TRUETYPE_H
#define OLT_TRUETYPE_H

/*
The layout of olt_Status aligns with posix return value conventions,
so you can compare it to 0 just as you would with any other posix call.
*/
typedef enum {
	OLT_FAILURE = -1,
	OLT_SUCCESS
} olt_Status;

typedef enum {
	OLT_BW_1_BIT,
	OLT_ALPHA_8_UINT,
	OLT_GRAY_8_SRGB,
	OLT_RGB_5_6_5_UINT,
	OLT_BGR_5_6_5_UINT,
	OLT_RGB_24_UINT,
	OLT_RGB_24_SRGB,
	OLT_BGR_24_UINT,
	OLT_BGR_24_SRGB,
	OLT_RGBA_32_UINT,
	OLT_RGBA_32_SRGB,
	OLT_BGRA_32_UINT,
	OLT_BGRA_32_SRGB,
	OLT_ALPHA_16_UINT,
	OLT_RGB_48_UINT,
	OLT_RGBA_64_UINT,
	OLT_RGB_96_FLOAT,
	OLT_RGBA_128_FLOAT
} olt_Format;

// TODO most fitting type
typedef unsigned long olt_Glyph;

typedef struct olt_Font olt_Font;
typedef struct olt_Parse olt_Parse;
typedef struct olt_Tessel olt_Tessel;
typedef struct olt_Raster olt_Raster;

olt_Status olt_open_file(char const *filename, olt_Font *font);
olt_Status olt_open_memory(void const *addr, olt_Font *font);

olt_Status olt_outline_draw(olt_Font *font, void *image);

olt_Status olt_outline_parse(olt_Font *font, olt_Glyph glyph, olt_Parse *parse);
olt_Status olt_outline_tessel(olt_Parse *parse, olt_Tessel *tessel);
olt_Status olt_outline_raster(olt_Tessel *tessel, olt_Raster *raster);
olt_Status olt_outline_gather(olt_Raster *raster, olt_Format format, void *image);

#endif
