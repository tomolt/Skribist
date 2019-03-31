# Roadmap
### Already implemented
- The basics of loading TTF files
- Loading simple glyph outlines
- A rasterizer with pixel-perfect anti-aliasing
### To be done before v1.0
- Fix that one 1-pixel discrepancy border glitch
- Manual array bounds checking in the entire TTF loader
- Verifying min / max values in TTF data
- cmap
- Text composing
- Kerning
- General API cleanup
- Example program with SDL2
### Coming after v1.0
- Font Collections
- Compound glyphs
- Vertical composing
- Total independence from the C stdlib
- nostdlib example program maybe?
- UTF16 convenience functions
- SIMD rasterizer optimization
- A subset of the Unicode BiDi Algorithm
- More cmap encodings
- Utilising GPOS, GSUB, JUSTF tables
