# Roadmap
### Already implemented
- The basics of loading TTF files
- Loading simple glyph outlines
- A rasterizer with pixel-perfect anti-aliasing
- An example program outputting BMP files
- Sample code to mmap a file on Windows or Unix (in limbo right now)
### To be done before v1.0
- Fix that one 1-pixel discrepancy border glitch
- Manual array bounds checking in the entire TTF loader
- Verifying min / max values in TTF data
- cmap
- Text composing
- Kerning
- Output format controllable by a generous list of enums ala OpenGL
- Gamma Correction
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
- Optional sub-pixel rendering for LCD screens
- Ideomatic C++ wrapper
### Crackpot ideas that may or may not be worth the effort
- Replacing the entire rasterizer with a new one entirely based on Signed Distance Fields
- Hinting (huge undertaking, but at least not encumbered by patents anymore)
- Optional transparent caching layer over the entire API for easy speedups
