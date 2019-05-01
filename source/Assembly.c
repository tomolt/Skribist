
static int GetCharCodeFromUTF8(char const * restrict * restrict ptr)
{
	int bytes = 0;
	for (int c = **ptr; c & 0x80; c <<= 1) ++bytes;
	if (bytes == 1 || bytes > 4) return '?';
	int code = *(*ptr)++ & (0x7F >> bytes);
	for (int i = 1; i < bytes; ++i, ++*ptr) {
		if ((**ptr & 0xC0) != 0x80) return '?';
		code = (code << 6) | (**ptr & 0x3F);
	}
	return code;
}

SKR_Status skrAssembleStringUTF8(SKR_Font * restrict font,
	char const * restrict line, float size,
	SKR_Assembly * restrict assembly, int * restrict count)
{
	// TODO watch for buffer overflows
	*count = 0;
	float x = 0.0f;
	while (*line != '\0') {
		int charCode = GetCharCodeFromUTF8(&line);
		Glyph glyph = skrGlyphFromCode(font, charCode);
		SKR_HorMetrics metrics;
		SKR_Status s = skrGetHorMetrics(font, glyph, &metrics);
		if (s) return s;
		assembly[(*count)++] = (SKR_Assembly) { glyph, size, x, 0.0f };
		x += metrics.advanceWidth * size;
	}
	return SKR_SUCCESS;
}

SKR_Status skrGetAssemblyBounds(SKR_Font * restrict font,
	SKR_Assembly * restrict assembly, int count, SKR_Bounds * restrict bounds)
{
	if (count <= 0) return SKR_FAILURE;
	SKR_Bounds total, next;
	SKR_Transform transform = {
		assembly[0].size, assembly[0].size, assembly[0].x, assembly[0].y };
	SKR_Status s = skrGetOutlineBounds(font, assembly[0].glyph, transform, &total);
	if (s) return s;
	for (int i = 1; i < count; ++i) {
		SKR_Transform transform = {
			assembly[i].size, assembly[i].size, assembly[i].x, assembly[i].y };
		s = skrGetOutlineBounds(font, assembly[i].glyph, transform, &next);
		if (s) return s;
		total.xMin = min(total.xMin, next.xMin);
		total.yMin = min(total.yMin, next.yMin);
		total.xMax = max(total.xMax, next.xMax);
		total.yMax = max(total.yMax, next.yMax);
	}
	*bounds = total;
	return SKR_SUCCESS;
}

SKR_Status skrDrawAssembly(SKR_Font * restrict font,
	SKR_Assembly * restrict assembly, int count,
	RasterCell * restrict raster, SKR_Bounds bounds)
{
	SKR_Dimensions dims = { bounds.xMax - bounds.xMin, bounds.yMax - bounds.yMin };
	for (int i = 0; i < count; ++i) {
		SKR_Assembly amb = assembly[i];
		SKR_Transform transform = { amb.size, amb.size,
			amb.x - bounds.xMin, amb.y - bounds.yMin };
		SKR_Status s = skrDrawOutline(font, amb.glyph, transform, raster, dims);
		if (s) return s;
	}
	return SKR_SUCCESS;
}

