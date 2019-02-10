/*

Copyright 2019 Thomas Oltmann

---- LICENSE ----

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

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
// TODO most fitting type
typedef unsigned long olt_Size;

typedef struct {
	void *userdata;
	void *(*alloc)(olt_Size, void *);
	void (*free)(void *, void *);
} olt_Allocator;

typedef struct olt_Font olt_Font;
typedef struct olt_Parse olt_Parse;
typedef struct olt_Tessel olt_Tessel;
typedef struct olt_Raster olt_Raster;

olt_Status olt_open_font_file(char const *filename, olt_Font *font);
olt_Status olt_open_font_memory(void const *addr, olt_Font *font);
olt_Status olt_close_font(olt_Font *font);

olt_Status olt_draw_nt_string(char const *string, olt_Font *font, void *image);
olt_Status olt_draw_len_string(unsigned long length,
	char const *string, olt_Font *font, void *image);

olt_Status olt_draw_glyph(olt_Font *font, void *image);

olt_Status olt_parse_outline(olt_Glyph glyph, olt_Font *font, olt_Parse *parse);
olt_Status olt_tessel_outline(olt_Parse *parse, olt_Tessel *tessel);
olt_Status olt_raster_outline(olt_Tessel *tessel, olt_Raster *raster);
olt_Status olt_gather_outline(olt_Format format, olt_Raster *raster, void *image);

#endif
